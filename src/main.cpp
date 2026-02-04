#include <windows.h>

#include "graphics/d3d12_device.h"
#include "pipeline/frame_coordinator.h"

namespace {

constexpr uint32_t WINDOW_WIDTH = 1920;
constexpr uint32_t WINDOW_HEIGHT = 1080;
constexpr wchar_t WINDOW_CLASS_NAME[] = L"GoblinStreamWindow";
constexpr wchar_t WINDOW_TITLE[] = L"Goblin Stream";

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
	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = instance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = WINDOW_CLASS_NAME;

	RegisterClassExW(&wc);

	RECT rect = {0, 0, static_cast<LONG>(WINDOW_WIDTH), static_cast<LONG>(WINDOW_HEIGHT)};
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

	return CreateWindowExW(0, WINDOW_CLASS_NAME, WINDOW_TITLE, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
						   CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr,
						   nullptr, instance, nullptr);
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, PSTR, int show_command) {
	HWND hwnd = CreateAppWindow(instance);
	if (!hwnd)
		return 1;

	D3D12Device device;
	DeviceConfig device_config = {};
	device_config.window_handle = hwnd;
	device_config.frame_width = WINDOW_WIDTH;
	device_config.frame_height = WINDOW_HEIGHT;
	device_config.buffer_count = 2;
	device_config.render_target_format = DXGI_FORMAT_R8G8B8A8_UNORM;

	if (!device.Initialize(device_config))
		return 1;

	FrameCoordinator coordinator;
	PipelineConfig pipeline_config = {};
	pipeline_config.width = WINDOW_WIDTH;
	pipeline_config.height = WINDOW_HEIGHT;
	pipeline_config.frame_rate = 60;
	pipeline_config.bitrate = 8000000;
	pipeline_config.codec = EncoderCodec::H264;
	pipeline_config.low_latency = true;

	if (!coordinator.Initialize(&device, pipeline_config))
		return 1;

	ShowWindow(hwnd, show_command);

	MSG msg = {};
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

		if (!coordinator.BeginFrame())
			continue;

		auto* commands = coordinator.GetCommands();

		D3D12_CPU_DESCRIPTOR_HANDLE rtv = device.rtv_heap->GetCPUDescriptorHandleForHeapStart();
		rtv.ptr += static_cast<SIZE_T>(device.current_frame_index * device.rtv_descriptor_size);

		commands->TransitionResource(device.render_targets[device.current_frame_index].Get(),
									 D3D12_RESOURCE_STATE_PRESENT,
									 D3D12_RESOURCE_STATE_RENDER_TARGET);

		float clear_color[4] = {0.1f, 0.2f, 0.3f, 1.0f};
		commands->ClearRenderTarget(rtv, clear_color);
		commands->SetRenderTarget(rtv);
		commands->SetViewportAndScissor(WINDOW_WIDTH, WINDOW_HEIGHT);

		if (!coordinator.EndFrame())
			continue;

		FrameData frame_data = {};
		coordinator.EncodeFrame(frame_data);

		device.swap_chain->Present(1, 0);
		device.MoveToNextFrame();
	}

	coordinator.Shutdown();
	device.Shutdown();

	return 0;
}
