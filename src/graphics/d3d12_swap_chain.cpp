#include "d3d12_swap_chain.h"

#include "try.h"

D3D12SwapChain::D3D12SwapChain(ID3D12Device* dev, IDXGIFactory7* fac, ID3D12CommandQueue* queue,
							   HWND window_handle, const SwapChainConfig& config)
	: device(dev)
	, factory(fac)
	, command_queue(queue)
	, buffer_count(config.buffer_count)
	, render_target_format(config.render_target_format)
	, rtv_descriptor_size(0) {
	render_targets.resize(buffer_count);
	this->create_swap_chain(window_handle, config.frame_width, config.frame_height);
	this->create_descriptor_heaps();
	this->create_render_targets();
}

HRESULT D3D12SwapChain::Present(uint32_t sync_interval, uint32_t flags) {
	return swap_chain->Present(sync_interval, flags);
}

void D3D12SwapChain::create_swap_chain(HWND window_handle, uint32_t width, uint32_t height) {
	DXGI_SWAP_CHAIN_DESC1 sc_desc{
		.Width		 = width,
		.Height		 = height,
		.Format		 = render_target_format,
		.SampleDesc	 = {.Count = 1, .Quality = 0},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = buffer_count,
		.SwapEffect	 = DXGI_SWAP_EFFECT_FLIP_DISCARD,
	};

	ComPtr<IDXGISwapChain1> sc1;
	Try
		| factory->CreateSwapChainForHwnd(command_queue, window_handle, &sc_desc, nullptr, nullptr,
										  sc1.GetAddressOf())
		| factory->MakeWindowAssociation(window_handle, DXGI_MWA_NO_ALT_ENTER);
	Try | sc1->QueryInterface(IID_PPV_ARGS(swap_chain.GetAddressOf()));
}

void D3D12SwapChain::create_descriptor_heaps() {
	if (!device)
		throw;

	D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc{
		.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = buffer_count,
	};

	Try | device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_heap));

	rtv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void D3D12SwapChain::create_render_targets() {
	D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = rtv_heap->GetCPUDescriptorHandleForHeapStart();
	for (uint32_t i = 0; i < buffer_count; ++i) {
		Try | swap_chain->GetBuffer(i, IID_PPV_ARGS(render_targets[i].GetAddressOf()));
		device->CreateRenderTargetView(render_targets[i].Get(), nullptr, rtv_handle);
		rtv_handle.ptr += rtv_descriptor_size;
	}
}
