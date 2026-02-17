# NVENC Crash Fix Summary

## Issue

**Symptom**: Access violation in `nvEncLockBitstream` when running the app
**Location**: `src/encoder/frame_encoder.cpp`, lines 128 and 136
**Severity**: Critical (application crash)

## Root Cause

The code was incorrectly passing a registered resource pointer directly to NVENC API functions that expect a `NV_ENC_OUTPUT_RESOURCE_D3D12` structure pointer when using D3D12.

### Incorrect Code (Before Fix)

```cpp
NV_ENC_LOCK_BITSTREAM lock_params{
    .version         = NV_ENC_LOCK_BITSTREAM_VER,
    .doNotWait       = false,
    .outputBitstream = output_registered_ptrs[texture_index],  // ❌ Wrong!
};
Try | session.nvEncLockBitstream(encoder, &lock_params);

// ...

Try | session.nvEncUnlockBitstream(encoder, output_registered_ptrs[texture_index]);  // ❌ Wrong!
```

### Correct Code (After Fix)

```cpp
NV_ENC_LOCK_BITSTREAM lock_params{
    .version         = NV_ENC_LOCK_BITSTREAM_VER,
    .doNotWait       = false,
    .outputBitstream = &output_resource,  // ✅ Correct!
};
Try | session.nvEncLockBitstream(encoder, &lock_params);

// ...

Try | session.nvEncUnlockBitstream(encoder, &output_resource);  // ✅ Correct!
```

## NVENC API Documentation

From `include/nvenc/nvEncodeAPI.h` (lines 2801-2816):

> **_NV_ENC_OUTPUT_RESOURCE_D3D12**  
> NV_ENC_PIC_PARAMS::outputBitstream and NV_ENC_LOCK_BITSTREAM::outputBitstream must be a pointer to a struct of this type, when D3D12 interface is used

The NVENC API requires that for D3D12:
- `NV_ENC_PIC_PARAMS::outputBitstream` → pointer to `NV_ENC_OUTPUT_RESOURCE_D3D12`
- `NV_ENC_LOCK_BITSTREAM::outputBitstream` → pointer to `NV_ENC_OUTPUT_RESOURCE_D3D12`

**Not** the registered resource pointer (`NV_ENC_INPUT_PTR`) returned by `nvEncRegisterResource`.

## Fix Details

### Changed Files
- `src/encoder/frame_encoder.cpp` (2 lines changed)

### Changes Made
1. Line 128: Changed `output_registered_ptrs[texture_index]` to `&output_resource`
2. Line 136: Changed `output_registered_ptrs[texture_index]` to `&output_resource`

The `output_resource` variable is a `NV_ENC_OUTPUT_RESOURCE_D3D12` structure that was already correctly constructed earlier in the function:

```cpp
NV_ENC_OUTPUT_RESOURCE_D3D12 output_resource{
    .version          = NV_ENC_OUTPUT_RESOURCE_D3D12_VER,
    .pOutputBuffer    = output_registered_ptrs[texture_index],  // Registered pointer goes here
    .outputFencePoint = output_fence_point,
};
```

This structure wraps the registered pointer and provides the fence synchronization information required by NVENC.

## Why This Caused a Crash

Passing the raw registered pointer instead of the structure pointer caused NVENC to:
1. Misinterpret the pointer as a structure address
2. Attempt to dereference invalid memory locations
3. Result in an access violation (0xC0000005)

The NVENC driver expected to read structure fields (version, buffer pointer, fence info) but instead found garbage data at the provided address.

## Prevention

To prevent similar issues:

1. **Always consult API documentation**: Check `nvEncodeAPI.h` for parameter requirements
2. **Use the Try | pattern**: Ensures all NVENC API calls are error-checked
3. **Test with debug layers**: D3D12 debug layer may catch some interop issues
4. **Run local tests**: Build and run headless locally to catch crashes early

**Note**: GitHub Actions workflows are temporarily removed and will be refactored back in eventually.

## Testing

The fix will be validated by:
1. Building the project successfully
2. Running the app in headless mode (30 frames)
3. Verifying no crashes occur during encoding
4. Checking that bitstream output is generated correctly

## References

- NVENC Programming Guide: https://docs.nvidia.com/video-technologies/video-codec-sdk/13.0/nvenc-video-encoder-api-prog-guide/
- Project NVENC guide: `.github/prompts/snippets/nvenc-guide.md`
- Known errors: `docs/copilot-known-errors.md` (now includes this fix)
- Bug triage system: `.github/BUG_TRIAGE_SYSTEM.md`

## Date

2026-02-17
