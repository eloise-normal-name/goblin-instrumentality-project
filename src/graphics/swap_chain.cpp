#include "swap_chain.h"

#include <windows.h>

#include "try.h"

D3D12SwapChain::D3D12SwapChain(ID3D12Device* dev, IDXGIFactory7* fac, ID3D12CommandQueue* queue,
							   HWND window_handle, const SwapChainConfig& config)
	: device(dev)
	, factory(fac)
	, command_queue(queue)
	, buffer_count(config.buffer_count)
	, render_target_format(config.render_target_format) {
	render_targets.resize(buffer_count);
	CreateSwapChain(window_handle);
	AcquireBackBuffers();
}

void D3D12SwapChain::CreateSwapChain(HWND window_handle) {
	RECT client_rect;
	if (!GetClientRect(window_handle, &client_rect))
		throw;

	auto width	= (uint32_t)(client_rect.right - client_rect.left);
	auto height = (uint32_t)(client_rect.bottom - client_rect.top);

	DXGI_SWAP_CHAIN_DESC1 sc_desc{
		.Width		 = width,
		.Height		 = height,
		.Format		 = render_target_format,
		.SampleDesc	 = {.Count = 1, .Quality = 0},
		.BufferUsage = DXGI_USAGE_BACK_BUFFER,
		.BufferCount = buffer_count,
		.SwapEffect	 = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.Flags		 = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT,
	};

	ComPtr<IDXGISwapChain1> sc1;
	Try
		| factory->CreateSwapChainForHwnd(command_queue, window_handle, &sc_desc, nullptr, nullptr,
										  &sc1)
		| factory->MakeWindowAssociation(window_handle, DXGI_MWA_NO_ALT_ENTER)
		| sc1->QueryInterface(IID_PPV_ARGS(&swap_chain))
		| swap_chain->SetMaximumFrameLatency(buffer_count);

	frame_latency_waitable = swap_chain->GetFrameLatencyWaitableObject();
	if (!frame_latency_waitable)
		throw;
}

D3D12SwapChain::~D3D12SwapChain() {
	CloseHandle(frame_latency_waitable);
}

void D3D12SwapChain::AcquireBackBuffers() {
	for (uint32_t i = 0; i < buffer_count; ++i) {
		Try | swap_chain->GetBuffer(i, IID_PPV_ARGS(&render_targets[i]));
	}
}

