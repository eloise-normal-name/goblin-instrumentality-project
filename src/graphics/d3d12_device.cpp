#include "d3d12_device.h"

D3D12Device::~D3D12Device() {
	Shutdown();
}

bool D3D12Device::Initialize(const DeviceConfig& cfg) {
	buffer_count = cfg.buffer_count > 0 ? cfg.buffer_count : 2;
	if (buffer_count > MAX_BUFFER_COUNT) {
		buffer_count = MAX_BUFFER_COUNT;
	}

	render_target_format = cfg.render_target_format;

	if (!CreateDevice())
		return false;
	if (!CreateCommandQueue())
		return false;
	if (!CreateSwapChain(cfg.window_handle, cfg.frame_width, cfg.frame_height))
		return false;
	if (!CreateDescriptorHeaps())
		return false;
	if (!CreateRenderTargets())
		return false;
	if (!CreateCommandAllocators())
		return false;
	if (!CreateFence())
		return false;

	return true;
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
	command_queue->Signal(fence.Get(), fence_value_to_signal);

	if (fence->GetCompletedValue() < fence_value_to_signal) {
		fence->SetEventOnCompletion(fence_value_to_signal, fence_event);
		WaitForSingleObject(fence_event, INFINITE);
	}

	fence_values[current_frame_index] = fence_value_to_signal;
}

void D3D12Device::MoveToNextFrame() {
	uint64_t current_fence_value = fence_values[current_frame_index];
	command_queue->Signal(fence.Get(), current_fence_value);

	current_frame_index = swap_chain->GetCurrentBackBufferIndex();

	if (fence->GetCompletedValue() < fence_values[current_frame_index]) {
		fence->SetEventOnCompletion(fence_values[current_frame_index], fence_event);
		WaitForSingleObject(fence_event, INFINITE);
	}

	fence_values[current_frame_index] = current_fence_value + 1;
}

ID3D12CommandAllocator* D3D12Device::GetCurrentCommandAllocator() const {
	return command_allocators[current_frame_index].Get();
}

ID3D12Resource* D3D12Device::GetCurrentRenderTarget() const {
	return render_targets[current_frame_index].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Device::GetCurrentRenderTargetView() const {
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtv_heap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(current_frame_index * rtv_descriptor_size);
	return handle;
}

bool D3D12Device::CreateDevice() {
	UINT dxgi_factory_flags = 0;

#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debug_controller;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)))) {
		debug_controller->EnableDebugLayer();
		dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	if (FAILED(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&factory)))) {
		return false;
	}

	ComPtr<IDXGIAdapter1> adapter1;
	for (UINT i = 0;
		 factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
											 IID_PPV_ARGS(&adapter1)) != DXGI_ERROR_NOT_FOUND;
		 ++i) {
		DXGI_ADAPTER_DESC1 desc;
		adapter1->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		if (SUCCEEDED(
				D3D12CreateDevice(adapter1.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)))) {
			adapter1.As(&adapter);
			break;
		}
	}

	return device != nullptr;
}

bool D3D12Device::CreateCommandQueue() {
	D3D12_COMMAND_QUEUE_DESC queue_desc = {};
	queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	return SUCCEEDED(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue)));
}

bool D3D12Device::CreateSwapChain(HWND window_handle, uint32_t width, uint32_t height) {
	DXGI_SWAP_CHAIN_DESC1 sc_desc = {};
	sc_desc.Width = width;
	sc_desc.Height = height;
	sc_desc.Format = render_target_format;
	sc_desc.Stereo = FALSE;
	sc_desc.SampleDesc.Count = 1;
	sc_desc.SampleDesc.Quality = 0;
	sc_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sc_desc.BufferCount = buffer_count;
	sc_desc.Scaling = DXGI_SCALING_STRETCH;
	sc_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sc_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	sc_desc.Flags = 0;

	ComPtr<IDXGISwapChain1> sc1;
	if (FAILED(factory->CreateSwapChainForHwnd(command_queue.Get(), window_handle, &sc_desc,
											   nullptr, nullptr, &sc1))) {
		return false;
	}

	factory->MakeWindowAssociation(window_handle, DXGI_MWA_NO_ALT_ENTER);

	if (FAILED(sc1.As(&swap_chain))) {
		return false;
	}

	current_frame_index = swap_chain->GetCurrentBackBufferIndex();
	return true;
}

bool D3D12Device::CreateDescriptorHeaps() {
	D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
	rtv_heap_desc.NumDescriptors = buffer_count;
	rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	if (FAILED(device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_heap)))) {
		return false;
	}

	rtv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	return true;
}

bool D3D12Device::CreateRenderTargets() {
	D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = rtv_heap->GetCPUDescriptorHandleForHeapStart();

	for (uint32_t i = 0; i < buffer_count; ++i) {
		if (FAILED(swap_chain->GetBuffer(i, IID_PPV_ARGS(&render_targets[i])))) {
			return false;
		}

		device->CreateRenderTargetView(render_targets[i].Get(), nullptr, rtv_handle);
		rtv_handle.ptr += rtv_descriptor_size;
	}

	return true;
}

bool D3D12Device::CreateCommandAllocators() {
	for (uint32_t i = 0; i < buffer_count; ++i) {
		if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
												  IID_PPV_ARGS(&command_allocators[i])))) {
			return false;
		}
	}
	return true;
}

bool D3D12Device::CreateFence() {
	if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))) {
		return false;
	}

	fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!fence_event) {
		return false;
	}

	for (uint32_t i = 0; i < buffer_count; ++i) {
		fence_values[i] = 1;
	}

	return true;
}
