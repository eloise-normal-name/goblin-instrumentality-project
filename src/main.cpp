#include <windows.h>

#include "graphics/d3d12_device.h"
#include "graphics/d3d12_swap_chain.h"
#include "pipeline/frame_coordinator.h"

constexpr uint32_t WINDOW_WIDTH		  = 1920;
constexpr uint32_t WINDOW_HEIGHT	  = 1080;
constexpr wchar_t WINDOW_CLASS_NAME[] = L"GoblinStreamWindow";
constexpr wchar_t WINDOW_TITLE[]	  = L"Goblin Stream";

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
	switch (message) {
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_KEYDOWN:
			if (wparam == VK_ESCAPE) {
				PostQuitMessage(0);
				return 0;
			}
			break;
	}
	return DefWindowProcW(hwnd, message, wparam, lparam);
}

HWND CreateAppWindow(HINSTANCE instance) {
	WNDCLASSEXW wc{
		.cbSize		   = sizeof(WNDCLASSEXW),
		.lpfnWndProc   = WindowProc,
		.hInstance	   = instance,
		.lpszClassName = WINDOW_CLASS_NAME,
	};

	RegisterClassExW(&wc);

	RECT rect{
		.left	= 0,
		.top	= 0,
		.right	= (LONG)WINDOW_WIDTH,
		.bottom = (LONG)WINDOW_HEIGHT,
	};
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

	return CreateWindowExW(0, WINDOW_CLASS_NAME, WINDOW_TITLE, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
						   CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr,
						   nullptr, instance, nullptr);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, PSTR, int show_command) {
	auto hwnd = CreateAppWindow(instance);
	if (!hwnd)
		return 1;

	uint32_t buffer_count = 2;

	D3D12Device device({
		.buffer_count = buffer_count,
	});

	D3D12SwapChain swap_chain(device.device.Get(), device.factory.Get(), device.command_queue.Get(),
							  {
								  .window_handle		= hwnd,
								  .frame_width			= WINDOW_WIDTH,
								  .frame_height			= WINDOW_HEIGHT,
								  .buffer_count			= buffer_count,
								  .render_target_format = DXGI_FORMAT_B8G8R8A8_UNORM,
							  });

	PipelineConfig pipeline_config{
		.width		 = WINDOW_WIDTH,
		.height		 = WINDOW_HEIGHT,
		.frame_rate	 = 60,
		.bitrate	 = 8000000,
		.codec		 = EncoderCodec::H264,
		.low_latency = true,
	};
	RenderTargets& render_targets = swap_chain.GetRenderTargets();
	FrameCoordinator coordinator(device, render_targets, pipeline_config);

	ShowWindow(hwnd, show_command);

	MSG msg{};
	bool running = true;

	while (running) {
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				running = false;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		if (!running)
			break;

		try {
			coordinator.BeginFrame();
		} catch (...) {
			continue;
		}

		auto commands = coordinator.GetCommands();

		auto rtv = render_targets.rtv_heap->GetCPUDescriptorHandleForHeapStart();
		rtv.ptr
			+= (SIZE_T)(render_targets.current_frame_index * render_targets.rtv_descriptor_size);

		commands->TransitionResource(
			render_targets.render_targets[render_targets.current_frame_index].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		float clear_color[4] = {1, 0, 1, 1};
		commands->ClearRenderTarget(rtv, clear_color);
		commands->SetRenderTarget(rtv);
		commands->SetViewportAndScissor(WINDOW_WIDTH, WINDOW_HEIGHT);

		try {
			coordinator.EndFrame();
		} catch (...) {
			continue;
		}

		FrameData frame_data{};
		try {
			coordinator.EncodeFrame(frame_data);
		} catch (...) {
			continue;
		}

		uint32_t previous_frame_index = render_targets.current_frame_index;
		swap_chain.Present(0, 0);
		swap_chain.UpdateCurrentFrameIndex();
		device.MoveToNextFrame(previous_frame_index, render_targets.current_frame_index);
	}

	return 0;
}
