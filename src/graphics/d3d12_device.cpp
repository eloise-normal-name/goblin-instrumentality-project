#include "d3d12_device.h"

#include "try.h"

D3D12Device::D3D12Device() {
	create_device();
	create_command_queue();
}

D3D12Device::~D3D12Device() {
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
