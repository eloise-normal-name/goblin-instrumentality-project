#include "frame_encoder.h"

#include "try.h"

FrameEncoder::FrameEncoder(NvencSession& sess, NvencD3D12& nvenc, ID3D12Device* device,
						   uint32_t count, uint32_t output_buffer_size, const char* output_path)
	: session(sess), nvenc_d3d12(nvenc), file_writer(output_path), buffer_count(count) {

	output_buffers.resize(count);

	void* encoder = session.encoder;
	for (auto i = 0u; i < count; ++i) {
		NV_ENC_CREATE_BITSTREAM_BUFFER create_params{
			.version = NV_ENC_CREATE_BITSTREAM_BUFFER_VER,
		};

		Try | session.nvEncCreateBitstreamBuffer(encoder, &create_params);
		output_buffers[i] = create_params.bitstreamBuffer;
	}
}

FrameEncoder::~FrameEncoder() {
	void* encoder = session.encoder;
	for (auto buffer : output_buffers) {
		if (buffer)
			session.nvEncDestroyBitstreamBuffer(encoder, buffer);
	}
	output_buffers.clear();
}

void FrameEncoder::EncodeFrame(uint32_t texture_index, uint64_t fence_wait_value,
							   uint32_t frame_index) {
	if (texture_index >= nvenc_d3d12.textures.size())
		return;
	if (texture_index >= output_buffers.size())
		return;

	void* encoder = session.encoder;

	nvenc_d3d12.MapInputTexture(texture_index, fence_wait_value);

	RegisteredTexture& texture = nvenc_d3d12.textures[texture_index];

	NV_ENC_FENCE_POINT_D3D12 input_fence_point{
		.version   = NV_ENC_FENCE_POINT_D3D12_VER,
		.pFence	   = texture.fence,
		.waitValue = fence_wait_value,
		.bWait	   = 1,
	};

	NV_ENC_INPUT_RESOURCE_D3D12 input_resource{
		.version		  = NV_ENC_INPUT_RESOURCE_D3D12_VER,
		.pInputBuffer	  = texture.mapped_ptr,
		.inputFencePoint  = input_fence_point,
	};

	NV_ENC_FENCE_POINT_D3D12 output_fence_point{
		.version   = NV_ENC_FENCE_POINT_D3D12_VER,
		.pFence	   = nullptr,
		.waitValue = 0,
		.bWait	   = 0,
	};

	NV_ENC_OUTPUT_RESOURCE_D3D12 output_resource{
		.version		   = NV_ENC_OUTPUT_RESOURCE_D3D12_VER,
		.pOutputBuffer	   = output_buffers[texture_index],
		.outputFencePoint  = output_fence_point,
	};

	NV_ENC_PIC_PARAMS pic_params{
		.version		 = NV_ENC_PIC_PARAMS_VER,
		.inputWidth		 = texture.width,
		.inputHeight	 = texture.height,
		.inputPitch		 = texture.width * 4,
		.inputBuffer	 = &input_resource,
		.outputBitstream = &output_resource,
		.bufferFmt		 = texture.buffer_format,
		.pictureStruct	 = NV_ENC_PIC_STRUCT_FRAME,
		.inputTimeStamp	 = frame_index,
	};

	Try | session.nvEncEncodePicture(encoder, &pic_params);

	NV_ENC_LOCK_BITSTREAM lock_params{
		.version		   = NV_ENC_LOCK_BITSTREAM_VER,
		.outputBitstream   = &output_resource,
		.doNotWait		   = false,
	};

	Try | session.nvEncLockBitstream(encoder, &lock_params);

	if (lock_params.bitstreamBufferPtr && lock_params.bitstreamSizeInBytes > 0)
		file_writer.WriteFrame(lock_params.bitstreamBufferPtr, lock_params.bitstreamSizeInBytes);

	Try | session.nvEncUnlockBitstream(encoder, output_buffers[texture_index]);

	nvenc_d3d12.UnmapInputTexture(texture_index);
}
