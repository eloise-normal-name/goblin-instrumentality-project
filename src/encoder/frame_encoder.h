#pragma once

#include <d3d12.h>
#include <nvenc/nvEncodeAPI.h>

#include <cstdint>
#include <deque>
#include <vector>

#include "bitstream_file_writer.h"
#include "nvenc_d3d12.h"
#include "nvenc_session.h"

class FrameEncoder {
  public:
	struct Stats {
		uint64_t submitted_frames;
		uint64_t completed_frames;
		uint64_t pending_frames;
		uint64_t wait_count;
	};

	FrameEncoder(NvencSession& session, NvencD3D12& nvenc_d3d12, ID3D12Device* device,
				 uint32_t buffer_count, uint32_t output_buffer_size);
	~FrameEncoder();

	void EncodeFrame(uint32_t texture_index, uint64_t fence_wait_value, uint32_t frame_index);
	void ProcessCompletedFrames(BitstreamFileWriter& writer, bool wait_for_all = false);
	Stats GetStats() const;

  private:
	NvencSession& session;
	NvencD3D12& nvenc_d3d12;
	std::vector<ID3D12Resource*> output_d3d12_buffers;
	std::vector<NV_ENC_REGISTERED_PTR> output_registered_ptrs;
	std::vector<ID3D12Fence*> output_fences;
	uint32_t buffer_count;
	HANDLE output_event = nullptr;

	struct PendingOutput {
		NV_ENC_OUTPUT_RESOURCE_D3D12 output_resource;
		uint64_t fence_value;
	};

	std::deque<PendingOutput> pending_outputs;
	uint64_t submitted_frames = 0;
	uint64_t completed_frames = 0;
	uint64_t wait_count		  = 0;
};
