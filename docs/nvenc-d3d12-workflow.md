# NVENC D3D12 Encoding Workflow

This document describes the proper D3D12 NVENC encoding workflow as implemented in this codebase.

## Overview

NVENC encoding with D3D12 requires creating and managing D3D12 resources for both input and output buffers, then registering them with NVENC. This is different from CUDA or other non-D3D12 workflows.

## Key Principle

**DO NOT use `nvEncCreateBitstreamBuffer` for D3D12 workflows.** This API is for non-D3D12 backends (CUDA, etc.). For D3D12, you must create D3D12 buffers and register them with NVENC.

## Implementation Steps

### 1. Create D3D12 Output Buffers

Create readback buffers for encoded output:

```cpp
D3D12_HEAP_PROPERTIES readback_heap{
    .Type = D3D12_HEAP_TYPE_READBACK,
};

D3D12_RESOURCE_DESC buffer_desc{
    .Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER,
    .Width            = output_buffer_size,  // width * height * 4 * 2 for H.264/HEVC
    .Height           = 1,
    .DepthOrArraySize = 1,
    .MipLevels        = 1,
    .Format           = DXGI_FORMAT_UNKNOWN,
    .SampleDesc       = {.Count = 1, .Quality = 0},
    .Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
};

ID3D12Resource* output_buffer = nullptr;
device->CreateCommittedResource(
    &readback_heap,
    D3D12_HEAP_FLAG_NONE,
    &buffer_desc,
    D3D12_RESOURCE_STATE_COPY_DEST,
    nullptr,
    IID_PPV_ARGS(&output_buffer)
);
```

**Buffer Size:** For H.264/HEVC/AV1, allocate `2 * input_size` where input_size is the uncompressed frame size (width × height × bytes_per_pixel).

### 2. Register D3D12 Buffers with NVENC

Register the created D3D12 buffers:

```cpp
NV_ENC_REGISTER_RESOURCE register_params{
    .version            = NV_ENC_REGISTER_RESOURCE_VER,
    .resourceType       = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX,
    .width              = output_buffer_size,
    .height             = 1,
    .resourceToRegister = output_buffer,
    .bufferFormat       = NV_ENC_BUFFER_FORMAT_U8,
    .bufferUsage        = NV_ENC_OUTPUT_BITSTREAM,
};

session.nvEncRegisterResource(encoder, &register_params);
NV_ENC_REGISTERED_PTR registered_ptr = register_params.registeredResource;
```

### 3. Use D3D12-Specific Structures for Encoding

When encoding, wrap pointers in D3D12-specific structures:

```cpp
// Input resource with fence synchronization
NV_ENC_FENCE_POINT_D3D12 input_fence_point{
    .version   = NV_ENC_FENCE_POINT_D3D12_VER,
    .pFence    = d3d12_fence,
    .waitValue = fence_value_to_wait_on,
    .bWait     = 1,
};

NV_ENC_INPUT_RESOURCE_D3D12 input_resource{
    .version          = NV_ENC_INPUT_RESOURCE_D3D12_VER,
    .pInputBuffer     = mapped_input_ptr,
    .inputFencePoint  = input_fence_point,
};

// Output resource
NV_ENC_OUTPUT_RESOURCE_D3D12 output_resource{
    .version          = NV_ENC_OUTPUT_RESOURCE_D3D12_VER,
    .pOutputBuffer    = registered_output_ptr,
    .outputFencePoint = {}, // Optional output fence
};

// Encode parameters
NV_ENC_PIC_PARAMS pic_params{
    .version         = NV_ENC_PIC_PARAMS_VER,
    .inputWidth      = width,
    .inputHeight     = height,
    .inputPitch      = width * bytes_per_pixel,
    .inputBuffer     = &input_resource,  // Pointer to structure, not raw ptr
    .outputBitstream = &output_resource, // Pointer to structure, not raw ptr
    .bufferFmt       = NV_ENC_BUFFER_FORMAT_ARGB,
    .pictureStruct   = NV_ENC_PIC_STRUCT_FRAME,
    .inputTimeStamp  = frame_index,
};

session.nvEncEncodePicture(encoder, &pic_params);
```

### 4. Lock and Retrieve Bitstream

Lock the bitstream to access encoded data:

```cpp
NV_ENC_LOCK_BITSTREAM lock_params{
    .version         = NV_ENC_LOCK_BITSTREAM_VER,
    .outputBitstream = registered_output_ptr,  // Registered pointer, not D3D12 structure
    .doNotWait       = false,
};

session.nvEncLockBitstream(encoder, &lock_params);

// Access encoded data
void* bitstream_data = lock_params.bitstreamBufferPtr;
uint32_t bitstream_size = lock_params.bitstreamSizeInBytes;

// Process/write data...

session.nvEncUnlockBitstream(encoder, registered_output_ptr);
```

### 5. Cleanup

Unregister resources before destroying encoder:

```cpp
// Unregister NVENC resources
session.nvEncUnregisterResource(encoder, registered_ptr);

// Release D3D12 resources
output_buffer->Release();
```

## Fence Synchronization

The `NV_ENC_INPUT_RESOURCE_D3D12` structure includes a fence point that tells NVENC to wait for GPU work to complete before encoding:

```cpp
NV_ENC_FENCE_POINT_D3D12 input_fence_point{
    .version   = NV_ENC_FENCE_POINT_D3D12_VER,
    .pFence    = fence,           // D3D12 fence object
    .waitValue = signaled_value,  // Value to wait for
    .bWait     = 1,               // Enable waiting
};
```

This ensures the encoder waits for rendering to complete before encoding the frame.

## Common Mistakes

1. **Using `nvEncCreateBitstreamBuffer`**: This is wrong for D3D12. Always create D3D12 buffers and register them.

2. **Passing raw pointers to `NV_ENC_PIC_PARAMS`**: Must use `NV_ENC_INPUT_RESOURCE_D3D12` and `NV_ENC_OUTPUT_RESOURCE_D3D12` structures.

3. **Wrong buffer type**: Output buffers must be `HEAP_TYPE_READBACK`, not `DEFAULT`.

4. **Insufficient buffer size**: Use at least `2 * uncompressed_frame_size` for output buffers.

## Implementation Reference

See `src/encoder/frame_encoder.cpp` for the complete implementation following this workflow.

## References

- NVENC Programming Guide: https://docs.nvidia.com/video-technologies/video-codec-sdk/13.0/nvenc-video-encoder-api-prog-guide/
- Project NVENC guide: `.github/prompts/snippets/nvenc-guide.md`
