#include "d3d12_resources.h"

#include "try.h"

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
