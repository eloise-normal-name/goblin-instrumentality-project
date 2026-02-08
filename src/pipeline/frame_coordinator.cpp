#include "frame_coordinator.h"

#include <windows.h>

#include "try.h"

FrameCoordinator::FrameCoordinator(D3D12Device& dev, D3D12FrameSync& sync,
								   D3D12CommandAllocators& alloc, RenderTargets& targets,
								   const PipelineConfig& cfg)
	: device(dev), frame_sync(sync), allocators(alloc), render_targets(targets), config(cfg) {
	uint32_t buffer_size	   = config.width * config.height * 4;
	DXGI_FORMAT encoder_format = DXGI_FORMAT_B8G8R8A8_UNORM;

	D3D12_RESOURCE_DESC resource_desc{
		.Dimension		  = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		.Width			  = config.width,
		.Height			  = config.height,
		.DepthOrArraySize = 1,
		.MipLevels		  = 1,
		.Format			  = encoder_format,
		.SampleDesc		  = {.Count = 1},
		.Flags			  = D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS,
	};

	D3D12_HEAP_PROPERTIES heap_props{
		.Type = D3D12_HEAP_TYPE_DEFAULT,
	};

	D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE;

	for (uint32_t i = 0; i < allocators.buffer_count; ++i) {
		Try
			| dev.device->CreateCommittedResource(&heap_props, heap_flags, &resource_desc,
												  D3D12_RESOURCE_STATE_COMMON, nullptr,
												  IID_PPV_ARGS(&encoder_textures[i]));
		encoder_states[i] = ResourceState::Common;
	}

	NV_ENC_BUFFER_FORMAT nvenc_format = DxgiFormatToNvencFormat(encoder_format);
	for (uint32_t i = 0; i < allocators.buffer_count; ++i)
		nvenc_d3d12.RegisterTexture(encoder_textures[i].Get(), config.width, config.height,
									nvenc_format);

	nvenc_d3d12.RegisterBitstreamBuffer(output_buffer.resource.Get(), buffer_size);

	encode_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!encode_fence_event)
		throw;
}

FrameCoordinator::~FrameCoordinator() {
	frame_sync.WaitForGpu();

	if (encode_fence_event)
		CloseHandle(encode_fence_event);
}

void FrameCoordinator::BeginFrame() {
	commands.Reset(allocators.GetAllocator(render_targets.current_frame_index));
}

void FrameCoordinator::EndFrame() {
	if (last_encode_fence_value > 0)
		Try | device.command_queue->Wait(frame_sync.fence.Get(), last_encode_fence_value);

	commands.TransitionResource(
		render_targets.render_targets[render_targets.current_frame_index].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);

	uint32_t frame_index			 = render_targets.current_frame_index;
	ID3D12Resource* encoder_resource = encoder_textures[frame_index].Get();
	ResourceState& encoder_state	 = encoder_states[frame_index];

	if (encoder_state != ResourceState::CopyDest) {
		commands.TransitionResource(encoder_resource, ToD3D12State(encoder_state),
									D3D12_RESOURCE_STATE_COPY_DEST);
		encoder_state = ResourceState::CopyDest;
	}

	commands.CopyResource(encoder_resource,
						  render_targets.render_targets[render_targets.current_frame_index].Get());

	if (encoder_state != ResourceState::Common) {
		commands.TransitionResource(encoder_resource, ToD3D12State(encoder_state),
									D3D12_RESOURCE_STATE_COMMON);
		encoder_state = ResourceState::Common;
	}

	commands.TransitionResource(
		render_targets.render_targets[render_targets.current_frame_index].Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PRESENT);

	ExecuteCommandList(device.command_queue.Get(), commands);
}

void FrameCoordinator::EncodeFrame(FrameData& output) {
	uint64_t gpu_fence_value = frame_sync.SignalFenceForFrame(render_targets.current_frame_index);

	SubmitFrameToEncoder();

	uint64_t output_fence_value = gpu_fence_value + 1;
	last_encode_fence_value		= output_fence_value;
	frame_sync.SetFenceEvent(output_fence_value, encode_fence_event);
	WaitForSingleObject(encode_fence_event, INFINITE);

	RetrieveEncodedFrame(output);

	++frame_count;
}

