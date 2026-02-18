#include "frame_encoder.h"

#include "try.h"

FrameEncoder::FrameEncoder(NvencSession& sess, ID3D12Device* device, uint32_t count,
						   uint32_t output_buffer_size)
	: session(sess), buffer_count(count) {
	textures.reserve(count);

	output_d3d12_buffers.resize(count);
	output_registered_ptrs.resize(count);
	output_fences.resize(count);

	output_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!output_event)
		throw;

	void* encoder = session.encoder;

	D3D12_HEAP_PROPERTIES readback_heap{
		.Type = D3D12_HEAP_TYPE_READBACK,
	};

	D3D12_RESOURCE_DESC buffer_desc{
		.Dimension		  = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Width			  = output_buffer_size,
		.Height			  = 1,
		.DepthOrArraySize = 1,
		.MipLevels		  = 1,
		.Format			  = DXGI_FORMAT_UNKNOWN,
		.SampleDesc		  = {.Count = 1, .Quality = 0},
		.Layout			  = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
	};

	for (auto i = 0u; i < count; ++i) {
		Try | device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&output_fences[i]));

		Try
			| device->CreateCommittedResource(&readback_heap, D3D12_HEAP_FLAG_NONE, &buffer_desc,
											  D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
											  IID_PPV_ARGS(&output_d3d12_buffers[i]));

		NV_ENC_REGISTER_RESOURCE register_params{
			.version			= NV_ENC_REGISTER_RESOURCE_VER,
			.resourceType		= NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX,
			.width				= output_buffer_size,
			.height				= 1,
			.resourceToRegister = output_d3d12_buffers[i],
			.bufferFormat		= NV_ENC_BUFFER_FORMAT_U8,
			.bufferUsage		= NV_ENC_OUTPUT_BITSTREAM,
		};

		Try | session.nvEncRegisterResource(encoder, &register_params);
		output_registered_ptrs[i] = register_params.registeredResource;
	}
}

FrameEncoder::~FrameEncoder() {
	UnregisterAllTextures();
	UnregisterAllBitstreamBuffers();

	void* encoder = session.encoder;

	for (auto i = 0u; i < output_registered_ptrs.size(); ++i) {
		if (output_registered_ptrs[i])
			session.nvEncUnregisterResource(encoder, output_registered_ptrs[i]);
	}
	output_registered_ptrs.clear();

	for (auto buffer : output_d3d12_buffers) {
		if (buffer)
			buffer->Release();
	}
	output_d3d12_buffers.clear();

	for (auto fence : output_fences) {
		if (fence)
			fence->Release();
	}
	output_fences.clear();

	if (output_event)
		CloseHandle(output_event);
}

void FrameEncoder::RegisterTexture(ID3D12Resource* texture, uint32_t width, uint32_t height,
								   NV_ENC_BUFFER_FORMAT format, ID3D12Fence* fence) {
	void* encoder = session.encoder;

	NV_ENC_FENCE_POINT_D3D12 fence_point{
		.version   = NV_ENC_FENCE_POINT_D3D12_VER,
		.pFence	   = fence,
		.waitValue = 0,
		.bWait	   = 1,
	};

	NV_ENC_REGISTER_RESOURCE register_params{
		.version			= NV_ENC_REGISTER_RESOURCE_VER,
		.resourceType		= NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX,
		.width				= width,
		.height				= height,
		.resourceToRegister = texture,
		.bufferFormat		= format,
		.bufferUsage		= NV_ENC_INPUT_IMAGE,
		.pInputFencePoint	= &fence_point,
	};

	Try | session.nvEncRegisterResource(encoder, &register_params);

	textures.push_back({
		.resource		= texture,
		.registered_ptr = register_params.registeredResource,
		.buffer_format	= format,
		.width			= width,
		.height			= height,
		.fence			= fence,
	});
}

