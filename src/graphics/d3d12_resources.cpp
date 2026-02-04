#include "d3d12_resources.h"

bool SharedTexture::Create(ID3D12Device* dev, const TextureDesc& desc) {
	D3D12_RESOURCE_DESC resource_desc = {};
	resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resource_desc.Alignment = 0;
	resource_desc.Width = desc.width;
	resource_desc.Height = desc.height;
	resource_desc.DepthOrArraySize = 1;
	resource_desc.MipLevels = 1;
	resource_desc.Format = desc.format;
	resource_desc.SampleDesc.Count = 1;
	resource_desc.SampleDesc.Quality = 0;
	resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	if (desc.allow_render_target) {
		resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}

	if (desc.allow_simultaneous_access) {
		resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
	}

	D3D12_HEAP_PROPERTIES heap_props = {};
	heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;
	heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE;
	if (desc.allow_simultaneous_access) {
		heap_flags |= D3D12_HEAP_FLAG_SHARED;
	}

	D3D12_CLEAR_VALUE clear_value = {};
	clear_value.Format = desc.format;
	clear_value.Color[0] = 0.0f;
	clear_value.Color[1] = 0.0f;
	clear_value.Color[2] = 0.0f;
	clear_value.Color[3] = 1.0f;

	D3D12_CLEAR_VALUE* clear_value_ptr = desc.allow_render_target ? &clear_value : nullptr;

	if (FAILED(dev->CreateCommittedResource(&heap_props, heap_flags, &resource_desc,
											D3D12_RESOURCE_STATE_COMMON, clear_value_ptr,
											IID_PPV_ARGS(&resource)))) {
		return false;
	}

	if (desc.allow_simultaneous_access) {
		ComPtr<ID3D12Device> device_ptr;
		resource->GetDevice(IID_PPV_ARGS(&device_ptr));

		if (FAILED(device_ptr->CreateSharedHandle(resource.Get(), nullptr, GENERIC_ALL, nullptr,
												  &shared_handle))) {
			resource.Reset();
			return false;
		}
	}

	width = desc.width;
	height = desc.height;
	format = desc.format;
	current_state = ResourceState::Common;

	return true;
}

void SharedTexture::Reset() {
	if (shared_handle) {
		CloseHandle(shared_handle);
		shared_handle = nullptr;
	}
	resource.Reset();
	width = 0;
	height = 0;
	format = DXGI_FORMAT_UNKNOWN;
	current_state = ResourceState::Common;
}

bool ReadbackBuffer::Create(ID3D12Device* device, uint32_t buffer_size) {
	if (buffer_size == 0)
		return false;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = buffer_size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES heap_props = {};
	heap_props.Type = D3D12_HEAP_TYPE_READBACK;
	heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	if (FAILED(device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &desc,
											   D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
											   IID_PPV_ARGS(&resource)))) {
		return false;
	}

	size = buffer_size;
	return true;
}

void ReadbackBuffer::Reset() {
	resource.Reset();
	size = 0;
}

D3D12_RESOURCE_STATES ToD3D12State(ResourceState state) {
	switch (state) {
		case ResourceState::Common:
			return D3D12_RESOURCE_STATE_COMMON;
		case ResourceState::RenderTarget:
			return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case ResourceState::CopySource:
			return D3D12_RESOURCE_STATE_COPY_SOURCE;
		case ResourceState::CopyDest:
			return D3D12_RESOURCE_STATE_COPY_DEST;
		case ResourceState::VideoEncodeRead:
			return D3D12_RESOURCE_STATE_VIDEO_ENCODE_READ;
		default:
			return D3D12_RESOURCE_STATE_COMMON;
	}
}

D3D12_RESOURCE_BARRIER CreateTransitionBarrier(ID3D12Resource* res, D3D12_RESOURCE_STATES before,
											   D3D12_RESOURCE_STATES after) {
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = res;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	return barrier;
}
