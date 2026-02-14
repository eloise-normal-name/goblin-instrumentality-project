#include <windows.h>

#include "crash_diagnostics.h"

import App;

constexpr auto WINDOW_WIDTH	 = 512u;
constexpr auto WINDOW_HEIGHT = 512u;

constexpr wchar_t WINDOW_CLASS_NAME[] = L"GoblinStreamWindow";
constexpr wchar_t WINDOW_TITLE[]	  = L"Goblin Stream";

bool CheckHeadlessFlag(PSTR cmd_line) {
	if (!cmd_line)
		return false;

	char* p = cmd_line;
	while (*p) {
		while (*p == ' ')
			++p;
		if (*p == '\0')
			break;

		char* arg_start = p;
		while (*p && *p != ' ')
			++p;

		size_t len = (size_t)(p - arg_start);
		if (len == 10 && memcmp(arg_start, "--headless", 10) == 0)
			return true;
	}

	return false;
}

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

HWND CreateAppWindow(HINSTANCE instance, int show_command, bool headless) {
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

	auto show_flags = headless ? SW_HIDE : show_command;
	auto hwnd		= CreateWindow(WINDOW_CLASS_NAME, WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
								   CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left,
								   rect.bottom - rect.top, nullptr, nullptr, instance, nullptr);
	if (!hwnd)
		return nullptr;

	ShowWindow(hwnd, show_flags);
	UpdateWindow(hwnd);

	return hwnd;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, PSTR cmd_line, int show_command) {
	bool headless = CheckHeadlessFlag(cmd_line);
	CrashDiagnostics diagnostics("crash_log.txt", headless);

	try {
		diagnostics.LogStep("Parsing command-line arguments");
		diagnostics.Log(headless ? "Headless mode: ENABLED" : "Headless mode: DISABLED");

		diagnostics.LogStep("Creating application window");
		auto hwnd = CreateAppWindow(instance, show_command, headless);
		if (!hwnd) {
			diagnostics.Log("ERROR: Failed to create window");
			return 1;
		}
		diagnostics.Log("Window created successfully");

		diagnostics.LogStep("Initializing application");
		auto return_code = App{hwnd, headless, &diagnostics}.Run();

		diagnostics.Log("Application completed successfully with return code: "
						+ std::to_string(return_code));
		return return_code;
	} catch (const std::exception& e) {
		diagnostics.LogException("WinMain", e.what());
		return 1;
	} catch (...) {
		diagnostics.LogException("WinMain", "Unknown exception");
		return 1;
	}
}
