#include <windows.h>

import App;

constexpr auto WINDOW_WIDTH	 = 512u;
constexpr auto WINDOW_HEIGHT = 512u;

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

HWND CreateAppWindow(HINSTANCE instance, int show_command) {
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

	auto hwnd = CreateWindow(WINDOW_CLASS_NAME, WINDOW_TITLE, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
							 CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr,
							 nullptr, instance, nullptr);
	if (!hwnd)
		return nullptr;

	ShowWindow(hwnd, show_command);
	UpdateWindow(hwnd);

	return hwnd;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, PSTR, int show_command) {
	try {
		auto hwnd = CreateAppWindow(instance, show_command);
		if (!hwnd)
			return 1;

		return App{hwnd}.Run();
	} catch (...) {
		return 1;
	}
}
