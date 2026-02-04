#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include <cstdint>

using Microsoft::WRL::ComPtr;

enum class ResourceState { Common, RenderTarget, CopySource, CopyDest, VideoEncodeRead };

struct TextureDesc {
	uint32_t width;
	uint32_t height;
	DXGI_FORMAT format;
	bool allow_render_target;
	bool allow_simultaneous_access;
};

class SharedTexture {
  public:
	SharedTexture() = default;
	~SharedTexture() = default;

	SharedTexture(const SharedTexture&) = delete;
	SharedTexture& operator=(const SharedTexture&) = delete;
	SharedTexture(SharedTexture&&) = default;
	SharedTexture& operator=(SharedTexture&&) = default;

	bool Create(ID3D12Device* device, const TextureDesc& desc);
	void Reset();

	ComPtr<ID3D12Resource> resource;
	HANDLE shared_handle = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	ResourceState current_state = ResourceState::Common;
};

class ReadbackBuffer {
  public:
	ReadbackBuffer() = default;
	~ReadbackBuffer() = default;

	ReadbackBuffer(const ReadbackBuffer&) = delete;
	ReadbackBuffer& operator=(const ReadbackBuffer&) = delete;
	ReadbackBuffer(ReadbackBuffer&&) = default;
	ReadbackBuffer& operator=(ReadbackBuffer&&) = default;

	bool Create(ID3D12Device* device, uint32_t size);
	void Reset();

	ComPtr<ID3D12Resource> resource;
	uint32_t size = 0;
};

D3D12_RESOURCE_STATES ToD3D12State(ResourceState state);
D3D12_RESOURCE_BARRIER CreateTransitionBarrier(ID3D12Resource* resource,
											   D3D12_RESOURCE_STATES before,
											   D3D12_RESOURCE_STATES after);
