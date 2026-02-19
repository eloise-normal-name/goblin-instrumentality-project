#pragma once

#include <windows.h>

#include <array>
#include <cstdio>

#ifdef ENABLE_FRAME_DEBUG_LOG
#define FRAME_LOG(...)                                                                          \
	do {                                                                                        \
		std::array<char, 2048> frame_log_buffer{};                                              \
		auto frame_log_written = std::snprintf(frame_log_buffer.data(), frame_log_buffer.size(),\
												__VA_ARGS__);                                   \
		if (frame_log_written > 0) {                                                            \
			OutputDebugStringA(frame_log_buffer.data());                                        \
			OutputDebugStringA("\n");                                                           \
		}                                                                                       \
	} while (0)
#else
#define FRAME_LOG(...)                                                                          \
	do {                                                                                        \
	} while (0)
#endif
