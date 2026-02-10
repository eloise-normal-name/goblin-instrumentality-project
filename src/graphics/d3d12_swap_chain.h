#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <vector>

using Microsoft::WRL::ComPtr;

struct SwapChainConfig {
	HWND window_handle;
	uint32_t frame_width;
	uint32_t frame_height;
	uint32_t buffer_count;
	DXGI_FORMAT render_target_format;
};

class D3D12SwapChain {
  public:
	explicit D3D12SwapChain(ID3D12Device* device, IDXGIFactory7* factory,
							ID3D12CommandQueue* command_queue, const SwapChainConfig& config);
	~D3D12SwapChain() = default;

	void Present(uint32_t sync_interval, uint32_t flags);

	ComPtr<IDXGISwapChain4> swap_chain;
	ComPtr<ID3D12DescriptorHeap> rtv_heap;
	std::vector<ComPtr<ID3D12Resource>> render_targets;
	uint32_t rtv_descriptor_size = 0;

  private:
	void create_swap_chain(HWND window_handle, uint32_t width, uint32_t height);
	void create_descriptor_heaps();
	void create_render_targets();

	ID3D12Device* device;
	IDXGIFactory7* factory;
	ID3D12CommandQueue* command_queue;
	uint32_t buffer_count;
	DXGI_FORMAT render_target_format;
};
