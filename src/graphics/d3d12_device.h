#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <cstdint>

using Microsoft::WRL::ComPtr;

enum class AdapterType {
	Hardware,
	WARP,
};

class D3D12Device {
  public:
	D3D12Device(AdapterType adapter_type = AdapterType::Hardware);
	~D3D12Device();

	ComPtr<IDXGIFactory7> factory;
	ComPtr<IDXGIAdapter4> adapter;
	ComPtr<ID3D12Device> device;
	ComPtr<ID3D12CommandQueue> command_queue;

  private:
	void create_device(AdapterType adapter_type);
	void create_command_queue();
};
