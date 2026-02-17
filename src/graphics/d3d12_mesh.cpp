#include "graphics/d3d12_mesh.h"

#include <array>
#include <cstring>

#include "try.h"

D3D12Mesh::D3D12Mesh(ID3D12Device* device) {
	std::array<Vertex, 3> vertices{
		Vertex{.position = {0.0f, 0.25f, 0.0f}, .color = {1.0f, 0.0f, 0.0f}},
		Vertex{.position = {0.25f, -0.25f, 0.0f}, .color = {0.0f, 1.0f, 0.0f}},
		Vertex{.position = {-0.25f, -0.25f, 0.0f}, .color = {0.0f, 0.0f, 1.0f}},
	};

	vertex_count	 = (uint32_t)vertices.size();
	auto buffer_size = (UINT)(sizeof(Vertex) * vertices.size());

	D3D12_HEAP_PROPERTIES heap_properties{
		.Type = D3D12_HEAP_TYPE_UPLOAD,
	};

	D3D12_RESOURCE_DESC resource_desc{
		.Dimension		  = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Width			  = buffer_size,
		.Height			  = 1,
		.DepthOrArraySize = 1,
		.MipLevels		  = 1,
		.SampleDesc		  = {.Count = 1},
		.Layout			  = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
	};

	Try
		| device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc,
										  D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
										  IID_PPV_ARGS(&vertex_buffer));

	void* mapped = nullptr;
	D3D12_RANGE range{.Begin = 0, .End = 0};
	Try | vertex_buffer->Map(0, &range, &mapped);
	memcpy(mapped, vertices.data(), buffer_size);
	vertex_buffer->Unmap(0, nullptr);

	vertex_buffer_view = D3D12_VERTEX_BUFFER_VIEW{
		.BufferLocation = vertex_buffer->GetGPUVirtualAddress(),
		.SizeInBytes	= buffer_size,
		.StrideInBytes	= sizeof(Vertex),
	};
}

void D3D12Mesh::Draw(ID3D12GraphicsCommandList* command_list) const {
	command_list->IASetVertexBuffers(0, 1, &vertex_buffer_view);
	command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	command_list->DrawInstanced(vertex_count, 1, 0, 0);
}
