#include "d3d12_resources.h"

#include "try.h"

SharedTexture::SharedTexture(ID3D12Device* dev, const TextureDesc& desc)
	: width(desc.width)
	, height(desc.height)
	, format(desc.format)
	, current_state(ResourceState::Common) {
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
	if (desc.allow_render_target)
		flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (desc.allow_simultaneous_access)
		flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;

	D3D12_RESOURCE_DESC resource_desc{
		.Dimension		  = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		.Width			  = desc.width,
		.Height			  = desc.height,
		.DepthOrArraySize = 1,
		.MipLevels		  = 1,
		.Format			  = desc.format,
		.SampleDesc		  = {.Count = 1},
		.Flags			  = flags,
	};

	D3D12_HEAP_PROPERTIES heap_props{
		.Type = D3D12_HEAP_TYPE_DEFAULT,
	};

	D3D12_HEAP_FLAGS heap_flags
		= desc.allow_simultaneous_access ? D3D12_HEAP_FLAG_SHARED : D3D12_HEAP_FLAG_NONE;

	D3D12_CLEAR_VALUE clear_value{
		.Format = desc.format,
		.Color	= {0.0f, 0.0f, 0.0f, 1.0f},
	};

	D3D12_CLEAR_VALUE* clear_value_ptr = desc.allow_render_target ? &clear_value : nullptr;

	Try
		| dev->CreateCommittedResource(&heap_props, heap_flags, &resource_desc,
									   D3D12_RESOURCE_STATE_COMMON, clear_value_ptr,
									   IID_PPV_ARGS(&resource));

	if (desc.allow_simultaneous_access) {
		ComPtr<ID3D12Device> device_ptr;
		resource->GetDevice(IID_PPV_ARGS(&device_ptr));

		Try
			| device_ptr->CreateSharedHandle(resource.Get(), nullptr, GENERIC_ALL, nullptr,
											 &shared_handle);
	}
}

SharedTexture::~SharedTexture() {
	if (shared_handle)
		CloseHandle(shared_handle);
}

ReadbackBuffer::ReadbackBuffer(ID3D12Device* device, uint32_t buffer_size) : size(buffer_size) {
	if (buffer_size == 0)
		throw;

	D3D12_RESOURCE_DESC desc{
		.Dimension		  = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Width			  = buffer_size,
		.Height			  = 1,
		.DepthOrArraySize = 1,
		.MipLevels		  = 1,
		.SampleDesc		  = {.Count = 1},
		.Layout			  = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
	};

	D3D12_HEAP_PROPERTIES heap_props{
		.Type = D3D12_HEAP_TYPE_READBACK,
	};

	Try
		| device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &desc,
										  D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
										  IID_PPV_ARGS(&resource));
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
	return D3D12_RESOURCE_BARRIER{
		.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		.Transition = {
			.pResource = res,
			.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			.StateBefore = before,
			.StateAfter = after,
		},
	};
}
