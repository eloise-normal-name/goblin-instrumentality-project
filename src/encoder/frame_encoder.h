#pragma once

#include <d3d12.h>
#include <nvenc/nvEncodeAPI.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "bitstream_file_writer.h"
#include "nvenc_session.h"

struct RegisteredTexture {
	ID3D12Resource* resource;
	NV_ENC_REGISTERED_PTR registered_ptr;
	NV_ENC_INPUT_PTR mapped_ptr;
	NV_ENC_BUFFER_FORMAT buffer_format;
	uint32_t width;
	uint32_t height;
	bool is_mapped;
	ID3D12Fence* fence;
};

struct BitstreamBuffer {
	ID3D12Resource* resource;
	NV_ENC_REGISTERED_PTR registered_ptr;
	uint32_t size;
};

class FrameEncoder {
  public:
	struct Stats {
		uint64_t submitted_frames;
		uint64_t completed_frames;
		uint64_t pending_frames;
		uint64_t wait_count;
	};

	FrameEncoder(NvencSession& session, ID3D12Device* device, uint32_t buffer_count,
				 uint32_t output_buffer_size);
	~FrameEncoder();

	void RegisterTexture(ID3D12Resource* texture, uint32_t width, uint32_t height,
						 NV_ENC_BUFFER_FORMAT format, ID3D12Fence* fence);
	void UnregisterTexture(uint32_t index);
	void UnregisterAllTextures();

	void RegisterBitstreamBuffer(ID3D12Resource* buffer, uint32_t size);
	void UnregisterBitstreamBuffer(uint32_t index);
	void UnregisterAllBitstreamBuffers();

	void EncodeFrame(uint32_t texture_index, uint64_t fence_wait_value, uint32_t frame_index);
	void ProcessCompletedFrames(BitstreamFileWriter& writer, bool wait_for_all = false);
	Stats GetStats() const;

	bool HasPendingOutputs() const;
	HANDLE NextOutputEvent() const;

	std::vector<RegisteredTexture> textures;
	std::vector<BitstreamBuffer> bitstream_buffers;

  private:
	struct TextureEncodeCache {
		NV_ENC_FENCE_POINT_D3D12 input_fence_point;
		NV_ENC_INPUT_RESOURCE_D3D12 input_resource;
		NV_ENC_FENCE_POINT_D3D12 output_fence_point;
		NV_ENC_OUTPUT_RESOURCE_D3D12 output_resource;
		NV_ENC_PIC_PARAMS pic_params;
	};

	NvencSession& session;
	std::vector<ID3D12Resource*> output_d3d12_buffers;
	std::vector<NV_ENC_REGISTERED_PTR> output_registered_ptrs;
	std::vector<ID3D12Fence*> output_fences;
	std::vector<TextureEncodeCache> texture_encode_caches;
	uint32_t buffer_count;
	struct PendingOutput {
		NV_ENC_REGISTERED_PTR output_buffer;
		ID3D12Fence* output_fence;
		uint64_t fence_value;
		HANDLE event;
	};

	void MapInputTexture(uint32_t index);
	void UnmapInputTexture(uint32_t index);

	std::vector<PendingOutput> pending_ring;
	uint32_t pending_head	  = 0;
	uint32_t pending_count	  = 0;
	uint64_t submitted_frames = 0;
	uint64_t completed_frames = 0;
	uint64_t wait_count		  = 0;
};

NV_ENC_BUFFER_FORMAT DxgiFormatToNvencFormat(DXGI_FORMAT format);