void FrameEncoder::UnregisterTexture(uint32_t index) {
	if (index >= textures.size())
		throw;

	RegisteredTexture& texture = textures[index];
	if (texture.is_mapped)
		UnmapInputTexture(index);

	if (texture.registered_ptr) {
		void* encoder = session.encoder;
		Try | session.nvEncUnregisterResource(encoder, texture.registered_ptr);
		texture.registered_ptr = nullptr;
	}
}

void FrameEncoder::UnregisterAllTextures() {
	for (uint32_t i = 0; i < textures.size(); ++i) {
		UnregisterTexture(i);
	}
	textures.clear();
}

void FrameEncoder::RegisterBitstreamBuffer(ID3D12Resource* buffer, uint32_t size) {
	if (!buffer)
		throw;

	void* encoder = session.encoder;

	NV_ENC_REGISTER_RESOURCE register_params{
		.version			= NV_ENC_REGISTER_RESOURCE_VER,
		.resourceType		= NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX,
		.width				= size,
		.height				= 1,
		.resourceToRegister = buffer,
		.bufferFormat		= NV_ENC_BUFFER_FORMAT_U8,
		.bufferUsage		= NV_ENC_OUTPUT_BITSTREAM,
	};

	Try | session.nvEncRegisterResource(encoder, &register_params);

	bitstream_buffers.push_back({
		.resource		= buffer,
		.registered_ptr = register_params.registeredResource,
		.size			= size,
	});
}

void FrameEncoder::UnregisterBitstreamBuffer(uint32_t index) {
	if (index >= bitstream_buffers.size())
		return;

	BitstreamBuffer& buffer = bitstream_buffers[index];
	if (buffer.registered_ptr) {
		void* encoder = session.encoder;
		Try | session.nvEncUnregisterResource(encoder, buffer.registered_ptr);
		buffer.registered_ptr = nullptr;
	}
}

void FrameEncoder::UnregisterAllBitstreamBuffers() {
	for (uint32_t i = 0; i < bitstream_buffers.size(); ++i) {
		UnregisterBitstreamBuffer(i);
	}
	bitstream_buffers.clear();
}

void FrameEncoder::MapInputTexture(uint32_t index, uint64_t) {
	if (index >= textures.size())
		throw;

	RegisteredTexture& texture = textures[index];
	if (texture.is_mapped)
		return;
	if (!texture.registered_ptr)
		throw;

	void* encoder = session.encoder;

	NV_ENC_MAP_INPUT_RESOURCE map_params{
		.version			= NV_ENC_MAP_INPUT_RESOURCE_VER,
		.registeredResource = texture.registered_ptr,
	};

	Try | session.nvEncMapInputResource(encoder, &map_params);

	texture.mapped_ptr	  = map_params.mappedResource;
	texture.buffer_format = map_params.mappedBufferFmt;
	texture.is_mapped	  = true;
}

void FrameEncoder::UnmapInputTexture(uint32_t index) {
	if (index >= textures.size())
		throw;

	RegisteredTexture& texture = textures[index];
	if (!texture.is_mapped)
		return;

	void* encoder = session.encoder;

	Try | session.nvEncUnmapInputResource(encoder, texture.mapped_ptr);

	texture.mapped_ptr = nullptr;
	texture.is_mapped  = false;
}

