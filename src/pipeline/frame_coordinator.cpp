#include "frame_coordinator.h"

#include <windows.h>

#include "try.h"

FrameCoordinator::FrameCoordinator(D3D12Device& dev, const PipelineConfig& cfg)
	: device(dev), config(cfg) {
	uint32_t buffer_size = config.width * config.height * 4;

	NV_ENC_BUFFER_FORMAT nvenc_format = DxgiFormatToNvencFormat(encoder_texture.format);
	nvenc_d3d12.RegisterTexture(encoder_texture.resource.Get(), config.width, config.height,
								nvenc_format);

	nvenc_d3d12.RegisterBitstreamBuffer(output_buffer.resource.Get(), buffer_size);

	encode_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!encode_fence_event)
		throw;
}

FrameCoordinator::~FrameCoordinator() {
	device.WaitForGpu();

	if (encode_fence_event)
		CloseHandle(encode_fence_event);
}

void FrameCoordinator::BeginFrame() {
	current_frame_index = device.current_frame_index;
	commands.Reset();
}

void FrameCoordinator::EndFrame() {
	if (last_encode_fence_value > 0)
		Try | device.command_queue->Wait(device.fence.Get(), last_encode_fence_value);

	commands.TransitionResource(device.render_targets[device.current_frame_index].Get(),
								D3D12_RESOURCE_STATE_RENDER_TARGET,
								D3D12_RESOURCE_STATE_COPY_SOURCE);

	commands.TransitionTexture(encoder_texture, ResourceState::CopyDest);

	commands.CopyResource(encoder_texture.resource.Get(),
						  device.render_targets[device.current_frame_index].Get());

	commands.TransitionTexture(encoder_texture, ResourceState::Common);

	commands.TransitionResource(device.render_targets[device.current_frame_index].Get(),
								D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PRESENT);

	ExecuteCommandList(device.command_queue.Get(), commands);
}

void FrameCoordinator::EncodeFrame(FrameData& output) {
	uint64_t gpu_fence_value = device.SignalFenceForCurrentFrame();

	SubmitFrameToEncoder();

	uint64_t output_fence_value = gpu_fence_value + 1;
	last_encode_fence_value		= output_fence_value;
	device.SetFenceEvent(output_fence_value, encode_fence_event);
	WaitForSingleObject(encode_fence_event, INFINITE);

	RetrieveEncodedFrame(output);

	++frame_count;
}

void FrameCoordinator::SubmitFrameToEncoder() {
	nvenc_d3d12.MapInputTexture(0);

	if (nvenc_d3d12.textures.empty() || nvenc_d3d12.bitstream_buffers.empty())
		throw;

	const RegisteredTexture& texture = nvenc_d3d12.textures[0];
	const BitstreamBuffer& bitstream = nvenc_d3d12.bitstream_buffers[0];

	void* encoder = nvenc_session.encoder;

	NV_ENC_MAP_INPUT_RESOURCE map_params{
		.version			= NV_ENC_MAP_INPUT_RESOURCE_VER,
		.registeredResource = bitstream.registered_ptr,
	};
	Try | nvenc_session.nvEncMapInputResource(encoder, &map_params);
	void* mapped_bitstream = map_params.mappedResource;

	NV_ENC_INPUT_RESOURCE_D3D12 input_resource{
		.version		 = NV_ENC_INPUT_RESOURCE_D3D12_VER,
		.pInputBuffer	 = texture.mapped_ptr,
		.inputFencePoint = {.version	 = NV_ENC_FENCE_POINT_D3D12_VER,
							.pFence		 = device.fence.Get(),
							.signalValue = device.fence_values[device.current_frame_index]},
	};

	NV_ENC_OUTPUT_RESOURCE_D3D12 output_resource{
		.version	   = NV_ENC_OUTPUT_RESOURCE_D3D12_VER,
		.pOutputBuffer = mapped_bitstream,
	};
	output_resource.outputFencePoint.version  = NV_ENC_FENCE_POINT_D3D12_VER;
	output_resource.outputFencePoint.reserved = 0;
	output_resource.outputFencePoint.pFence	  = device.fence.Get();
	output_resource.outputFencePoint.signalValue
		= device.fence_values[device.current_frame_index] + 1;
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

	nvenc_d3d12.UnmapInputTexture(0);
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
		.version		  = NV_ENC_OUTPUT_RESOURCE_D3D12_VER,
		.pOutputBuffer	  = mapped_bitstream,
		.outputFencePoint = {.version	  = NV_ENC_FENCE_POINT_D3D12_VER,
							 .pFence	  = device.fence.Get(),
							 .signalValue = device.fence_values[device.current_frame_index] + 1,
							 .bSignal	  = 1},
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
