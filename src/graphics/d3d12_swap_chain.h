#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <cstdint>

#include "render_targets.h"

using Microsoft::WRL::ComPtr;

struct SwapChainConfig {
	HWND window_handle;
	uint32_t frame_width;
	uint32_t frame_height;
	uint32_t buffer_count = 3;
	DXGI_FORMAT render_target_format;
};

class D3D12SwapChain {
  public:
	explicit D3D12SwapChain(ID3D12Device* device, IDXGIFactory7* factory,
							ID3D12CommandQueue* command_queue, const SwapChainConfig& config);
	~D3D12SwapChain() = default;

	void Present(uint32_t sync_interval, uint32_t flags);
	void UpdateCurrentFrameIndex();
	RenderTargets& GetRenderTargets();
	const RenderTargets& GetRenderTargets() const;

	ComPtr<IDXGISwapChain4> swap_chain;

  private:
	void create_swap_chain(HWND window_handle, uint32_t width, uint32_t height);
	void create_descriptor_heaps();
	void create_render_targets();

	ID3D12Device* device			  = nullptr;
	IDXGIFactory7* factory			  = nullptr;
	ID3D12CommandQueue* command_queue = nullptr;
	RenderTargets render_targets;
	uint32_t buffer_count			 = 2;
	DXGI_FORMAT render_target_format = DXGI_FORMAT_B8G8R8A8_UNORM;
};
