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

	ID3D12Resource* GetResource() const {
		return resource.Get();
	}
	HANDLE GetSharedHandle() const {
		return shared_handle;
	}
	uint32_t GetWidth() const {
		return width;
	}
	uint32_t GetHeight() const {
		return height;
	}
	DXGI_FORMAT GetFormat() const {
		return format;
	}
	ResourceState GetCurrentState() const {
		return current_state;
	}
	void SetCurrentState(ResourceState state) {
		current_state = state;
	}

  private:
	ComPtr<ID3D12Resource> resource;
	HANDLE shared_handle = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	ResourceState current_state = ResourceState::Common;
};

D3D12_RESOURCE_STATES ToD3D12State(ResourceState state);
D3D12_RESOURCE_BARRIER CreateTransitionBarrier(ID3D12Resource* resource,
											   D3D12_RESOURCE_STATES before,
											   D3D12_RESOURCE_STATES after);
