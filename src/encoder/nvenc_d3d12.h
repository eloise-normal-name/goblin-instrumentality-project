#pragma once

#include <d3d12.h>
#include <nvenc/nvEncodeAPI.h>

#include <cstdint>
#include <vector>

#include "nvenc_session.h"

struct RegisteredTexture {
	ID3D12Resource* resource;
	NV_ENC_REGISTERED_PTR registered_ptr;
	NV_ENC_INPUT_PTR mapped_ptr;
	NV_ENC_BUFFER_FORMAT buffer_format;
	uint32_t width;
	uint32_t height;
	bool is_mapped;
};

struct BitstreamBuffer {
	ID3D12Resource* resource;
	NV_ENC_REGISTERED_PTR registered_ptr;
	uint32_t size;
};

class NvencD3D12 {
  public:
	NvencD3D12(NvencSession* session, uint32_t buffer_count);
	~NvencD3D12();

	void RegisterTexture(ID3D12Resource* texture, uint32_t width, uint32_t height,
						 NV_ENC_BUFFER_FORMAT format);
	void UnregisterTexture(uint32_t index);
	void UnregisterAllTextures();

	void RegisterBitstreamBuffer(ID3D12Resource* buffer, uint32_t size);
	void UnregisterBitstreamBuffer(uint32_t index);
	void UnregisterAllBitstreamBuffers();

	void MapInputTexture(uint32_t index);
	void UnmapInputTexture(uint32_t index);

	NvencSession* session = nullptr;
	std::vector<RegisteredTexture> textures;
	std::vector<BitstreamBuffer> bitstream_buffers;
};

NV_ENC_BUFFER_FORMAT DxgiFormatToNvencFormat(DXGI_FORMAT format);
