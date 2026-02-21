module;

#include <windows.h>

#include <chrono>
#include <cstring>
#include <utility>
#include <vector>

#include "app_logging.h"
#include "debug_log.h"
#include "encoder/bitstream_file_writer.h"
#include "encoder/frame_encoder.h"
#include "encoder/nvenc_session.h"
#include "graphics/device.h"
#include "graphics/frame_resources.h"
#include "graphics/mesh.h"
#include "graphics/pipeline.h"
#include "graphics/swap_chain.h"
#include "try.h"

export module App;

constexpr auto BUFFER_COUNT			= 3u;
constexpr auto RENDER_TARGET_FORMAT = DXGI_FORMAT_B8G8R8A8_UNORM;
constexpr auto MVP_BUFFER_ALIGNMENT = 256u;

struct MvpConstantBuffer {
	struct MvpConstants {
		float mvp[16];
	}
	static constexpr  IDENTITY{
			.mvp = {
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f,
			},
		};

	ComPtr<ID3D12Resource> resource;
	uint8_t* mapped = nullptr;

	explicit MvpConstantBuffer(ID3D12Device* device) {
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
			| device->CreateCommittedResource(&cb_heap_properties, D3D12_HEAP_FLAG_NONE,
											  &cb_resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ,
											  nullptr, IID_PPV_ARGS(&resource));

		D3D12_RANGE range{.Begin = 0, .End = 0};
		Try | resource->Map(0, &range, (void**)&mapped);
	}

	~MvpConstantBuffer() {
		if (resource && mapped) {
			resource->Unmap(0, nullptr);
			mapped = nullptr;
		}
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const {
		return resource->GetGPUVirtualAddress();
	}

	void WriteIdentity() {
		if (!mapped)
			return;

		memcpy(mapped, &IDENTITY, sizeof(IDENTITY));
	}
};

struct Renderer {
	D3D12Device& d;

	MvpConstantBuffer mvp_constant_buffer{*&d.device};
	D3D12FrameResources frames{*&d.device, BUFFER_COUNT};
	D3D12Pipeline pipeline{*&d.device, RENDER_TARGET_FORMAT};
	D3D12Mesh mesh{*&d.device};

	Renderer(D3D12Device& d) : d(d) {
	}

	void WriteToCommandList(ID3D12GraphicsCommandList* command_list,
							D3D12_CPU_DESCRIPTOR_HANDLE rtv, uint32_t width, uint32_t height) {
		command_list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

		D3D12_VIEWPORT viewport{.TopLeftX = 0.0f,
								.TopLeftY = 0.0f,
								.Width	  = (float)width,
								.Height	  = (float)height,
								.MinDepth = 0.0f,
								.MaxDepth = 1.0f};
		D3D12_RECT scissor{.left = 0, .top = 0, .right = (LONG)width, .bottom = (LONG)height};
		command_list->RSSetViewports(1, &viewport);
		command_list->RSSetScissorRects(1, &scissor);

		command_list->SetGraphicsRootSignature(pipeline.GetRootSignature());
		command_list->SetPipelineState(pipeline.GetPipelineState());
		command_list->SetGraphicsRootConstantBufferView(0,
														mvp_constant_buffer.GetGpuVirtualAddress());

		float clear_color[]{0.0f, 0.0f, 0.0f, 1.0f};
		command_list->ClearRenderTargetView(rtv, clear_color, 0, nullptr);

		mesh.Draw(command_list);
	}
};

export class App {
	HWND hwnd;
	bool headless;
	uint32_t width;
	uint32_t height;
	D3D12Device device;
	EncoderConfig encoder_config{.codec		   = EncoderCodec::H264,
								 .preset	   = EncoderPreset::Fastest,
								 .rate_control = RateControlMode::VariableBitrate,
								 .width		   = width,
								 .height	   = height};
	NvencSession nvenc_session{*&device.device, encoder_config};
	D3D12SwapChain swap_chain{*&device.device, *&device.factory, *&device.command_queue, hwnd,
							  SwapChainConfig{.buffer_count			= BUFFER_COUNT,
											  .render_target_format = RENDER_TARGET_FORMAT}};
	Renderer renderer{device};
	ComPtr<ID3D12DescriptorHeap> offscreen_rtv_heap;
	uint32_t offscreen_rtv_descriptor_size;
	ComPtr<ID3D12CommandAllocator> allocator;
	RenderTextureArray offscreen_render_targets{*&device.device, BUFFER_COUNT, width, height,
												RENDER_TARGET_FORMAT};
	BitstreamFileWriter bitstream_writer{"output.h264"};
	FrameEncoder frame_encoder{nvenc_session, *&device.device, BUFFER_COUNT,
							   width * height * 4 * 2};

