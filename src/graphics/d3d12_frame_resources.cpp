#include "d3d12_frame_resources.h"

#include <wrl/client.h>

#include "try.h"

using Microsoft::WRL::ComPtr;

D3D12FrameResources::D3D12FrameResources(ID3D12Device* device, uint32_t count, uint32_t width,
								 uint32_t height, DXGI_FORMAT format) {
	command_lists.resize(count);
	fences.resize(count);
	fence_events.resize(count);
	offscreen_render_targets.resize(count);

	ComPtr<ID3D12Device4> device4;
	Try | device->QueryInterface(IID_PPV_ARGS(&device4));

	D3D12_HEAP_PROPERTIES default_heap_props{
		.Type = D3D12_HEAP_TYPE_DEFAULT,
	};

	D3D12_RESOURCE_DESC texture_desc{
		.Dimension		  = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		.Width			  = width,
		.Height			  = height,
		.DepthOrArraySize = 1,
		.MipLevels		  = 1,
		.Format			  = format,
		.SampleDesc		  = {.Count = 1},
		.Flags			  = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
	};

	D3D12_CLEAR_VALUE clear_value{
		.Format = format,
		.Color	= {0, 0, 0, 1},
	};

	for (auto i = 0u; i < count; ++i) {
		fence_events[i] = CreateEvent(nullptr, FALSE, TRUE, nullptr);
		if (!fence_events[i])
			throw;

		ID3D12GraphicsCommandList1* command_list1 = nullptr;
		Try | device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fences[i]))
			| device4->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
									  D3D12_COMMAND_LIST_FLAG_NONE,
									  IID_PPV_ARGS(&command_list1))
			| device->CreateCommittedResource(&default_heap_props, D3D12_HEAP_FLAG_NONE,
									  &texture_desc, D3D12_RESOURCE_STATE_COMMON,
									  &clear_value,
									  IID_PPV_ARGS(&offscreen_render_targets[i]));
		command_lists[i] = (ID3D12GraphicsCommandList*)command_list1;
	}
}

D3D12FrameResources::~D3D12FrameResources() {
	for (auto command_list : command_lists)
		if (command_list)
			command_list->Release();
	command_lists.clear();

	for (auto fence : fences)
		if (fence)
			fence->Release();
	fences.clear();

	for (auto offscreen_render_target : offscreen_render_targets)
		if (offscreen_render_target)
			offscreen_render_target->Release();
	offscreen_render_targets.clear();

	for (auto fence_event : fence_events)
		if (fence_event)
			CloseHandle(fence_event);
	fence_events.clear();
}
