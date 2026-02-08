#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include <cstdint>

using Microsoft::WRL::ComPtr;

class D3D12Commands {
  public:
	explicit D3D12Commands(ID3D12Device* device, ID3D12CommandAllocator* allocator);
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
