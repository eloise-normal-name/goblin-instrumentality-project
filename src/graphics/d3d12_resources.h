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
	explicit SharedTexture(ID3D12Device* device, const TextureDesc& desc);
	~SharedTexture();

	ComPtr<ID3D12Resource> resource;
	HANDLE shared_handle		= nullptr;
	uint32_t width				= 0;
	uint32_t height				= 0;
	DXGI_FORMAT format			= DXGI_FORMAT_UNKNOWN;
	ResourceState current_state = ResourceState::Common;
};

class ReadbackBuffer {
  public:
	explicit ReadbackBuffer(ID3D12Device* device, uint32_t size);
	~ReadbackBuffer() = default;

	ComPtr<ID3D12Resource> resource;
	uint32_t size = 0;
};

D3D12_RESOURCE_STATES ToD3D12State(ResourceState state);
D3D12_RESOURCE_BARRIER CreateTransitionBarrier(ID3D12Resource* resource,
											   D3D12_RESOURCE_STATES before,
											   D3D12_RESOURCE_STATES after);
