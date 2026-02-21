#include "frame_resources.h"

#include <wrl/client.h>

#include "try.h"

using Microsoft::WRL::ComPtr;

D3D12FrameResources::D3D12FrameResources(ID3D12Device* device, uint32_t count) {
	command_lists.resize(count);
	fences.resize(count);
	fence_events.resize(count);

	ComPtr<ID3D12Device4> device4;
	Try | device->QueryInterface(IID_PPV_ARGS(&device4));

	for (auto i = 0u; i < count; ++i) {
		fence_events[i] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (!fence_events[i])
			throw;

		ID3D12GraphicsCommandList1* command_list1 = nullptr;
		Try | device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fences[i]))
			| device4->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
										  D3D12_COMMAND_LIST_FLAG_NONE,
										  IID_PPV_ARGS(&command_list1));
		command_lists[i] = (ID3D12GraphicsCommandList*)command_list1;
	}
}

D3D12FrameResources::~D3D12FrameResources() {
	for (auto command_list : command_lists)
		if (command_list)
			command_list->Release();

	for (auto fence : fences)
		if (fence)
			fence->Release();

	for (auto fence_event : fence_events)
		if (fence_event)
			CloseHandle(fence_event);
}
