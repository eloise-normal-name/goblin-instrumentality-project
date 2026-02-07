#pragma once

#include <d3d12.h>
#include <windows.h>
#include <wrl/client.h>

#include <cstdint>

using Microsoft::WRL::ComPtr;

struct FrameSyncConfig {
	uint32_t buffer_count = 3;
};

class D3D12FrameSync {
  public:
	explicit D3D12FrameSync(ID3D12Device* device, ID3D12CommandQueue* command_queue,
							const FrameSyncConfig& config);
	~D3D12FrameSync();

	void WaitForGpu();
	void MoveToNextFrame(uint32_t previous_frame_index, uint32_t next_frame_index);
	uint64_t SignalFenceForFrame(uint32_t frame_index);
	void SetFenceEvent(uint64_t value, HANDLE event);

	ComPtr<ID3D12Fence> fence;
	HANDLE fence_event		 = nullptr;
	uint64_t fence_values[3] = {};

  private:
	void create_fence();

	ID3D12Device* device			  = nullptr;
	ID3D12CommandQueue* command_queue = nullptr;
	uint32_t buffer_count			  = 2;
};
