#include "d3d12_device.h"

#include "try.h"

D3D12Device::D3D12Device(const DeviceConfig& config)
	: buffer_count(config.buffer_count > 0 ? config.buffer_count : 2) {
	if (buffer_count > 3)
		buffer_count = 3;

	create_device();
	create_command_queue();
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

	for (uint32_t i = 0; i < buffer_count; ++i) {
		uint64_t fence_value_to_signal = fence_values[i] + 1;
		Try | command_queue->Signal(fence.Get(), fence_value_to_signal);

		if (fence->GetCompletedValue() < fence_value_to_signal) {
			Try | fence->SetEventOnCompletion(fence_value_to_signal, fence_event);
			WaitForSingleObject(fence_event, INFINITE);
		}

		fence_values[i] = fence_value_to_signal;
	}
}

void D3D12Device::MoveToNextFrame(uint32_t previous_frame_index, uint32_t next_frame_index) {
	if (previous_frame_index >= buffer_count || next_frame_index >= buffer_count)
		throw;

	uint64_t current_fence_value = fence_values[previous_frame_index];
	Try | command_queue->Signal(fence.Get(), current_fence_value);

	if (fence->GetCompletedValue() < fence_values[next_frame_index]) {
		Try | fence->SetEventOnCompletion(fence_values[next_frame_index], fence_event);
		WaitForSingleObject(fence_event, INFINITE);
	}

	fence_values[next_frame_index] = current_fence_value + 1;
}

uint64_t D3D12Device::SignalFenceForFrame(uint32_t frame_index) {
	if (frame_index >= buffer_count)
		throw;

	uint64_t fence_value_to_signal = fence_values[frame_index] + 1;
	Try | command_queue->Signal(fence.Get(), fence_value_to_signal);
	fence_values[frame_index] = fence_value_to_signal;
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
