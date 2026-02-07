#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <cstdint>

using Microsoft::WRL::ComPtr;

struct DeviceConfig {
	uint32_t buffer_count = 3;
};

class D3D12Device {
  public:
	explicit D3D12Device(const DeviceConfig& config);
	~D3D12Device();

	ComPtr<IDXGIFactory7> factory;
	ComPtr<IDXGIAdapter4> adapter;
	ComPtr<ID3D12Device> device;
	ComPtr<ID3D12CommandQueue> command_queue;
	ComPtr<ID3D12CommandAllocator> command_allocators[3];

	uint32_t buffer_count = 2;

  private:
	void create_device();
	void create_command_queue();
	void create_command_allocators();
	void create_fence();
};
