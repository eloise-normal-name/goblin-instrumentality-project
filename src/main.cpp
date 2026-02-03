#include <windows.h>

#include "graphics/d3d12_device.h"
#include "pipeline/frame_coordinator.h"

namespace {

constexpr uint32_t kWindowWidth = 1920;
constexpr uint32_t kWindowHeight = 1080;
constexpr wchar_t kWindowClassName[] = L"GoblinStreamWindow";
constexpr wchar_t kWindowTitle[] = L"Goblin Stream";

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
    wc.lpszClassName = kWindowClassName;

    RegisterClassExW(&wc);

    RECT rect = {0, 0, static_cast<LONG>(kWindowWidth), static_cast<LONG>(kWindowHeight)};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    return CreateWindowExW(0, kWindowClassName, kWindowTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                           CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr,
                           nullptr, instance, nullptr);
}

}  // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, PSTR, int show_command) {
    HWND hwnd = CreateAppWindow(instance);
    if (!hwnd) return 1;

    D3D12Device device;
    DeviceConfig device_config = {};
    device_config.window_handle = hwnd;
    device_config.frame_width = kWindowWidth;
    device_config.frame_height = kWindowHeight;
    device_config.buffer_count = 2;
    device_config.render_target_format = DXGI_FORMAT_R8G8B8A8_UNORM;

    if (!device.Initialize(device_config)) return 1;

    FrameCoordinator coordinator;
    PipelineConfig pipeline_config = {};
    pipeline_config.width = kWindowWidth;
    pipeline_config.height = kWindowHeight;
    pipeline_config.frame_rate = 60;
    pipeline_config.bitrate = 8000000;
    pipeline_config.codec = EncoderCodec::H264;
    pipeline_config.low_latency = true;

    if (!coordinator.Initialize(&device, pipeline_config)) return 1;

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

        if (!running) break;

        if (!coordinator.BeginFrame()) continue;

        auto* commands = coordinator.GetCommands();
        auto rtv = device.GetCurrentRenderTargetView();

        commands->TransitionResource(device.GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_PRESENT,
                                      D3D12_RESOURCE_STATE_RENDER_TARGET);

        float clear_color[4] = {0.1f, 0.2f, 0.3f, 1.0f};
        commands->ClearRenderTarget(rtv, clear_color);
        commands->SetRenderTarget(rtv);
        commands->SetViewportAndScissor(kWindowWidth, kWindowHeight);

        if (!coordinator.EndFrame()) continue;

        FrameData frame_data = {};
        coordinator.EncodeFrame(frame_data);

        device.GetSwapChain()->Present(1, 0);
        device.MoveToNextFrame();
    }

    coordinator.Shutdown();
    device.Shutdown();

    return 0;
}
