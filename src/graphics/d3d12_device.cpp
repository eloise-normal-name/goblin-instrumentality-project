#include "d3d12_device.h"

#include "try.h"

D3D12Device::D3D12Device() {
	CreateDevice();
	CreateCommandQueue();
}

D3D12Device::~D3D12Device() {
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

	Try | CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&factory));

	ComPtr<IDXGIAdapter1> selected_adapter;

	Try
		| factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
											  IID_PPV_ARGS(&selected_adapter))
		| D3D12CreateDevice(*&selected_adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device));
}

void D3D12Device::CreateCommandQueue() {
	D3D12_COMMAND_QUEUE_DESC queue_desc{
		.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
	};

	Try | device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue));
}
