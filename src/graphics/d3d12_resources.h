#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include <cstdint>

using Microsoft::WRL::ComPtr;

enum class ResourceState { Common, RenderTarget, CopySource, CopyDest, VideoEncodeRead };

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
