#pragma once

#include <d3d12.h>

#include <cstdint>
#include <vector>

class D3D12FrameResources {
  public:
	D3D12FrameResources(ID3D12Device* device, uint32_t count, uint32_t width, uint32_t height,
						 DXGI_FORMAT format);
	~D3D12FrameResources();

	std::vector<ID3D12GraphicsCommandList*> command_lists;
	std::vector<ID3D12Fence*> fences;
	std::vector<HANDLE> fence_events;
	std::vector<ID3D12Resource*> offscreen_render_targets;
};
