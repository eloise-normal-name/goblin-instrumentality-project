#pragma once

#include <cstdio>

#ifdef ENABLE_FRAME_DEBUG_LOG
#define FRAME_LOG(...)                                                                          \
	do {                                                                                        \
		std::fprintf(stderr, __VA_ARGS__);                                                      \
		std::fputc('\n', stderr);                                                               \
	} while (0)
#else
#define FRAME_LOG(...)                                                                          \
	do {                                                                                        \
	} while (0)
#endif
