#include <windows.h>

#include <array>
#include <fstream>
#include <string>
#include <vector>

#include "graphics/d3d12_device.h"
#include "graphics/d3d12_swap_chain.h"
#include "try.h"

constexpr uint32_t WINDOW_WIDTH		  = 512;
constexpr uint32_t WINDOW_HEIGHT	  = 512;
constexpr wchar_t WINDOW_CLASS_NAME[] = L"GoblinStreamWindow";
constexpr wchar_t WINDOW_TITLE[]	  = L"Goblin Stream";

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
	switch (message) {
		case WM_CLOSE:
			PostQuitMessage(0);
			return 0;
		case WM_KEYDOWN:
			if (wparam == VK_ESCAPE) {
				PostQuitMessage(0);
				return 0;
			}
			break;
	}
	return DefWindowProc(hwnd, message, wparam, lparam);
}

HWND CreateAppWindow(HINSTANCE instance) {
	WNDCLASS wc{
		.lpfnWndProc   = WindowProc,
		.hInstance	   = instance,
		.hIcon		   = LoadIcon(nullptr, IDI_QUESTION),
		.hCursor	   = LoadCursor(nullptr, IDC_ARROW),
		.lpszClassName = WINDOW_CLASS_NAME,
	};

	RegisterClass(&wc);

	RECT rect{
		.left	= 0,
		.top	= 0,
		.right	= (LONG)WINDOW_WIDTH,
		.bottom = (LONG)WINDOW_HEIGHT,
	};
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

	return CreateWindow(WINDOW_CLASS_NAME, WINDOW_TITLE, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
						CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr,
						nullptr, instance, nullptr);
}

class FrameResources {
  public:
	FrameResources(ID3D12Device* device, ID3D12CommandAllocator* allocator) {
		Try | device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		fence_event = CreateEvent(nullptr, FALSE, TRUE, nullptr);
		if (!fence_event)
			throw;

		Try
			| device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, nullptr,
										IID_PPV_ARGS(&command_list));
	}

	FrameResources(FrameResources&& rhs) {
		command_list	= std::move(rhs.command_list);
		fence			= std::move(rhs.fence);
		fence_event		= rhs.fence_event;
		rhs.fence_event = nullptr;
	}

	~FrameResources() {
		CloseHandle(fence_event);
	}

	ComPtr<ID3D12GraphicsCommandList> command_list;
	ComPtr<ID3D12Fence> fence;
	HANDLE fence_event = nullptr;
};

void RecordCommandList(ID3D12GraphicsCommandList* command_list, D3D12_CPU_DESCRIPTOR_HANDLE rtv,
					   ID3D12Resource* render_target, uint32_t index) {
	{
		D3D12_RESOURCE_BARRIER barrier{
			.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Transition = {.pResource	= render_target,
						   .StateBefore = D3D12_RESOURCE_STATE_COMMON,
						   .StateAfter	= D3D12_RESOURCE_STATE_RENDER_TARGET}};
		command_list->ResourceBarrier(1, &barrier);
	}

	float clear_color[]{(float)(index / 4 % 2), (float)(index / 2 % 2), (float)(index % 2), 1};
	command_list->ClearRenderTargetView(rtv, clear_color, 0, nullptr);

	{
		D3D12_RESOURCE_BARRIER barrier{
			.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Transition = {.pResource	= render_target,
						   .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
						   .StateAfter	= D3D12_RESOURCE_STATE_PRESENT}};
		command_list->ResourceBarrier(1, &barrier);
	}

	command_list->Close();
}

bool ProcessWindowMessages(MSG& msg) {
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT)
			return false;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return true;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, PSTR, int show_command) {
	auto hwnd = CreateAppWindow(instance);
	if (!hwnd)
		return 1;

	ShowWindow(hwnd, show_command);
	UpdateWindow(hwnd);

	constexpr auto BUFFER_COUNT = 3u;

	D3D12Device device;

	D3D12SwapChain swap_chain{
		*&device.device,
		*&device.factory,
		*&device.command_queue,
		hwnd,
		SwapChainConfig{.frame_width		  = WINDOW_WIDTH,
						.frame_height		  = WINDOW_HEIGHT,
						.buffer_count		  = BUFFER_COUNT,
						.render_target_format = DXGI_FORMAT_R8G8B8A8_UNORM},
	};

	ComPtr<ID3D12CommandAllocator> allocator;
	Try
		| device.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
												IID_PPV_ARGS(&allocator));
	std::vector<FrameResources> frames;
	for (auto i = 0u; i < BUFFER_COUNT; ++i) {
		frames.emplace_back(device.device.Get(), allocator.Get());

		D3D12_CPU_DESCRIPTOR_HANDLE rtv = swap_chain.rtv_heap->GetCPUDescriptorHandleForHeapStart();
		rtv.ptr += i * swap_chain.rtv_descriptor_size;
		auto* render_target = swap_chain.render_targets[i].Get();
		RecordCommandList(frames[i].command_list.Get(), rtv, render_target, i);
	}

	std::array<HANDLE, BUFFER_COUNT> fence_handles{};
	for (auto i = 0u; i < BUFFER_COUNT; ++i)
		fence_handles[i] = frames[i].fence_event;

	MSG msg{};
	bool running		   = true;
	auto frames_submitted  = 0u;
	auto back_buffer_index = swap_chain.swap_chain->GetCurrentBackBufferIndex();
	HRESULT present_result = S_OK;
	std::ofstream debug_output("debug_output.txt");
	while (running) {
		debug_output << "MsgWaitForMultipleObjects...\t";
		auto wait_handle = fence_handles[back_buffer_index];
		auto wait_result = MsgWaitForMultipleObjects(1, &wait_handle, FALSE, INFINITE, QS_ALLINPUT);

		debug_output << "Wait result of fence_handles[" << back_buffer_index << "]: " << wait_result
					 << "\n";

		if (wait_result == WAIT_OBJECT_0 + 1) {
			running = ProcessWindowMessages(msg);
			if (!running)
				break;
			else
				continue;
		}

		auto completed_value = frames[back_buffer_index].fence->GetCompletedValue();
		if (completed_value + frames.size() < frames_submitted + 1)
			continue;
		debug_output << "\tCompleted Value: " << completed_value
					 << "\n\tpresent_result: " << present_result << "\n";

		if (present_result == S_OK) {
			ID3D12CommandList* command_list_to_execute
				= frames[back_buffer_index].command_list.Get();
			ID3D12CommandList* lists[] = {command_list_to_execute};
			device.command_queue->ExecuteCommandLists(1, lists);
		}
		present_result = swap_chain.Present(1, DXGI_PRESENT_DO_NOT_WAIT);
		device.command_queue->Signal(frames[back_buffer_index].fence.Get(), frames_submitted + 1);
		frames[back_buffer_index].fence->SetEventOnCompletion(
			frames_submitted + 1, frames[back_buffer_index].fence_event);
		if (present_result == DXGI_ERROR_WAS_STILL_DRAWING) {
			debug_output << "\t‼Present returned DXGI_ERROR_WAS_STILL_DRAWING, skipping...\n";
			continue;
		}

		debug_output << "\tFrame submitted, fence[" << back_buffer_index
					 << "] signaled with value: " << frames_submitted + 1;
		back_buffer_index = swap_chain.swap_chain->GetCurrentBackBufferIndex();
		++frames_submitted;
		debug_output << "\n\t\tnew back_buffer_index: " << back_buffer_index << "\n";
	}
	debug_output.close();
	return 0;
}
