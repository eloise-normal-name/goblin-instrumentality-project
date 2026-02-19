#include "commands.h"

#include "mesh.h"
#include "resources.h"
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

void RecordFrameCommandList(ID3D12GraphicsCommandList* command_list,
							D3D12_CPU_DESCRIPTOR_HANDLE rtv,
							ID3D12Resource* offscreen_render_target,
							ID3D12Resource* swap_chain_render_target, uint32_t width,
							uint32_t height, ID3D12RootSignature* root_signature,
							ID3D12PipelineState* pipeline_state,
							D3D12_GPU_VIRTUAL_ADDRESS mvp_buffer_address, const D3D12Mesh& mesh) {
	{
		D3D12_RESOURCE_BARRIER barrier{
			.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Transition = {.pResource	= offscreen_render_target,
						   .StateBefore = D3D12_RESOURCE_STATE_COMMON,
						   .StateAfter	= D3D12_RESOURCE_STATE_RENDER_TARGET}};
		command_list->ResourceBarrier(1, &barrier);
	}

	command_list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

	D3D12_VIEWPORT viewport{.TopLeftX = 0.0f,
							.TopLeftY = 0.0f,
							.Width	  = (float)width,
							.Height	  = (float)height,
							.MinDepth = 0.0f,
							.MaxDepth = 1.0f};
	D3D12_RECT scissor{.left = 0, .top = 0, .right = (LONG)width, .bottom = (LONG)height};
	command_list->RSSetViewports(1, &viewport);
	command_list->RSSetScissorRects(1, &scissor);

	command_list->SetGraphicsRootSignature(root_signature);
	command_list->SetPipelineState(pipeline_state);
	command_list->SetGraphicsRootConstantBufferView(0, mvp_buffer_address);

	float clear_color[]{0.0f, 0.0f, 0.0f, 1.0f};
	command_list->ClearRenderTargetView(rtv, clear_color, 0, nullptr);

	mesh.Draw(command_list);

	{
		D3D12_RESOURCE_BARRIER barrier{
			.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Transition = {.pResource	= offscreen_render_target,
						   .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
						   .StateAfter	= D3D12_RESOURCE_STATE_COPY_SOURCE}};
		command_list->ResourceBarrier(1, &barrier);
	}

	{
		D3D12_RESOURCE_BARRIER barrier{
			.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Transition = {.pResource	= swap_chain_render_target,
						   .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
						   .StateAfter	= D3D12_RESOURCE_STATE_COPY_DEST}};
		command_list->ResourceBarrier(1, &barrier);
	}

	command_list->CopyResource(swap_chain_render_target, offscreen_render_target);

	{
		D3D12_RESOURCE_BARRIER barrier{.Type	   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
									   .Transition = {.pResource   = swap_chain_render_target,
													  .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
													  .StateAfter  = D3D12_RESOURCE_STATE_PRESENT}};
		command_list->ResourceBarrier(1, &barrier);
	}

	{
		D3D12_RESOURCE_BARRIER barrier{
			.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Transition = {.pResource	= offscreen_render_target,
						   .StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE,
						   .StateAfter	= D3D12_RESOURCE_STATE_COMMON}};
		command_list->ResourceBarrier(1, &barrier);
	}

	command_list->Close();
}

