#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "try.h"

struct RenderTextureArray {
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> textures;

	RenderTextureArray(ID3D12Device* device, uint32_t count, uint32_t width, uint32_t height,
					   DXGI_FORMAT format) {
		D3D12_HEAP_PROPERTIES default_heap_props{
			.Type = D3D12_HEAP_TYPE_DEFAULT,
		};

		D3D12_RESOURCE_DESC texture_desc{
			.Dimension		  = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Width			  = width,
			.Height			  = height,
			.DepthOrArraySize = 1,
			.MipLevels		  = 1,
			.Format			  = format,
			.SampleDesc		  = {.Count = 1},
			.Flags			  = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
		};

		D3D12_CLEAR_VALUE clear_value{
			.Format = format,
			.Color	= {0.0f, 0.0f, 0.0f, 1.0f},
		};
		for (auto i = 0u; i < count; ++i)
			Try
				| device->CreateCommittedResource(&default_heap_props, D3D12_HEAP_FLAG_NONE,
												  &texture_desc, D3D12_RESOURCE_STATE_COMMON,
												  &clear_value,
												  IID_PPV_ARGS(&textures.emplace_back()));
	}
};

class D3D12FrameResources {
  public:
	D3D12FrameResources(ID3D12Device* device, uint32_t count);
	~D3D12FrameResources();

	std::vector<ID3D12GraphicsCommandList*> command_lists;
	std::vector<ID3D12Fence*> fences;
	std::vector<HANDLE> fence_events;
};