void FrameCoordinator::SubmitFrameToEncoder() {
	uint32_t texture_index = render_targets.current_frame_index;
	nvenc_d3d12.MapInputTexture(texture_index);

	if (nvenc_d3d12.textures.empty() || nvenc_d3d12.bitstream_buffers.empty())
		throw;

	const RegisteredTexture& texture = nvenc_d3d12.textures[texture_index];
	const BitstreamBuffer& bitstream = nvenc_d3d12.bitstream_buffers[0];

	void* encoder = nvenc_session.encoder;

	NV_ENC_MAP_INPUT_RESOURCE map_params{
		.version			= NV_ENC_MAP_INPUT_RESOURCE_VER,
		.registeredResource = bitstream.registered_ptr,
	};
	Try | nvenc_session.nvEncMapInputResource(encoder, &map_params);
	void* mapped_bitstream = map_params.mappedResource;

	NV_ENC_INPUT_RESOURCE_D3D12 input_resource{
		.version	  = NV_ENC_INPUT_RESOURCE_D3D12_VER,
		.pInputBuffer = texture.mapped_ptr,
		.inputFencePoint
		= {.version		= NV_ENC_FENCE_POINT_D3D12_VER,
		   .pFence		= frame_sync.fence.Get(),
		   .signalValue = frame_sync.fence_values[render_targets.current_frame_index]},
	};

	NV_ENC_OUTPUT_RESOURCE_D3D12 output_resource{
		.version	   = NV_ENC_OUTPUT_RESOURCE_D3D12_VER,
		.pOutputBuffer = mapped_bitstream,
	};
	output_resource.outputFencePoint.version  = NV_ENC_FENCE_POINT_D3D12_VER;
	output_resource.outputFencePoint.reserved = 0;
	output_resource.outputFencePoint.pFence	  = frame_sync.fence.Get();
	output_resource.outputFencePoint.signalValue
		= frame_sync.fence_values[render_targets.current_frame_index] + 1;
	output_resource.outputFencePoint.bSignal = 1;

	NV_ENC_PIC_PARAMS pic_params{
		.version		 = NV_ENC_PIC_PARAMS_VER,
		.inputWidth		 = config.width,
		.inputHeight	 = config.height,
		.inputPitch		 = config.width,
		.frameIdx		 = (uint32_t)frame_count,
		.inputTimeStamp	 = frame_count,
		.inputBuffer	 = &input_resource,
		.outputBitstream = &output_resource,
		.bufferFmt		 = texture.buffer_format,
		.pictureStruct	 = NV_ENC_PIC_STRUCT_FRAME,
	};

	Try | nvenc_session.nvEncEncodePicture(encoder, &pic_params)
		| nvenc_session.nvEncUnmapInputResource(encoder, mapped_bitstream);

	nvenc_d3d12.UnmapInputTexture(texture_index);
}

void FrameCoordinator::RetrieveEncodedFrame(FrameData& output) {
	if (nvenc_d3d12.bitstream_buffers.empty())
		throw;

	const BitstreamBuffer& bitstream = nvenc_d3d12.bitstream_buffers[0];

	void* encoder = nvenc_session.encoder;

	NV_ENC_MAP_INPUT_RESOURCE map_params{
		.version			= NV_ENC_MAP_INPUT_RESOURCE_VER,
		.registeredResource = bitstream.registered_ptr,
	};
	Try | nvenc_session.nvEncMapInputResource(encoder, &map_params);
	void* mapped_bitstream = map_params.mappedResource;

	NV_ENC_OUTPUT_RESOURCE_D3D12 output_resource{
		.version	   = NV_ENC_OUTPUT_RESOURCE_D3D12_VER,
		.pOutputBuffer = mapped_bitstream,
		.outputFencePoint
		= {.version		= NV_ENC_FENCE_POINT_D3D12_VER,
		   .pFence		= frame_sync.fence.Get(),
		   .signalValue = frame_sync.fence_values[render_targets.current_frame_index] + 1,
		   .bSignal		= 1},
	};

	NV_ENC_LOCK_BITSTREAM lock_params{
		.version		 = NV_ENC_LOCK_BITSTREAM_VER,
		.outputBitstream = &output_resource,
	};

	Try | nvenc_session.nvEncLockBitstream(encoder, &lock_params);

	output.data		   = (uint8_t*)lock_params.bitstreamBufferPtr;
	output.size		   = lock_params.bitstreamSizeInBytes;
	output.timestamp   = lock_params.outputTimeStamp;
	output.is_keyframe = (lock_params.pictureType == NV_ENC_PIC_TYPE_IDR
						  || lock_params.pictureType == NV_ENC_PIC_TYPE_I);

	nvenc_session.nvEncUnlockBitstream(encoder, &output_resource);
	Try | nvenc_session.nvEncUnmapInputResource(encoder, mapped_bitstream);
}
