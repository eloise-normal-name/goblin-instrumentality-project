#pragma once

#include <windows.h>

#include <cstdint>
#include <fstream>
#include <ostream>

class FrameDebugLog {
  public:
	FrameDebugLog(const char* path);

	void BeginFrame(uint64_t frame_index);
	std::ostream& Line();

  private:
	void LogPrefix();
	double MeasureFrameMs();

	std::ofstream log_file;
	LARGE_INTEGER perf_frequency{};
	LARGE_INTEGER last_counter{};
	bool has_last_counter = false;
	uint64_t frame_index  = 0;
	double frame_time_ms  = 0.0;
};
