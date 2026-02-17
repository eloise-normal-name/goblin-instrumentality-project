#pragma once

#include <d3d12.h>
#include <wrl.h>

#include <cstdint>

class D3D12Mesh {
  public:
	struct Vertex {
		float position[3];
		float color[3];
	};

  private:
	Microsoft::WRL::ComPtr<ID3D12Resource> vertex_buffer;
	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view{};
	uint32_t vertex_count = 0;

  public:
	D3D12Mesh(ID3D12Device* device);

	void Draw(ID3D12GraphicsCommandList* command_list) const;

	const D3D12_VERTEX_BUFFER_VIEW& GetView() const {
		return vertex_buffer_view;
	}
};
