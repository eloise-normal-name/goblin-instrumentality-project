#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <cstdint>

using Microsoft::WRL::ComPtr;

struct DeviceConfig {
    HWND window_handle;
    uint32_t frame_width;
    uint32_t frame_height;
    uint32_t buffer_count;
    DXGI_FORMAT render_target_format;
};

class D3D12Device {
   public:
    D3D12Device() = default;
    ~D3D12Device();

    D3D12Device(const D3D12Device&) = delete;
    D3D12Device& operator=(const D3D12Device&) = delete;
    D3D12Device(D3D12Device&&) = delete;
    D3D12Device& operator=(D3D12Device&&) = delete;

    bool Initialize(const DeviceConfig& config);
    void Shutdown();

    void WaitForGpu();
    void MoveToNextFrame();

    ID3D12Device* GetDevice() const { return device.Get(); }
    ID3D12CommandQueue* GetCommandQueue() const { return command_queue.Get(); }
    ID3D12CommandAllocator* GetCurrentCommandAllocator() const;
    IDXGISwapChain4* GetSwapChain() const { return swap_chain.Get(); }

    ID3D12Resource* GetCurrentRenderTarget() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;

    uint32_t GetCurrentFrameIndex() const { return current_frame_index; }
    uint32_t GetBufferCount() const { return buffer_count; }
    uint64_t GetCurrentFenceValue() const { return fence_values[current_frame_index]; }

   private:
    bool CreateDevice();
    bool CreateCommandQueue();
    bool CreateSwapChain(HWND window_handle, uint32_t width, uint32_t height);
    bool CreateDescriptorHeaps();
    bool CreateRenderTargets();
    bool CreateCommandAllocators();
    bool CreateFence();

    static constexpr uint32_t MAX_BUFFER_COUNT = 3;

    ComPtr<IDXGIFactory7> factory;
    ComPtr<IDXGIAdapter4> adapter;
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> command_queue;
    ComPtr<IDXGISwapChain4> swap_chain;
    ComPtr<ID3D12DescriptorHeap> rtv_heap;
    ComPtr<ID3D12Resource> render_targets[MAX_BUFFER_COUNT];
    ComPtr<ID3D12CommandAllocator> command_allocators[MAX_BUFFER_COUNT];
    ComPtr<ID3D12Fence> fence;
    HANDLE fence_event = nullptr;

    uint64_t fence_values[MAX_BUFFER_COUNT] = {};
    uint32_t current_frame_index = 0;
    uint32_t buffer_count = 2;
    uint32_t rtv_descriptor_size = 0;
    DXGI_FORMAT render_target_format = DXGI_FORMAT_R8G8B8A8_UNORM;
};
