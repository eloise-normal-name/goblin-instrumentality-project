#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <vector>

using Microsoft::WRL::ComPtr;

struct SwapChainConfig {
	uint32_t buffer_count;
	DXGI_FORMAT render_target_format;
};

class D3D12SwapChain {
  public:
	D3D12SwapChain(ID3D12Device* device, IDXGIFactory7* factory, ID3D12CommandQueue* command_queue,
				   HWND window_handle, const SwapChainConfig& config);
	~D3D12SwapChain() = default;

	ComPtr<IDXGISwapChain4> swap_chain;
	std::vector<ComPtr<ID3D12Resource>> render_targets;

  private:
	void CreateSwapChain(HWND window_handle);
	void AcquireBackBuffers();

	ID3D12Device* device;
	IDXGIFactory7* factory;
	ID3D12CommandQueue* command_queue;
	uint32_t buffer_count;
	DXGI_FORMAT render_target_format;
};
