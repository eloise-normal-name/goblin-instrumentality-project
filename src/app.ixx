module;

#include <windows.h>

#include <chrono>
#include <cstring>
#include <utility>
#include <vector>

#include "debug_log.h"
#include "encoder/bitstream_file_writer.h"
#include "encoder/frame_encoder.h"
#include "encoder/nvenc_config.h"
#include "encoder/nvenc_session.h"
#include "graphics/d3d12_commands.h"
#include "graphics/d3d12_device.h"
#include "graphics/d3d12_frame_resources.h"
#include "graphics/d3d12_mesh.h"
#include "graphics/d3d12_pipeline.h"
#include "graphics/d3d12_swap_chain.h"
#include "try.h"

export module App;

constexpr auto BUFFER_COUNT			= 3u;
constexpr auto RENDER_TARGET_FORMAT = DXGI_FORMAT_B8G8R8A8_UNORM;
constexpr auto MVP_BUFFER_ALIGNMENT = 256u;

export class App {
	HWND hwnd;
	bool headless;
	uint32_t width;
	uint32_t height;
	D3D12Device device;
	NvencSession nvenc_session{*&device.device};
	EncoderConfig encoder_config{.codec		   = EncoderCodec::H264,
								 .preset	   = EncoderPreset::Fastest,
								 .rate_control = RateControlMode::VariableBitrate,
								 .width		   = width,
								 .height	   = height};
	NvencConfig nvenc_config{&nvenc_session, encoder_config};
	D3D12SwapChain swap_chain{*&device.device, *&device.factory, *&device.command_queue, hwnd,
							  SwapChainConfig{.buffer_count			= BUFFER_COUNT,
											  .render_target_format = RENDER_TARGET_FORMAT}};
	D3D12Pipeline pipeline{*&device.device, RENDER_TARGET_FORMAT};
	D3D12Mesh mesh{*&device.device};
	ComPtr<ID3D12CommandAllocator> allocator;
	ComPtr<ID3D12DescriptorHeap> offscreen_rtv_heap;
	uint32_t offscreen_rtv_descriptor_size;
	ComPtr<ID3D12Resource> mvp_constant_buffer;
	uint8_t* mvp_mapped = nullptr;
	D3D12FrameResources frames{*&device.device, BUFFER_COUNT, width, height, RENDER_TARGET_FORMAT};
	BitstreamFileWriter bitstream_writer{"output.h264"};
	FrameEncoder frame_encoder{nvenc_session, *&device.device, BUFFER_COUNT, width* height * 4 * 2};

  public:
	App(HWND hwnd, bool headless, uint32_t width, uint32_t height)
		: hwnd(hwnd), headless(headless), width(width), height(height) {
		Try
			| device.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
													IID_PPV_ARGS(&allocator));

		auto mvp_buffer_size = (UINT)((sizeof(MvpConstants) + MVP_BUFFER_ALIGNMENT - 1)
									  & ~(MVP_BUFFER_ALIGNMENT - 1));

		D3D12_HEAP_PROPERTIES cb_heap_properties{
			.Type = D3D12_HEAP_TYPE_UPLOAD,
		};

		D3D12_RESOURCE_DESC cb_resource_desc{
			.Dimension		  = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Width			  = mvp_buffer_size,
			.Height			  = 1,
			.DepthOrArraySize = 1,
			.MipLevels		  = 1,
			.SampleDesc		  = {.Count = 1},
			.Layout			  = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		};

