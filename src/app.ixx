module;

#include <windows.h>

#include <cstring>
#include <utility>
#include <vector>

#include "encoder/bitstream_file_writer.h"
#include "encoder/frame_encoder.h"
#include "encoder/nvenc_config.h"
#include "encoder/nvenc_session.h"
#include "frame_debug_log.h"
#include "graphics/d3d12_device.h"
#include "graphics/d3d12_mesh.h"
#include "graphics/d3d12_pipeline.h"
#include "graphics/d3d12_swap_chain.h"
#include "try.h"

export module App;

constexpr auto BUFFER_COUNT			= 3u;
constexpr auto RENDER_TARGET_FORMAT = DXGI_FORMAT_B8G8R8A8_UNORM;
constexpr auto MVP_BUFFER_ALIGNMENT = 256u;

export class App {
	class FrameResources {
	  public:
		FrameResources(ID3D12Device* device, uint32_t count, uint32_t width, uint32_t height,
					   DXGI_FORMAT format);
		~FrameResources();

		std::vector<ID3D12GraphicsCommandList*> command_lists;
		std::vector<ID3D12Fence*> fences;
		std::vector<HANDLE> fence_events;
		std::vector<ID3D12Resource*> offscreen_render_targets;
	};
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
	FrameResources frames{*&device.device, BUFFER_COUNT, width, height, RENDER_TARGET_FORMAT};
	BitstreamFileWriter bitstream_writer{"output.h264"};
	FrameEncoder frame_encoder{nvenc_session, *&device.device, BUFFER_COUNT,
							   width * height * 4 * 2};
	FrameDebugLog frame_debug_log{"debug_output.txt"};

  public:
	App(HWND hwnd, bool headless, uint32_t width, uint32_t height)
		: hwnd(hwnd), headless(headless), width(width), height(height) {
		Try | device.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
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

		Try | device.device->CreateCommittedResource(
				  &cb_heap_properties, D3D12_HEAP_FLAG_NONE, &cb_resource_desc,
				  D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mvp_constant_buffer));

		D3D12_RANGE range{.Begin = 0, .End = 0};
		Try | mvp_constant_buffer->Map(0, &range, (void**)&mvp_mapped);

		D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc{
			.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			.NumDescriptors = BUFFER_COUNT,
		};