  public:
	App(HWND hwnd, bool headless, uint32_t width, uint32_t height)
		: hwnd(hwnd), headless(headless), width(width), height(height) {
		D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc{
			.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			.NumDescriptors = BUFFER_COUNT,
		};
		Try
			| device.device->CreateDescriptorHeap(&rtv_heap_desc,
												  IID_PPV_ARGS(&offscreen_rtv_heap));
		offscreen_rtv_descriptor_size
			= device.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		Try
			| device.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
													IID_PPV_ARGS(&allocator));

		for (auto j = 0u; j < BUFFER_COUNT; ++j)
			frame_encoder.RegisterTexture(*&offscreen_render_targets.textures[j], width, height,
										  DxgiFormatToNvencFormat(RENDER_TARGET_FORMAT),
										  renderer.frames.fences[j]);

		auto offscreen_rtv_for = [this](uint32_t index) {
			auto rtv = offscreen_rtv_heap->GetCPUDescriptorHandleForHeapStart();
			rtv.ptr += index * offscreen_rtv_descriptor_size;
			return rtv;
		};

		auto record_frame_command_list
			= [this](uint32_t index, D3D12_CPU_DESCRIPTOR_HANDLE cmd_rtv) {
				  auto command_list				= renderer.frames.command_lists[index];
				  auto render_target			= *&offscreen_render_targets.textures[index];
				  auto swap_chain_render_target = *&swap_chain.render_targets[index];

				  auto apply_transition_barriers = [command_list](auto... transitions) {
					  D3D12_RESOURCE_BARRIER barriers[]{{
						  .Type		  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
						  .Transition = transitions,
					  }...};
					  command_list->ResourceBarrier((UINT)sizeof...(transitions), barriers);
				  };

				  command_list->Reset(*&allocator, nullptr);
				  apply_transition_barriers(D3D12_RESOURCE_TRANSITION_BARRIER{
					  .pResource   = render_target,
					  .StateBefore = D3D12_RESOURCE_STATE_COMMON,
					  .StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET,
				  });

				  renderer.WriteToCommandList(command_list, cmd_rtv, this->width, this->height);

				  apply_transition_barriers(
					  D3D12_RESOURCE_TRANSITION_BARRIER{
						  .pResource   = render_target,
						  .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
						  .StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE,
					  },
					  D3D12_RESOURCE_TRANSITION_BARRIER{
						  .pResource   = swap_chain_render_target,
						  .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
						  .StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST,
					  });

				  command_list->CopyResource(swap_chain_render_target, render_target);

				  apply_transition_barriers(
					  D3D12_RESOURCE_TRANSITION_BARRIER{
						  .pResource   = swap_chain_render_target,
						  .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
						  .StateAfter  = D3D12_RESOURCE_STATE_PRESENT,
					  },
					  D3D12_RESOURCE_TRANSITION_BARRIER{
						  .pResource   = render_target,
						  .StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE,
						  .StateAfter  = D3D12_RESOURCE_STATE_COMMON,
					  });

				  command_list->Close();
			  };

		for (auto k = 0u; k < BUFFER_COUNT; ++k) {
			auto cmd_rtv = offscreen_rtv_for(k);
			device.device->CreateRenderTargetView(*&offscreen_render_targets.textures[k], nullptr,
												  cmd_rtv);
			record_frame_command_list(k, cmd_rtv);
		}
	}