void FrameEncoder::EncodeFrame(uint32_t texture_index, uint64_t fence_wait_value,
							   uint32_t frame_index) {
	if (texture_index >= buffer_count)
		return;

	void* encoder = session.encoder;

	MapInputTexture(texture_index, fence_wait_value);

	RegisteredTexture& texture = textures[texture_index];

	NV_ENC_FENCE_POINT_D3D12 input_fence_point{
		.version   = NV_ENC_FENCE_POINT_D3D12_VER,
		.pFence	   = texture.fence,
		.waitValue = fence_wait_value,
		.bWait	   = 1,
	};

	NV_ENC_INPUT_RESOURCE_D3D12 input_resource{
		.version		 = NV_ENC_INPUT_RESOURCE_D3D12_VER,
		.pInputBuffer	 = texture.mapped_ptr,
		.inputFencePoint = input_fence_point,
	};

	NV_ENC_FENCE_POINT_D3D12 output_fence_point{
		.version	 = NV_ENC_FENCE_POINT_D3D12_VER,
		.pFence		 = output_fences[texture_index],
		.signalValue = frame_index + 1,
		.bSignal	 = 1,
	};

	NV_ENC_OUTPUT_RESOURCE_D3D12 output_resource{
		.version		  = NV_ENC_OUTPUT_RESOURCE_D3D12_VER,
		.pOutputBuffer	  = output_registered_ptrs[texture_index],
		.outputFencePoint = output_fence_point,
	};

	NV_ENC_PIC_PARAMS pic_params{
		.version		 = NV_ENC_PIC_PARAMS_VER,
		.inputWidth		 = texture.width,
		.inputHeight	 = texture.height,
		.inputPitch		 = texture.width * 4,
		.inputTimeStamp	 = frame_index,
		.inputBuffer	 = &input_resource,
		.outputBitstream = &output_resource,
		.bufferFmt		 = texture.buffer_format,
		.pictureStruct	 = NV_ENC_PIC_STRUCT_FRAME,
	};

	Try | session.nvEncEncodePicture(encoder, &pic_params);

	pending_outputs.push_back(
		PendingOutput{.output_resource = output_resource, .fence_value = frame_index + 1});
	++submitted_frames;

	UnmapInputTexture(texture_index);
}

void FrameEncoder::ProcessCompletedFrames(BitstreamFileWriter& writer, bool wait_for_all) {
	void* encoder = session.encoder;

	while (!pending_outputs.empty()) {
		auto& pending = pending_outputs.front();
		auto fence	  = (ID3D12Fence*)pending.output_resource.outputFencePoint.pFence;
		if (!fence)
			throw;

		auto completed_value = fence->GetCompletedValue();
		if (completed_value < pending.fence_value) {
			if (!wait_for_all)
				break;
			fence->SetEventOnCompletion(pending.fence_value, output_event);
			WaitForSingleObject(output_event, INFINITE);
			++wait_count;
		}

		NV_ENC_LOCK_BITSTREAM lock_params{
			.version		 = NV_ENC_LOCK_BITSTREAM_VER,
			.doNotWait		 = false,
			.outputBitstream = &pending.output_resource,
		};

		Try | session.nvEncLockBitstream(encoder, &lock_params);

		if (lock_params.bitstreamBufferPtr && lock_params.bitstreamSizeInBytes > 0)
			writer.WriteFrame(lock_params.bitstreamBufferPtr, lock_params.bitstreamSizeInBytes);

		Try | session.nvEncUnlockBitstream(encoder, &pending.output_resource);
		pending_outputs.pop_front();
		++completed_frames;
	}
}

FrameEncoder::Stats FrameEncoder::GetStats() const {
	return Stats{
		.submitted_frames = submitted_frames,
		.completed_frames = completed_frames,
		.pending_frames	  = pending_outputs.size(),
		.wait_count		  = wait_count,
	};
}

NV_ENC_BUFFER_FORMAT DxgiFormatToNvencFormat(DXGI_FORMAT format) {
	switch (format) {
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			return NV_ENC_BUFFER_FORMAT_ARGB;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return NV_ENC_BUFFER_FORMAT_ABGR;
		case DXGI_FORMAT_R10G10B10A2_UNORM:
			return NV_ENC_BUFFER_FORMAT_ABGR10;
		case DXGI_FORMAT_NV12:
			return NV_ENC_BUFFER_FORMAT_NV12;
		case DXGI_FORMAT_P010:
			return NV_ENC_BUFFER_FORMAT_YUV420_10BIT;
		default:
			return NV_ENC_BUFFER_FORMAT_ARGB;
	}
}
