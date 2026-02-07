#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include <cstdint>

using Microsoft::WRL::ComPtr;

struct CommandAllocatorsConfig {
	uint32_t buffer_count = 3;
};

class D3D12CommandAllocators {
  public:
	explicit D3D12CommandAllocators(ID3D12Device* device, const CommandAllocatorsConfig& config);
	~D3D12CommandAllocators() = default;

	ID3D12CommandAllocator* GetAllocator(uint32_t frame_index) const {
		return command_allocators[frame_index].Get();
	}

	uint32_t buffer_count = 2;

  private:
	ComPtr<ID3D12CommandAllocator> command_allocators[3];

	void create_command_allocators(ID3D12Device* device);
};