		Try | device.device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&offscreen_rtv_heap));
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
			RecordCommandList(frames.command_lists[k], cmd_rtv, frames.offscreen_render_targets[k],
							  *&swap_chain.render_targets[k]);
		}
	}

	~App() {
		if (mvp_constant_buffer && mvp_mapped) {
			mvp_constant_buffer->Unmap(0, nullptr);
			mvp_mapped = nullptr;
		}
	}

	int Run() && {
		MSG msg{};
		bool running			   = true;
		uint32_t frames_submitted  = 0;
		uint32_t back_buffer_index = swap_chain.swap_chain->GetCurrentBackBufferIndex();
		HRESULT present_result	   = S_OK;

		while (running) {
			frame_debug_log.BeginFrame(frames_submitted);
			frame_encoder.ProcessCompletedFrames(bitstream_writer);
			{
				auto stats = frame_encoder.GetStats();
				frame_debug_log.Line()
					<< "encoder_stats submitted=" << stats.submitted_frames
					<< " completed=" << stats.completed_frames
					<< " pending=" << stats.pending_frames << " waits=" << stats.wait_count << "\n";
			}
			auto wait_result
				= WaitForFrame(frame_debug_log, swap_chain.frame_latency_waitable, msg);
			if (wait_result == FrameWaitResult::Continue) {
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
			LogFenceStatus(frame_debug_log, completed_value, present_result);

			UpdateMvpConstants();

			auto signaled_value = frames_submitted + 1;

			if (SUCCEEDED(present_result)) {
				auto command_list_to_execute
					= (ID3D12CommandList*)frames.command_lists[back_buffer_index];
				device.command_queue->ExecuteCommandLists(1, &command_list_to_execute);
			}

			present_result = PresentAndSignal(*&device.command_queue, *&swap_chain.swap_chain,
											  frames, back_buffer_index, signaled_value);
			if (!HandlePresentResult(frame_debug_log, present_result))
				continue;

			if (SUCCEEDED(present_result))
				frame_encoder.EncodeFrame(back_buffer_index, signaled_value, frames_submitted);
			frame_encoder.ProcessCompletedFrames(bitstream_writer);

			auto new_back_buffer_index = swap_chain.swap_chain->GetCurrentBackBufferIndex();
			LogFrameSubmitted(frame_debug_log, back_buffer_index, signaled_value,
							  new_back_buffer_index);
			back_buffer_index = new_back_buffer_index;
			++frames_submitted;

			if (headless && frames_submitted >= 30)
				break;
		}

		frame_encoder.ProcessCompletedFrames(bitstream_writer, true);
		WaitForMultipleObjects((DWORD)frames.fences.size(), frames.fence_events.data(), TRUE,
							   INFINITE);
		frame_encoder.ProcessCompletedFrames(bitstream_writer, true);
		{
			auto stats = frame_encoder.GetStats();
			frame_debug_log.Line()
				<< "encoder_drain submitted=" << stats.submitted_frames
				<< " completed=" << stats.completed_frames << " pending=" << stats.pending_frames
				<< " waits=" << stats.wait_count << "\n";
		}
		return 0;
	}

  private:
	enum class FrameWaitResult {
		Continue,
		Proceed,
	};

	struct MvpConstants {
		float mvp[16];
	};

	void RecordCommandList(ID3D12GraphicsCommandList* command_list, D3D12_CPU_DESCRIPTOR_HANDLE rtv,
						   ID3D12Resource* offscreen_render_target,
						   ID3D12Resource* swap_chain_render_target) {
		{
			D3D12_RESOURCE_BARRIER barrier{
				.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Transition = {.pResource	= offscreen_render_target,
							   .StateBefore = D3D12_RESOURCE_STATE_COMMON,
							   .StateAfter	= D3D12_RESOURCE_STATE_RENDER_TARGET}};
			command_list->ResourceBarrier(1, &barrier);
		}

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
		command_list->SetGraphicsRootConstantBufferView(
			0, mvp_constant_buffer->GetGPUVirtualAddress());

		float clear_color[]{0.0f, 0.0f, 0.0f, 1.0f};
		command_list->ClearRenderTargetView(rtv, clear_color, 0, nullptr);

		mesh.Draw(command_list);

		{
			D3D12_RESOURCE_BARRIER barrier{
				.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Transition = {.pResource	= offscreen_render_target,
							   .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
							   .StateAfter	= D3D12_RESOURCE_STATE_COPY_SOURCE}};
			command_list->ResourceBarrier(1, &barrier);
		}

		{
			D3D12_RESOURCE_BARRIER barrier{
				.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Transition = {.pResource	= swap_chain_render_target,
							   .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
							   .StateAfter	= D3D12_RESOURCE_STATE_COPY_DEST}};
			command_list->ResourceBarrier(1, &barrier);
		}

		command_list->CopyResource(swap_chain_render_target, offscreen_render_target);

		{
			D3D12_RESOURCE_BARRIER barrier{
				.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Transition = {.pResource	= swap_chain_render_target,
							   .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
							   .StateAfter	= D3D12_RESOURCE_STATE_PRESENT}};
			command_list->ResourceBarrier(1, &barrier);
		}

		{
			D3D12_RESOURCE_BARRIER barrier{
				.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Transition = {.pResource	= offscreen_render_target,
							   .StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE,
							   .StateAfter	= D3D12_RESOURCE_STATE_COMMON}};
			command_list->ResourceBarrier(1, &barrier);
		}

		command_list->Close();
	}

	FrameWaitResult WaitForFrame(FrameDebugLog& log, HANDLE frame_latency_waitable, MSG&) {
		log.Line() << "WaitForFrame..." << "\n";

		auto wait_result
			= MsgWaitForMultipleObjects(1, &frame_latency_waitable, FALSE, INFINITE, QS_ALLINPUT);
		log.Line() << "Wait result: " << wait_result << "\n";

		if (wait_result != WAIT_OBJECT_0 + 1)
			return FrameWaitResult::Proceed;

		return FrameWaitResult::Continue;
	}

	bool IsFrameReady(const FrameResources& resources, uint32_t back_buffer_index,
					  uint32_t frames_submitted, uint64_t& completed_value) {
		completed_value = resources.fences[back_buffer_index]->GetCompletedValue();
		return completed_value + resources.fences.size() >= frames_submitted + 1;
	}

	void LogFenceStatus(FrameDebugLog& log, uint64_t completed_value, HRESULT present_result) {
		log.Line() << "Completed Value: " << completed_value << "\n";
		log.Line() << "present_result: " << present_result << "\n";
	}

	HRESULT PresentAndSignal(ID3D12CommandQueue* command_queue, IDXGISwapChain4* swap_chain_handle,
							 FrameResources& frame_resources, uint32_t back_buffer_index,
							 uint64_t signaled_value) {
		auto present_result = swap_chain_handle->Present(1, DXGI_PRESENT_DO_NOT_WAIT);
		command_queue->Signal(frame_resources.fences[back_buffer_index], signaled_value);
		frame_resources.fences[back_buffer_index]->SetEventOnCompletion(
			signaled_value, frame_resources.fence_events[back_buffer_index]);
		return present_result;
	}

	bool HandlePresentResult(FrameDebugLog& log, HRESULT present_result) {
		if (present_result != DXGI_ERROR_WAS_STILL_DRAWING)
			return true;

		log.Line() << "Present returned DXGI_ERROR_WAS_STILL_DRAWING, skipping..." << "\n";
		return false;
	}

	void LogFrameSubmitted(FrameDebugLog& log, uint32_t back_buffer_index, uint64_t signaled_value,
						   uint32_t new_back_buffer_index) {
		log.Line() << "Frame submitted, fence[" << back_buffer_index
				   << "] signaled with value: " << signaled_value << "\n";
		log.Line() << "new back_buffer_index: " << new_back_buffer_index << "\n";
	}

	void UpdateMvpConstants();
};

