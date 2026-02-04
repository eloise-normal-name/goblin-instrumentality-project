#include "d3d12_device.h"

#include "try.h"

D3D12Device::~D3D12Device() {
	Shutdown();
}

void D3D12Device::Initialize(const DeviceConfig& cfg) {
	buffer_count = cfg.buffer_count > 0 ? cfg.buffer_count : 2;
	if (buffer_count > MAX_BUFFER_COUNT) {
		buffer_count = MAX_BUFFER_COUNT;
	}

	render_target_format = cfg.render_target_format;

	CreateDevice();
	CreateCommandQueue();
	CreateSwapChain(cfg.window_handle, cfg.frame_width, cfg.frame_height);
	CreateDescriptorHeaps();
	CreateRenderTargets();
	CreateCommandAllocators();
	CreateFence();
}

void D3D12Device::Shutdown() {
	WaitForGpu();

	if (fence_event) {
		CloseHandle(fence_event);
		fence_event = nullptr;
	}

	for (uint32_t i = 0; i < buffer_count; ++i) {
		command_allocators[i].Reset();
		render_targets[i].Reset();
	}

	fence.Reset();
	rtv_heap.Reset();
	swap_chain.Reset();
	command_queue.Reset();
	device.Reset();
	adapter.Reset();
	factory.Reset();
}

void D3D12Device::WaitForGpu() {
	if (!command_queue || !fence || !fence_event)
		return;

	uint64_t fence_value_to_signal = fence_values[current_frame_index] + 1;
	Try | command_queue->Signal(fence.Get(), fence_value_to_signal);

	if (fence->GetCompletedValue() < fence_value_to_signal) {
		Try | fence->SetEventOnCompletion(fence_value_to_signal, fence_event);
		WaitForSingleObject(fence_event, INFINITE);
	}

	fence_values[current_frame_index] = fence_value_to_signal;
}

void D3D12Device::MoveToNextFrame() {
	uint64_t current_fence_value = fence_values[current_frame_index];
	Try | command_queue->Signal(fence.Get(), current_fence_value);

	current_frame_index = swap_chain->GetCurrentBackBufferIndex();

	if (fence->GetCompletedValue() < fence_values[current_frame_index]) {
		Try | fence->SetEventOnCompletion(fence_values[current_frame_index], fence_event);
		WaitForSingleObject(fence_event, INFINITE);
	}

	fence_values[current_frame_index] = current_fence_value + 1;
}

void D3D12Device::CreateDevice() {
	auto dxgi_factory_flags = 0u;

#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debug_controller;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)))) {
		debug_controller->EnableDebugLayer();
		dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif
	ComPtr<IDXGIAdapter1> adapter;
	DXGI_ADAPTER_DESC1 desc;
	Try | CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&factory))
		| factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
											  IID_PPV_ARGS(&adapter))
		| D3D12CreateDevice(*&adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device))
		| adapter->GetDesc1(&desc);

	if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		throw;
}

void D3D12Device::CreateCommandQueue() {
	D3D12_COMMAND_QUEUE_DESC queue_desc{
		.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
	};

	Try | device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue));
}

void D3D12Device::CreateSwapChain(HWND window_handle, uint32_t width, uint32_t height) {
	DXGI_SWAP_CHAIN_DESC1 sc_desc{
		.Width = width,
		.Height = height,
		.Format = render_target_format,
		.Stereo = FALSE,
		.SampleDesc = {.Count = 1, .Quality = 0},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = buffer_count,
		.Scaling = DXGI_SCALING_STRETCH,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
		.Flags = 0,
	};

	ComPtr<IDXGISwapChain1> sc1;
	Try
		| factory->CreateSwapChainForHwnd(command_queue.Get(), window_handle, &sc_desc, nullptr,
										  nullptr, &sc1)
		| factory->MakeWindowAssociation(window_handle, DXGI_MWA_NO_ALT_ENTER)
		| sc1.As(&swap_chain);

	current_frame_index = swap_chain->GetCurrentBackBufferIndex();
}

void D3D12Device::CreateDescriptorHeaps() {
	D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
	rtv_heap_desc.NumDescriptors = buffer_count;
	rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	Try | device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_heap));

	rtv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void D3D12Device::CreateRenderTargets() {
	D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = rtv_heap->GetCPUDescriptorHandleForHeapStart();

	for (uint32_t i = 0; i < buffer_count; ++i) {
		Try | swap_chain->GetBuffer(i, IID_PPV_ARGS(&render_targets[i]));

		device->CreateRenderTargetView(render_targets[i].Get(), nullptr, rtv_handle);
		rtv_handle.ptr += rtv_descriptor_size;
	}
}

void D3D12Device::CreateCommandAllocators() {
	for (uint32_t i = 0; i < buffer_count; ++i) {
		Try
			| device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
											 IID_PPV_ARGS(&command_allocators[i]));
	}
}

void D3D12Device::CreateFence() {
	Try | device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!fence_event) {
		throw;
	}

	for (uint32_t i = 0; i < buffer_count; ++i) {
		fence_values[i] = 1;
	}
}