		Try
			| device.device->CreateCommittedResource(
				&cb_heap_properties, D3D12_HEAP_FLAG_NONE, &cb_resource_desc,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mvp_constant_buffer));

		D3D12_RANGE range{.Begin = 0, .End = 0};
		Try | mvp_constant_buffer->Map(0, &range, (void**)&mvp_mapped);

		D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc{
			.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			.NumDescriptors = BUFFER_COUNT,
		};

		Try
			| device.device->CreateDescriptorHeap(&rtv_heap_desc,
												  IID_PPV_ARGS(&offscreen_rtv_heap));
		offscreen_rtv_descriptor_size
			= device.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		for (auto i = 0u; i < BUFFER_COUNT; ++i) {
			D3D12_CPU_DESCRIPTOR_HANDLE offscreen_rtv
				= offscreen_rtv_heap->GetCPUDescriptorHandleForHeapStart();
			offscreen_rtv.ptr += i * offscreen_rtv_descriptor_size;
			device.device->CreateRenderTargetView(frames.offscreen_render_targets[i], nullptr,
												  offscreen_rtv);
		}

		for (auto j = 0u; j < BUFFER_COUNT; ++j)
			frame_encoder.RegisterTexture(frames.offscreen_render_targets[j], width, height,
										  DxgiFormatToNvencFormat(RENDER_TARGET_FORMAT),
										  frames.fences[j]);

		for (auto k = 0u; k < BUFFER_COUNT; ++k) {
			D3D12_CPU_DESCRIPTOR_HANDLE cmd_rtv
				= offscreen_rtv_heap->GetCPUDescriptorHandleForHeapStart();
			cmd_rtv.ptr += k * offscreen_rtv_descriptor_size;

			frames.command_lists[k]->Reset(*&allocator, nullptr);
			RecordFrameCommandList(
				frames.command_lists[k], cmd_rtv, frames.offscreen_render_targets[k],
				*&swap_chain.render_targets[k], width, height, pipeline.GetRootSignature(),
				pipeline.GetPipelineState(), mvp_constant_buffer->GetGPUVirtualAddress(), mesh);
		}
	}

	~App() {
		if (mvp_constant_buffer && mvp_mapped) {
			mvp_constant_buffer->Unmap(0, nullptr);
			mvp_mapped = nullptr;
		}
	}

	int Run() && {
		bool running			   = true;
		uint32_t frames_submitted  = 0;
		uint32_t back_buffer_index = swap_chain.swap_chain->GetCurrentBackBufferIndex();
		HRESULT present_result	   = S_OK;
		auto last_frame_time	   = std::chrono::steady_clock::now();

		while (running) {
			auto now	= std::chrono::steady_clock::now();
			auto cpu_ms = std::chrono::duration<double, std::milli>(now - last_frame_time).count();
			last_frame_time = now;
			{
				auto stats = frame_encoder.GetStats();
				FRAME_LOG(
					"frame=%u cpu_ms=%.3f encoder_stats submitted=%llu completed=%llu "
					"pending=%llu waits=%llu",
					frames_submitted, cpu_ms, stats.submitted_frames, stats.completed_frames,
					stats.pending_frames, stats.wait_count);
			}
			if (frame_wait_coordinator.Wait(*this, frames_submitted, cpu_ms)
				== FrameLoopAction::Continue) {
				MSG msg{};
				while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
					if (msg.message == WM_QUIT) {
						running = false;
						break;
					}
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				continue;
			}

			uint64_t completed_value = 0;
			if (!IsFrameReady(frames, back_buffer_index, frames_submitted, completed_value))
				continue;
			FRAME_LOG("frame=%u cpu_ms=%.3f fence_completed_value=%llu", frames_submitted, cpu_ms,
					  completed_value);
			FRAME_LOG("frame=%u cpu_ms=%.3f present_result=%ld", frames_submitted, cpu_ms,
					  present_result);

			UpdateMvpConstants();

			auto signaled_value = frames_submitted + 1;

			if (SUCCEEDED(present_result)) {
				auto command_list_to_execute
					= (ID3D12CommandList*)frames.command_lists[back_buffer_index];
				device.command_queue->ExecuteCommandLists(1, &command_list_to_execute);
			}

			present_result = PresentAndSignal(*&device.command_queue, *&swap_chain.swap_chain,
											  frames, back_buffer_index, signaled_value);
			if (present_result == DXGI_ERROR_WAS_STILL_DRAWING) {
				FRAME_LOG("frame=%u cpu_ms=%.3f present=DXGI_ERROR_WAS_STILL_DRAWING skipping",
						  frames_submitted, cpu_ms);
				continue;
			}

			if (SUCCEEDED(present_result))
				frame_encoder.EncodeFrame(back_buffer_index, signaled_value, frames_submitted);

			auto new_back_buffer_index = swap_chain.swap_chain->GetCurrentBackBufferIndex();
			FRAME_LOG("frame=%u cpu_ms=%.3f frame_submitted fence_index=%u signal_value=%u",
					  frames_submitted, cpu_ms, back_buffer_index, signaled_value);
			FRAME_LOG("frame=%u cpu_ms=%.3f new_back_buffer_index=%u", frames_submitted, cpu_ms,
					  new_back_buffer_index);
			back_buffer_index = new_back_buffer_index;
			++frames_submitted;

			if (headless && frames_submitted >= 30)
				break;
		}

		DrainAndWait();
		return 0;
	}

  private:
	enum class FrameLoopAction {
		Continue,
		Proceed,
	};

	class FrameWaitCoordinator {
	  public:
		struct WaitableComponent {
			HANDLE handle;
			FrameLoopAction (App::*on_complete)();
		};

		FrameLoopAction Wait(App& app, uint32_t frames_submitted, double cpu_ms);
	};

	struct MvpConstants {
		float mvp[16];
	};

	FrameWaitCoordinator frame_wait_coordinator;

	FrameLoopAction OnFrameLatencyReady() {
		return FrameLoopAction::Proceed;
	}

	FrameLoopAction OnWriterReady() {
		bitstream_writer.DrainCompleted();
		return FrameLoopAction::Continue;
	}

	FrameLoopAction OnEncoderReady() {
		frame_encoder.ProcessCompletedFrames(bitstream_writer);
		return FrameLoopAction::Continue;
	}

	bool IsFrameReady(const D3D12FrameResources& resources, uint32_t back_buffer_index,
					  uint32_t frames_submitted, uint64_t& completed_value) {
		completed_value = resources.fences[back_buffer_index]->GetCompletedValue();
		return completed_value + resources.fences.size() >= frames_submitted + 1;
	}

	HRESULT PresentAndSignal(ID3D12CommandQueue* command_queue, IDXGISwapChain4* swap_chain_handle,
							 D3D12FrameResources& frame_resources, uint32_t back_buffer_index,
							 uint64_t signaled_value) {
		auto present_result = swap_chain_handle->Present(1, DXGI_PRESENT_DO_NOT_WAIT);
		Try | command_queue->Signal(frame_resources.fences[back_buffer_index], signaled_value)
			| frame_resources.fences[back_buffer_index]->SetEventOnCompletion(
				signaled_value, frame_resources.fence_events[back_buffer_index]);
		return present_result;
	}

	void DrainAndWait() {
		frame_encoder.ProcessCompletedFrames(bitstream_writer, true);
		WaitForMultipleObjects((DWORD)frames.fences.size(), frames.fence_events.data(), TRUE,
							   INFINITE);
		frame_encoder.ProcessCompletedFrames(bitstream_writer, true);
		auto stats = frame_encoder.GetStats();
		FRAME_LOG("encoder_drain submitted=%llu completed=%llu pending=%llu waits=%llu",
				  stats.submitted_frames, stats.completed_frames, stats.pending_frames,
				  stats.wait_count);
	}

	void UpdateMvpConstants();
};

