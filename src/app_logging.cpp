#include "app_logging.h"

#include "debug_log.h"

AppLogging::FrameLogContext AppLogging::BuildFrameLogContext(
	uint32_t frames_submitted,
	std::chrono::time_point<std::chrono::steady_clock>& last_frame_time) {
	auto now		= std::chrono::steady_clock::now();
	auto cpu_ms		= std::chrono::duration<double, std::milli>(now - last_frame_time).count();
	last_frame_time = now;
	return FrameLogContext{.frame = frames_submitted, .cpu_ms = cpu_ms};
}

void AppLogging::LogFrameLoopStart(const FrameLogContext& frame_log,
								   const FrameEncoder::Stats& stats) {
#ifndef ENABLE_FRAME_DEBUG_LOG
	(void)frame_log;
	(void)stats;
#endif
	FRAME_LOG(
		"frame=%u cpu_ms=%.3f encoder_stats submitted=%llu completed=%llu pending=%llu "
		"waits=%llu",
		frame_log.frame, frame_log.cpu_ms, stats.submitted_frames, stats.completed_frames,
		stats.pending_frames, stats.wait_count);
}

void AppLogging::LogFenceCompletion(const FrameLogContext& frame_log, uint64_t completed_value) {
#ifndef ENABLE_FRAME_DEBUG_LOG
	(void)frame_log;
	(void)completed_value;
#endif
	FRAME_LOG("frame=%u cpu_ms=%.3f fence_completed_value=%llu", frame_log.frame, frame_log.cpu_ms,
			  completed_value);
}

void AppLogging::LogPresentStatus(const FrameLogContext& frame_log, HRESULT present_result) {
#ifndef ENABLE_FRAME_DEBUG_LOG
	(void)frame_log;
	(void)present_result;
#endif
	FRAME_LOG("frame=%u cpu_ms=%.3f present_result=%ld", frame_log.frame, frame_log.cpu_ms,
			  present_result);
}

void AppLogging::LogPresentStillDrawing(const FrameLogContext& frame_log) {
#ifndef ENABLE_FRAME_DEBUG_LOG
	(void)frame_log;
#endif
	FRAME_LOG("frame=%u cpu_ms=%.3f present=DXGI_ERROR_WAS_STILL_DRAWING skipping", frame_log.frame,
			  frame_log.cpu_ms);
}

void AppLogging::LogFrameSubmitResult(const FrameLogContext& frame_log, uint32_t back_buffer_index,
									  uint32_t signaled_value, uint32_t new_back_buffer_index) {
#ifndef ENABLE_FRAME_DEBUG_LOG
	(void)frame_log;
	(void)back_buffer_index;
	(void)signaled_value;
	(void)new_back_buffer_index;
#endif
	FRAME_LOG("frame=%u cpu_ms=%.3f frame_submitted fence_index=%u signal_value=%u",
			  frame_log.frame, frame_log.cpu_ms, back_buffer_index, signaled_value);
	FRAME_LOG("frame=%u cpu_ms=%.3f new_back_buffer_index=%u", frame_log.frame, frame_log.cpu_ms,
			  new_back_buffer_index);
}
