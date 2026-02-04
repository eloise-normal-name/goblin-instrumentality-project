#include "d3d12_commands.h"

bool D3D12Commands::Initialize(ID3D12Device* dev) {
	if (FAILED(dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
										   IID_PPV_ARGS(&command_allocator)))) {
		return false;
	}

	if (FAILED(dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator.Get(),
									  nullptr, IID_PPV_ARGS(&command_list)))) {
		return false;
	}

	command_list->Close();
	return true;
}

void D3D12Commands::Reset() {
	command_allocator->Reset();
	command_list->Reset(command_allocator.Get(), nullptr);
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

void D3D12Commands::TransitionTexture(SharedTexture& texture, ResourceState new_state) {
	ResourceState cur_state = texture.GetCurrentState();
	if (cur_state == new_state)
		return;

	D3D12_RESOURCE_STATES before = ToD3D12State(cur_state);
	D3D12_RESOURCE_STATES after = ToD3D12State(new_state);

	TransitionResource(texture.GetResource(), before, after);
	texture.SetCurrentState(new_state);
}

void D3D12Commands::ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float color[4]) {
	command_list->ClearRenderTargetView(rtv, color, 0, nullptr);
}

void D3D12Commands::SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv) {
	command_list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
}

void D3D12Commands::SetViewportAndScissor(uint32_t w, uint32_t h) {
	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(w);
	viewport.Height = static_cast<float>(h);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	D3D12_RECT scissor = {};
	scissor.left = 0;
	scissor.top = 0;
	scissor.right = static_cast<LONG>(w);
	scissor.bottom = static_cast<LONG>(h);

	command_list->RSSetViewports(1, &viewport);
	command_list->RSSetScissorRects(1, &scissor);
}

void D3D12Commands::CopyResource(ID3D12Resource* dest, ID3D12Resource* source) {
	command_list->CopyResource(dest, source);
}

void D3D12Commands::CopyTexture(SharedTexture& dest, ID3D12Resource* source) {
	TransitionTexture(dest, ResourceState::CopyDest);
	command_list->CopyResource(dest.GetResource(), source);
}

ID3D12CommandList* const* D3D12Commands::GetCommandListForExecution() const {
	command_list_ptr = command_list.Get();
	return &command_list_ptr;
}

void ExecuteCommandList(ID3D12CommandQueue* queue, D3D12Commands& cmds) {
	cmds.Close();
	queue->ExecuteCommandLists(1, cmds.GetCommandListForExecution());
}
