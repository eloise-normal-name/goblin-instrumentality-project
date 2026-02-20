#pragma once

#include <windows.h>

#include <chrono>
#include <cstdint>

#include "encoder/frame_encoder.h"

struct AppLogging {
	struct FrameLogContext {
		uint32_t frame;
		double cpu_ms;
	};

	static FrameLogContext BuildFrameLogContext(
		uint32_t frames_submitted,
		std::chrono::time_point<std::chrono::steady_clock>& last_frame_time);
	static void LogFrameLoopStart(const FrameLogContext& frame_log,
								  const FrameEncoder::Stats& stats);
	static void LogFenceCompletion(const FrameLogContext& frame_log, uint64_t completed_value);
	static void LogPresentStatus(const FrameLogContext& frame_log, HRESULT present_result);
	static void LogPresentStillDrawing(const FrameLogContext& frame_log);
	static void LogFrameSubmitResult(const FrameLogContext& frame_log, uint32_t back_buffer_index,
									 uint32_t signaled_value, uint32_t new_back_buffer_index);
};
