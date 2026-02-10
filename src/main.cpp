#include <windows.h>

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

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, PSTR, int show_command) {
	auto hwnd = CreateAppWindow(instance);
	if (!hwnd)
		return 1;

	ShowWindow(hwnd, show_command);
	UpdateWindow(hwnd);

	constexpr auto BUFFER_COUNT = 8u;

	D3D12Device device;

	D3D12SwapChain swap_chain(*&device.device, *&device.factory, *&device.command_queue,
							  {
								  .window_handle		= hwnd,
								  .frame_width			= WINDOW_WIDTH,
								  .frame_height			= WINDOW_HEIGHT,
								  .buffer_count			= BUFFER_COUNT,
								  .render_target_format = DXGI_FORMAT_R8G8B8A8_UNORM,
							  });

	ComPtr<ID3D12CommandAllocator> allocator;
	std::vector<ComPtr<ID3D12GraphicsCommandList>> command_lists(BUFFER_COUNT);
	std::vector<ComPtr<ID3D12Fence>> fences(BUFFER_COUNT);
	std::vector<HANDLE> fence_events(BUFFER_COUNT);
	{ // Pre-record command lists
		Try
			| device.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
													IID_PPV_ARGS(&allocator));

		auto rtv_start = swap_chain.rtv_heap->GetCPUDescriptorHandleForHeapStart();
		auto rtv	   = rtv_start;
		for (auto i = 0u; i < command_lists.size();
			 ++i, rtv.ptr += swap_chain.rtv_descriptor_size) {
			Try | device.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fences[i]));
			fence_events[i] = CreateEvent(nullptr, FALSE, TRUE, nullptr);
			if (!fence_events[i])
				throw;

			auto& command_list = command_lists[i];
			Try
				| device.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, *&allocator,
												   nullptr, IID_PPV_ARGS(&command_list));

			{
				D3D12_RESOURCE_BARRIER barrier{
					.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
					.Transition = {
							.pResource   = *&swap_chain.render_targets[i],
							.StateBefore = D3D12_RESOURCE_STATE_COMMON,
							.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET,
						},
				};
				command_list->ResourceBarrier(1, &barrier);
			}

			float clear_color[]{(float)(i / 4 % 2), (float)(i / 2 % 2), (float)(i % 2), 1};
			command_list->ClearRenderTargetView(rtv, clear_color, 0, nullptr);

			{
				D3D12_RESOURCE_BARRIER barrier{
					.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
					.Transition = {
							.pResource   = *&swap_chain.render_targets[i],
							.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
							.StateAfter  = D3D12_RESOURCE_STATE_PRESENT,
						},
				};
				command_list->ResourceBarrier(1, &barrier);
			}

			command_list->Close();
		}
	}

	MSG msg{};
	bool running		   = true;
	auto frames_submitted  = 0u;
	auto back_buffer_index = swap_chain.swap_chain->GetCurrentBackBufferIndex();
	while (running) {
		MsgWaitForMultipleObjects(fence_events.size(), fence_events.data(), FALSE, INFINITE,
								  QS_ALLINPUT);

		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				running = false;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (!running)
			break;

		auto completed_value = fences[back_buffer_index]->GetCompletedValue();
		if (completed_value + fences.size() < frames_submitted + 1)
			continue;

		auto command_lists_to_execute = *&command_lists[back_buffer_index];
		device.command_queue->ExecuteCommandLists(
			1, (ID3D12CommandList* const*)&command_lists_to_execute);
		swap_chain.Present(1, 0);
		device.command_queue->Signal(*&fences[back_buffer_index], frames_submitted + 1);

		fences[back_buffer_index]->SetEventOnCompletion(frames_submitted + 1,
														fence_events[back_buffer_index]);

		++frames_submitted;
		back_buffer_index = swap_chain.swap_chain->GetCurrentBackBufferIndex();
	}

	return 0;
}