App::FrameLoopAction App::FrameWaitCoordinator::Wait(App& app, uint32_t frames_submitted,
													 double cpu_ms) {
	WaitableComponent components[3];
	DWORD component_count = 0;

	components[component_count++] = WaitableComponent{
		.handle		 = app.swap_chain.frame_latency_waitable,
		.on_complete = &App::OnFrameLatencyReady,
	};

	if (app.bitstream_writer.HasPendingWrites()) {
		components[component_count++] = WaitableComponent{
			.handle		 = app.bitstream_writer.NextWriteEvent(),
			.on_complete = &App::OnWriterReady,
		};
	}

	if (app.frame_encoder.HasPendingOutputs()) {
		components[component_count++] = WaitableComponent{
			.handle		 = app.frame_encoder.NextOutputEvent(),
			.on_complete = &App::OnEncoderReady,
		};
	}

#ifndef ENABLE_FRAME_DEBUG_LOG
	(void)frames_submitted;
	(void)cpu_ms;
#endif
	FRAME_LOG("frame=%u cpu_ms=%.3f wait_for_frame begin", frames_submitted, cpu_ms);

	HANDLE handles[3];
	for (DWORD i = 0; i < component_count; ++i)
		handles[i] = components[i].handle;

	DWORD wait_result
		= MsgWaitForMultipleObjects(component_count, handles, FALSE, INFINITE, QS_ALLINPUT);
	FRAME_LOG("frame=%u cpu_ms=%.3f wait_for_frame result=%lu component_count=%lu",
			  frames_submitted, cpu_ms, wait_result, component_count);
	if (wait_result == WAIT_FAILED) {
		auto wait_error = GetLastError();
		FRAME_LOG("frame=%u cpu_ms=%.3f wait_for_frame failed error=%lu", frames_submitted,
				  cpu_ms, wait_error);
#ifndef ENABLE_FRAME_DEBUG_LOG
		(void)wait_error;
#endif
		throw;
	}

	if (wait_result >= WAIT_OBJECT_0 && wait_result < WAIT_OBJECT_0 + component_count) {
		DWORD component_index = wait_result - WAIT_OBJECT_0;
		auto on_complete	  = components[component_index].on_complete;
		return (app.*on_complete)();
	}
	return FrameLoopAction::Continue;
}

void App::UpdateMvpConstants() {
	if (!mvp_mapped)
		return;

	MvpConstants constants{
		.mvp = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f,
		},
	};

	memcpy(mvp_mapped, &constants, sizeof(constants));
}
