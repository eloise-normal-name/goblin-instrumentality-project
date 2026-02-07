#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include <cstdint>

using Microsoft::WRL::ComPtr;

struct RenderTargets {
	ComPtr<ID3D12DescriptorHeap> rtv_heap;
	ComPtr<ID3D12Resource> render_targets[3];
	uint32_t buffer_count		 = 0;
	uint32_t current_frame_index = 0;
	uint32_t rtv_descriptor_size = 0;
};
