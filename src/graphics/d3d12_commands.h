#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include <cstdint>

using Microsoft::WRL::ComPtr;

class D3D12Mesh;

class D3D12Commands {
  public:
	D3D12Commands(ID3D12Device* device, ID3D12CommandAllocator* allocator);
	~D3D12Commands() = default;

	void Reset(ID3D12CommandAllocator* allocator);
	void Close();

	void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before,
							D3D12_RESOURCE_STATES after);
	void ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float color[4]);
	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv);
	void SetViewportAndScissor(uint32_t width, uint32_t height);

	void CopyResource(ID3D12Resource* dest, ID3D12Resource* source);

	ID3D12CommandList* const* GetCommandListForExecution() const;

	ComPtr<ID3D12GraphicsCommandList> command_list;
	mutable ID3D12CommandList* command_list_ptr = nullptr;
};

void ExecuteCommandList(ID3D12CommandQueue* queue, D3D12Commands& commands);

void RecordFrameCommandList(ID3D12GraphicsCommandList* command_list,
							D3D12_CPU_DESCRIPTOR_HANDLE rtv,
							ID3D12Resource* offscreen_render_target,
							ID3D12Resource* swap_chain_render_target, uint32_t width,
							uint32_t height, ID3D12RootSignature* root_signature,
							ID3D12PipelineState* pipeline_state,
							D3D12_GPU_VIRTUAL_ADDRESS mvp_buffer_address, const D3D12Mesh& mesh);
