#include "d3d12_device.h"

#include "try.h"

D3D12Device::D3D12Device(const DeviceConfig& config)
	: buffer_count(config.buffer_count > 0 ? config.buffer_count : 2)
	, render_target_format(config.render_target_format) {
	if (buffer_count > 3)
		buffer_count = 3;

	create_device();
	create_command_queue();
	create_swap_chain(config.window_handle, config.frame_width, config.frame_height);
	create_descriptor_heaps();
	create_render_targets();
	create_command_allocators();
	create_fence();
}

D3D12Device::~D3D12Device() {
	WaitForGpu();

	if (fence_event)
		CloseHandle(fence_event);
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

uint64_t D3D12Device::SignalFenceForCurrentFrame() {
	uint32_t index = current_frame_index;
	uint64_t fence_value_to_signal = fence_values[index] + 1;
	Try | command_queue->Signal(fence.Get(), fence_value_to_signal);
	fence_values[index] = fence_value_to_signal;
	return fence_value_to_signal;
}

void D3D12Device::SetFenceEvent(uint64_t value, HANDLE event) {
	if (!fence || !event)
		return;
	if (fence->GetCompletedValue() < value)
		Try | fence->SetEventOnCompletion(value, event);
	else
		SetEvent(event);
}

void D3D12Device::create_device() {
	auto dxgi_factory_flags = 0u;

#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debug_controller;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)))) {
		debug_controller->EnableDebugLayer();
		dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif
	ComPtr<IDXGIAdapter1> selected_adapter;
	DXGI_ADAPTER_DESC1 desc;
	Try | CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&factory))
		| factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
											  IID_PPV_ARGS(&selected_adapter))
		| D3D12CreateDevice(*&selected_adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device))
		| selected_adapter->GetDesc1(&desc);

	if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		throw;
}

void D3D12Device::create_command_queue() {
	D3D12_COMMAND_QUEUE_DESC queue_desc{
		.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
	};

	Try | device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue));
}

void D3D12Device::create_swap_chain(HWND window_handle, uint32_t width, uint32_t height) {
	DXGI_SWAP_CHAIN_DESC1 sc_desc{
		.Width = width,
		.Height = height,
		.Format = render_target_format,
		.SampleDesc = {.Count = 1, .Quality = 0},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = buffer_count,
		.Scaling = DXGI_SCALING_STRETCH,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
	};

	ComPtr<IDXGISwapChain1> sc1;
	Try
		| factory->CreateSwapChainForHwnd(command_queue.Get(), window_handle, &sc_desc, nullptr,
										  nullptr, &sc1)
		| factory->MakeWindowAssociation(window_handle, DXGI_MWA_NO_ALT_ENTER)
		| sc1.As(&swap_chain);

	current_frame_index = swap_chain->GetCurrentBackBufferIndex();
}

void D3D12Device::create_descriptor_heaps() {
	D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = buffer_count,
	};

	Try | device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_heap));

	rtv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void D3D12Device::create_render_targets() {
	D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = rtv_heap->GetCPUDescriptorHandleForHeapStart();

	for (uint32_t i = 0; i < buffer_count; ++i) {
		Try | swap_chain->GetBuffer(i, IID_PPV_ARGS(&render_targets[i]));

		device->CreateRenderTargetView(render_targets[i].Get(), nullptr, rtv_handle);
		rtv_handle.ptr += rtv_descriptor_size;
	}
}

void D3D12Device::create_command_allocators() {
	for (uint32_t i = 0; i < buffer_count; ++i) {
		Try
			| device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
											 IID_PPV_ARGS(&command_allocators[i]));
	}
}

void D3D12Device::create_fence() {
	Try | device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!fence_event)
		throw;

	for (uint32_t i = 0; i < buffer_count; ++i) {
		fence_values[i] = 1;
	}
}