App::FrameResources::FrameResources(ID3D12Device* device, uint32_t count, uint32_t width,
									uint32_t height, DXGI_FORMAT format) {
	command_lists.resize(count);
	fences.resize(count);
	fence_events.resize(count);
	offscreen_render_targets.resize(count);

	ComPtr<ID3D12Device4> device4;
	Try | device->QueryInterface(IID_PPV_ARGS(&device4));

	D3D12_HEAP_PROPERTIES default_heap_props{
		.Type = D3D12_HEAP_TYPE_DEFAULT,
	};

	D3D12_RESOURCE_DESC texture_desc{
		.Dimension		  = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		.Width			  = width,
		.Height			  = height,
		.DepthOrArraySize = 1,
		.MipLevels		  = 1,
		.Format			  = format,
		.SampleDesc		  = {.Count = 1},
		.Flags			  = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
	};

	D3D12_CLEAR_VALUE clear_value{
		.Format = format,
		.Color	= {0, 0, 0, 1},
	};

	for (auto i = 0u; i < count; ++i) {
		fence_events[i] = CreateEvent(nullptr, FALSE, TRUE, nullptr);
		if (!fence_events[i])
			throw;

		ID3D12GraphicsCommandList1* command_list1 = nullptr;
		Try | device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fences[i]))
			| device4->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
										  D3D12_COMMAND_LIST_FLAG_NONE,
										  IID_PPV_ARGS(&command_list1))
			| device->CreateCommittedResource(&default_heap_props, D3D12_HEAP_FLAG_NONE,
											  &texture_desc, D3D12_RESOURCE_STATE_COMMON,
											  &clear_value,
											  IID_PPV_ARGS(&offscreen_render_targets[i]));
		command_lists[i] = (ID3D12GraphicsCommandList*)command_list1;
	}
}

App::FrameResources::~FrameResources() {
	for (auto command_list : command_lists)
		if (command_list)
			command_list->Release();
	command_lists.clear();

	for (auto fence : fences)
		if (fence)
			fence->Release();
	fences.clear();

	for (auto offscreen_render_target : offscreen_render_targets)
		if (offscreen_render_target)
			offscreen_render_target->Release();
	offscreen_render_targets.clear();

	for (auto fence_event : fence_events)
		if (fence_event)
			CloseHandle(fence_event);
	fence_events.clear();
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
