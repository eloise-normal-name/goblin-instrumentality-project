#include "d3d12_command_allocators.h"

#include "try.h"

D3D12CommandAllocators::D3D12CommandAllocators(ID3D12Device* device,
											   const CommandAllocatorsConfig& config)
	: buffer_count(config.buffer_count > 0 ? config.buffer_count : 2) {
	if (buffer_count > 3)
		buffer_count = 3;

	create_command_allocators(device);
}

void D3D12CommandAllocators::create_command_allocators(ID3D12Device* device) {
	for (uint32_t i = 0; i < buffer_count; ++i) {
		Try
			| device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
											 IID_PPV_ARGS(&command_allocators[i]));
	}
}
