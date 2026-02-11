#include "frame_debug_log.h"

#include <iomanip>

FrameDebugLog::FrameDebugLog(const char* path)
	: log_file(path, std::ios::out | std::ios::trunc) {
	if (!log_file)
		throw;

	if (QueryPerformanceFrequency(&perf_frequency) == 0)
		perf_frequency.QuadPart = 0;
}

void FrameDebugLog::BeginFrame(uint64_t new_frame_index) {
	frame_index = new_frame_index;
	frame_time_ms = MeasureFrameMs();
}

std::ostream& FrameDebugLog::Line() {
	LogPrefix();
	return log_file;
}

void FrameDebugLog::LogPrefix() {
	log_file << "[frame " << frame_index << " | cpu_ms " << std::fixed
			 << std::setprecision(3) << frame_time_ms << "] ";
}

double FrameDebugLog::MeasureFrameMs() {
	if (perf_frequency.QuadPart == 0)
		return 0.0;

	LARGE_INTEGER current_counter{};
	QueryPerformanceCounter(&current_counter);

	double frame_ms = 0.0;
	if (has_last_counter) {
		auto delta = current_counter.QuadPart - last_counter.QuadPart;
		frame_ms = (double)delta * 1000.0 / (double)perf_frequency.QuadPart;
	}

	last_counter = current_counter;
	has_last_counter = true;
	return frame_ms;
}