	int Run() && {
		bool running			   = true;
		uint32_t frames_submitted  = 0;
		uint32_t back_buffer_index = swap_chain.swap_chain->GetCurrentBackBufferIndex();
		HRESULT present_result	   = S_OK;
		auto last_frame_time	   = std::chrono::steady_clock::now();

		while (running) {
			auto frame_log = AppLogging::BuildFrameLogContext(frames_submitted, last_frame_time);
			AppLogging::LogFrameLoopStart(frame_log, frame_encoder.GetStats());

			if (frame_wait_coordinator.Wait(*this, frame_log.frame, frame_log.cpu_ms)
				== FrameLoopAction::Continue) {
				PumpMessages(running);
				continue;
			}

			uint64_t completed_value = 0;
			if (!IsFrameReady(renderer.frames, back_buffer_index, frame_log.frame, completed_value))
				continue;

			AppLogging::LogFenceCompletion(frame_log, completed_value);
			AppLogging::LogPresentStatus(frame_log, present_result);

			renderer.mvp_constant_buffer.WriteIdentity();

			auto signaled_value = frame_log.frame + 1;

			if (SUCCEEDED(present_result)) {
				auto command_list_to_execute
					= (ID3D12CommandList*)renderer.frames.command_lists[back_buffer_index];
				device.command_queue->ExecuteCommandLists(1, &command_list_to_execute);
			}

			present_result = PresentAndSignal(*&device.command_queue, *&swap_chain.swap_chain,
											  renderer.frames, back_buffer_index, signaled_value);
			if (present_result == DXGI_ERROR_WAS_STILL_DRAWING) {
				AppLogging::LogPresentStillDrawing(frame_log);
				continue;
			}

			if (SUCCEEDED(present_result))
				frame_encoder.EncodeFrame(back_buffer_index, signaled_value, frame_log.frame);

			auto new_back_buffer_index = swap_chain.swap_chain->GetCurrentBackBufferIndex();
			AppLogging::LogFrameSubmitResult(frame_log, back_buffer_index, signaled_value,
											 new_back_buffer_index);
			back_buffer_index = new_back_buffer_index;
			++frames_submitted;

			if (headless && frames_submitted >= 500)
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
		enum class WaitableComponent {
			FrameLatency,
			BitstreamWrite,
			EncoderOutput,
		};

		FrameLoopAction Wait(App& app, uint32_t frames_submitted, double cpu_ms);

	  private:
		static FrameLoopAction Complete(App& app, WaitableComponent component);
	};

	FrameWaitCoordinator frame_wait_coordinator;

	void PumpMessages(bool& running) const {
		for (MSG msg{}; PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);) {
			if (msg.message == WM_QUIT) {
				running = false;
				return;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
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
		WaitForMultipleObjects((DWORD)renderer.frames.fences.size(),
							   renderer.frames.fence_events.data(), TRUE, INFINITE);
		frame_encoder.ProcessCompletedFrames(bitstream_writer, true);
		auto stats = frame_encoder.GetStats();
		FRAME_LOG("encoder_drain submitted=%llu completed=%llu pending=%llu waits=%llu",
				  stats.submitted_frames, stats.completed_frames, stats.pending_frames,
				  stats.wait_count);
	}
};

App::FrameLoopAction App::FrameWaitCoordinator::Complete(App& app, WaitableComponent component) {
	switch (component) {
		case WaitableComponent::FrameLatency:
			return FrameLoopAction::Proceed;
		case WaitableComponent::BitstreamWrite:
			app.bitstream_writer.DrainCompleted();
			return FrameLoopAction::Continue;
		case WaitableComponent::EncoderOutput:
			app.frame_encoder.ProcessCompletedFrames(app.bitstream_writer);
			return FrameLoopAction::Continue;
	}
	throw;
}

App::FrameLoopAction App::FrameWaitCoordinator::Wait(App& app, uint32_t frames_submitted,
													 double cpu_ms) {
	HANDLE handles[3];
	WaitableComponent components[3];
	DWORD component_count = 0;

	auto add_waitable
		= [&handles, &components, &component_count](HANDLE handle, WaitableComponent component) {
			  handles[component_count]	  = handle;
			  components[component_count] = component;
			  ++component_count;
		  };

	add_waitable(app.swap_chain.frame_latency_waitable, WaitableComponent::FrameLatency);

	if (app.bitstream_writer.HasPendingWrites())
		add_waitable(app.bitstream_writer.NextWriteEvent(), WaitableComponent::BitstreamWrite);

	if (app.frame_encoder.HasPendingOutputs())
		add_waitable(app.frame_encoder.NextOutputEvent(), WaitableComponent::EncoderOutput);

#ifndef ENABLE_FRAME_DEBUG_LOG
	(void)frames_submitted;
	(void)cpu_ms;
#endif
	FRAME_LOG("frame=%u cpu_ms=%.3f wait_for_frame begin", frames_submitted, cpu_ms);

	DWORD wait_result
		= MsgWaitForMultipleObjects(component_count, handles, FALSE, INFINITE, QS_ALLINPUT);
	FRAME_LOG("frame=%u cpu_ms=%.3f wait_for_frame result=%lu component_count=%lu",
			  frames_submitted, cpu_ms, wait_result, component_count);
	if (wait_result == WAIT_FAILED) {
		auto wait_error = GetLastError();
		FRAME_LOG("frame=%u cpu_ms=%.3f wait_for_frame failed error=%lu", frames_submitted, cpu_ms,
				  wait_error);
#ifndef ENABLE_FRAME_DEBUG_LOG
		(void)wait_error;
#endif
		throw;
	}

	if (wait_result >= WAIT_OBJECT_0 && wait_result < WAIT_OBJECT_0 + component_count) {
		auto component_index = wait_result - WAIT_OBJECT_0;
		return Complete(app, components[component_index]);
	}
	return FrameLoopAction::Continue;
}
