#pragma once

#include <d3d12.h>
#include <nvenc/nvEncodeAPI.h>

#include <cstdint>
#include <vector>

#include "bitstream_file_writer.h"
#include "nvenc_d3d12.h"
#include "nvenc_session.h"

class FrameEncoder {
  public:
	FrameEncoder(NvencSession& session, NvencD3D12& nvenc_d3d12, ID3D12Device* device,
				 uint32_t buffer_count, uint32_t output_buffer_size, const char* output_path);
	~FrameEncoder();

	void EncodeFrame(uint32_t texture_index, uint64_t fence_wait_value, uint32_t frame_index);

  private:
	NvencSession& session;
	NvencD3D12& nvenc_d3d12;
	BitstreamFileWriter file_writer;
	std::vector<ID3D12Resource*> output_d3d12_buffers;
	std::vector<NV_ENC_REGISTERED_PTR> output_registered_ptrs;
	std::vector<ID3D12Fence*> output_fences;
	uint32_t buffer_count;
};
