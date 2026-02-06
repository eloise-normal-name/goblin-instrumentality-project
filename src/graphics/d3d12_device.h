#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <cstdint>

using Microsoft::WRL::ComPtr;

struct DeviceConfig {
	HWND window_handle;
	uint32_t frame_width;
	uint32_t frame_height;
	uint32_t buffer_count = 3;
	DXGI_FORMAT render_target_format;
};

class D3D12Device {
  public:
	explicit D3D12Device(const DeviceConfig& config);
	~D3D12Device();

	void WaitForGpu();
	void MoveToNextFrame();
	uint64_t SignalFenceForCurrentFrame();
	void SetFenceEvent(uint64_t value, HANDLE event);

	ComPtr<IDXGIFactory7> factory;
	ComPtr<IDXGIAdapter4> adapter;
	ComPtr<ID3D12Device> device;
	ComPtr<ID3D12CommandQueue> command_queue;
	ComPtr<IDXGISwapChain4> swap_chain;
	ComPtr<ID3D12DescriptorHeap> rtv_heap;
	ComPtr<ID3D12Resource> render_targets[3];
	ComPtr<ID3D12CommandAllocator> command_allocators[3];
	ComPtr<ID3D12Fence> fence;
	HANDLE fence_event = nullptr;

	uint64_t fence_values[3]		 = {};
	uint32_t current_frame_index	 = 0;
	uint32_t buffer_count			 = 2;
	uint32_t rtv_descriptor_size	 = 0;
	DXGI_FORMAT render_target_format = DXGI_FORMAT_B8G8R8A8_UNORM;

  private:
	void create_device();
	void create_command_queue();
	void create_swap_chain(HWND window_handle, uint32_t width, uint32_t height);
	void create_descriptor_heaps();
	void create_render_targets();
	void create_command_allocators();
	void create_fence();
};
