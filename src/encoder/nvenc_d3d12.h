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
  NV_ENC_OUTPUT_PTR output_ptr;
  uint32_t size;
};

class NvencD3D12 {
public:
  NvencD3D12() = default;
  ~NvencD3D12();

  NvencD3D12(const NvencD3D12&) = delete;
  NvencD3D12& operator=(const NvencD3D12&) = delete;
  NvencD3D12(NvencD3D12&&) = delete;
  NvencD3D12& operator=(NvencD3D12&&) = delete;

  bool Initialize(NvencSession* init_session, uint32_t buffer_count);
  void Shutdown();

  bool RegisterTexture(ID3D12Resource* texture, uint32_t width, uint32_t height,
                       NV_ENC_BUFFER_FORMAT format);
  bool UnregisterTexture(uint32_t index);
  void UnregisterAllTextures();

  bool CreateBitstreamBuffers(uint32_t count, uint32_t size);
  void DestroyBitstreamBuffers();

  bool MapInputTexture(uint32_t index);
  bool UnmapInputTexture(uint32_t index);

  RegisteredTexture* GetRegisteredTexture(uint32_t index);
  BitstreamBuffer* GetBitstreamBuffer(uint32_t index);
  uint32_t GetTextureCount() const {
    return static_cast<uint32_t>(textures.size());
  }
  uint32_t GetBitstreamBufferCount() const {
    return static_cast<uint32_t>(bitstream_buffers.size());
  }

private:
  NvencSession* session = nullptr;
  std::vector<RegisteredTexture> textures;
  std::vector<BitstreamBuffer> bitstream_buffers;
};

NV_ENC_BUFFER_FORMAT DxgiFormatToNvencFormat(DXGI_FORMAT format);
