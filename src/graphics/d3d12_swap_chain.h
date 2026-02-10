#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <vector>

using Microsoft::WRL::ComPtr;

struct SwapChainConfig {
	uint32_t frame_width;
	uint32_t frame_height;
	uint32_t buffer_count;
	DXGI_FORMAT render_target_format;
};

class D3D12SwapChain {
  public:
	explicit D3D12SwapChain(ID3D12Device* device, IDXGIFactory7* factory,
							ID3D12CommandQueue* command_queue, HWND window_handle,
							const SwapChainConfig& config);
	~D3D12SwapChain() = default;

	HRESULT Present(uint32_t sync_interval, uint32_t flags);

	ComPtr<IDXGISwapChain4> swap_chain;
	ComPtr<ID3D12DescriptorHeap> rtv_heap;
	std::vector<ComPtr<ID3D12Resource>> render_targets;
	uint32_t rtv_descriptor_size = 0;

  private:
	uint32_t buffer_count;
	DXGI_FORMAT render_target_format;
};
