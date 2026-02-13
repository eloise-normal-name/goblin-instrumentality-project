#include "d3d12_commands.h"

#include "d3d12_resources.h"
#include "try.h"

D3D12Commands::D3D12Commands(ID3D12Device* device, ID3D12CommandAllocator* allocator) {
	Try
		| device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, nullptr,
									IID_PPV_ARGS(&command_list));

	command_list->Close();
}

void D3D12Commands::Reset(ID3D12CommandAllocator* allocator) {
	allocator->Reset();
	command_list->Reset(allocator, nullptr);
}

void D3D12Commands::Close() {
	command_list->Close();
}

void D3D12Commands::TransitionResource(ID3D12Resource* res, D3D12_RESOURCE_STATES before,
									   D3D12_RESOURCE_STATES after) {
	if (before == after)
		return;

	D3D12_RESOURCE_BARRIER barrier = CreateTransitionBarrier(res, before, after);
	command_list->ResourceBarrier(1, &barrier);
}

void D3D12Commands::ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float color[4]) {
	command_list->ClearRenderTargetView(rtv, color, 0, nullptr);
}

void D3D12Commands::SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv) {
	command_list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
}

void D3D12Commands::SetViewportAndScissor(uint32_t w, uint32_t h) {
	D3D12_VIEWPORT viewport{
		.TopLeftX = 0.0f,
		.TopLeftY = 0.0f,
		.Width	  = (float)w,
		.Height	  = (float)h,
		.MinDepth = 0.0f,
		.MaxDepth = 1.0f,
	};

	D3D12_RECT scissor{
		.left	= 0,
		.top	= 0,
		.right	= (LONG)w,
		.bottom = (LONG)h,
	};

	command_list->RSSetViewports(1, &viewport);
	command_list->RSSetScissorRects(1, &scissor);
}

void D3D12Commands::CopyResource(ID3D12Resource* dest, ID3D12Resource* source) {
	command_list->CopyResource(dest, source);
}

ID3D12CommandList* const* D3D12Commands::GetCommandListForExecution() const {
	command_list_ptr = command_list.Get();
	return &command_list_ptr;
}

void ExecuteCommandList(ID3D12CommandQueue* queue, D3D12Commands& cmds) {
	cmds.Close();
	queue->ExecuteCommandLists(1, cmds.GetCommandListForExecution());
}
