#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include "d3d12_resources.h"

using Microsoft::WRL::ComPtr;

class D3D12Commands {
  public:
	D3D12Commands() = default;
	~D3D12Commands() = default;

	D3D12Commands(const D3D12Commands&) = delete;
	D3D12Commands& operator=(const D3D12Commands&) = delete;
	D3D12Commands(D3D12Commands&&) = default;
	D3D12Commands& operator=(D3D12Commands&&) = default;

	bool Initialize(ID3D12Device* device);
	void Reset();
	void Close();

	void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before,
							D3D12_RESOURCE_STATES after);
	void TransitionTexture(SharedTexture& texture, ResourceState new_state);

	void ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float color[4]);
	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv);
	void SetViewportAndScissor(uint32_t width, uint32_t height);

	void CopyResource(ID3D12Resource* dest, ID3D12Resource* source);
	void CopyTexture(SharedTexture& dest, ID3D12Resource* source);

	ID3D12CommandList* const* GetCommandListForExecution() const;

	ComPtr<ID3D12CommandAllocator> command_allocator;
	ComPtr<ID3D12GraphicsCommandList> command_list;
	mutable ID3D12CommandList* command_list_ptr = nullptr;
};

void ExecuteCommandList(ID3D12CommandQueue* queue, D3D12Commands& commands);
