NVENC GUIDE (SDK 13.0) — D3D12-ONLY, COMPRESSED

SCOPE
- Use NVENCODE C API (nvEncodeAPI.h) via nvEncodeAPI.dll (LoadLibrary).
- Devices: D3D12 only. No CUDA/OpenGL notes.
- OS: Windows 10+; D3D12 requires Windows 10 20H1+.

FLOW (1→9)
1) Open session
   - NvEncodeAPICreateInstance → NvEncOpenEncodeSessionEx (device=IUnknown*, deviceType=NV_ENC_DEVICE_TYPE_DIRECTX).
2) Select codec/preset/profile/input fmt
   - NvEncGetEncodeGUIDs → encodeGUID
   - NvEncGetEncodePresetGUIDs → presetGUID (tuningInfo)
   - NvEncGetEncodeProfileGUIDs → profileGUID
   - NvEncGetInputFormats → NV_ENC_BUFFER_FORMAT
3) Query capabilities
   - NvEncGetEncodeCaps with NV_ENC_CAPS_PARAM::capsToQuery
4) Initialize encoder
   - NvEncInitializeEncoder with NV_ENC_INITIALIZE_PARAMS + NV_ENC_CONFIG
   - Required: encodeGUID, encodeWidth/Height (and other session params)
5) Allocate IO buffers (D3D12)
   - Input: CreateCommittedResource(HEAP_TYPE_DEFAULT, DIMENSION_TEXTURE2D)
   - Output: CreateCommittedResource(HEAP_TYPE_READBACK, DIMENSION_BUFFER)
   - Output size (H264/HEVC/AV1): 2 * input YUV size
   - Allocate >= (1 + NB) input/output buffers (NB = B frames between P frames)
6) Register/Map external resources
   - NV_ENC_REGISTER_RESOURCE → NvEncRegisterResource → registeredResource
   - NvEncMapInputResource → mappedResource
   - Use mappedResource in NV_ENC_PIC_PARAMS
   - NvEncUnmapInputResource, NvEncUnregisterResource before destroy
7) Encode
   - NvEncEncodePicture with NV_ENC_PIC_PARAMS (+ codec-specific params)
   - D3D12: pass NV_ENC_INPUT_RESOURCE_D3D12* in inputBuffer and
     NV_ENC_OUTPUT_RESOURCE_D3D12* in outputBuffer
8) Retrieve output
   - NvEncLockBitstream → CPU ptr + size → copy/process → NvEncUnlockBitstream
   - D3D12: pass same NV_ENC_OUTPUT_RESOURCE_D3D12* in NV_ENC_LOCK_BITSTREAM::outputBitstream
9) End
   - EOS: NvEncEncodePicture with encodePicFlags=NV_ENC_FLAGS_EOS, rest 0
   - Destroy buffers (unlock first), then NvEncDestroyEncoder
   - Unregister events and unmap any resources before close

D3D12 SYNC
- NvEncRegisterResource accepts fence points:
  - NV_ENC_REGISTER_RESOURCE_PARAMS_D3D12::pInputFencePoint / pOutputFencePoint
  - NVENC waits on input fence; signals output fence when done.
- During encode, NVENC waits on NV_ENC_INPUT_RESOURCE_D3D12::inputFencePoint
  and signals NV_ENC_OUTPUT_RESOURCE_D3D12::outputFencePoint.

PER-FRAME CONTROLS
- Force I: NV_ENC_PIC_PARAMS::encodePicFlags = NV_ENC_PIC_FLAG_FORCEINTRA
- Force IDR: NV_ENC_PIC_FLAG_FORCEIDR
- Output SPS/PPS or AV1 seq header: NV_ENC_PIC_FLAG_OUTPUT_SPSPPS
- Ref frame: NV_ENC_PIC_PARAMS_*::refPicFlag = 1

MODE OF OPERATION
- Async: enableEncodeAsync=1 (Windows 10+ only); register events
- Sync: enableEncodeAsync=0; NvEncLockBitstream blocks (doNotWait=0)
- Threading: keep encode thread non-blocking; process output on secondary thread

D3D12 LIMITS / INCOMPATIBLES
- Motion-estimation-only mode: NOT supported with D3D12
- Weighted prediction: NOT supported with D3D12
- Output-in-vidmem: supported for D3D11/CUDA only (not D3D12)

RATE CONTROL (RC)
- CBR: averageBitRate required; optionally lowDelayKeyFrameScale
- VBR: averageBitRate + maxBitRate recommended
- CONSTQP: use rcParams.constQP
- Target quality: VBR + targetQuality (H264/HEVC: 0–51; AV1: 0–63)
- Multipass: disabled, 2-pass quarter/full (quality vs perf tradeoff)

ADVANCED FEATURES (D3D12-OK unless noted)
- Lookahead: enableLookahead + lookaheadDepth; may return NV_ENC_ERR_NEED_MORE_INPUT
- B-frames as reference: useBFramesAsRef (caps check)
- Reconfigure: NvEncReconfigureEncoder (cannot change GOP, async/sync, bit-depth, chroma, maxEncodeWxH, enablePTD, etc.)
- AQ: enableAQ / enableTemporalAQ (caps check)
- High bit depth: set profile GUID + input/output bit depths (caps check)
- LTR: enableLTR + ltrNumFrames + ltrMarkFrame / ltrUseFrameBitmap; not with B-frames
- Emphasis map: qpMapMode=NV_ENC_QP_MAP_EMPHASIS (not with AQ)
- Alpha layer (HEVC): enableAlphaLayerEncoding (not with 10-bit, vidmem output, weighted pred)
- Temporal SVC: enableTemporalSVC + layer counts; not with B-frames
- Error resiliency: NvEncInvalidateRefFrames; intraRefreshPeriod/intraRefreshCnt
- Split frame encoding (HEVC/AV1): improves speed, reduces quality; incompatible with weighted pred, alpha layer, subframe readback, vidmem output
- Recon frame output: enableReconFrameOutput + register recon buffer
- Output stats: enableOutputStats + outputStatsLevel; set outputStatsPtr/Size in NvEncLockBitstream
- Iterative encoding: numStateBuffers + DISABLE_ENC_STATE_ADVANCE + NvEncRestoreEncoderState
- External lookahead: enableExtLookahead + NvEncLookaheadPicture
- Unidirectional B: enableUniDirectionalB (HEVC)
- Lookahead level: enableLookahead + lookaheadLevel
- Temporal filter (HEVC): tfLevel (caps check)
- MV-HEVC: enableMVHEVC; not with LTR/Alpha/B/Lookahead/TemporalFilter/Split/2-pass/non-HQ tuning
- HDR10/HDR10+: outputMaxCll/outputMasteringDisplay; ITU-T T.35 SEI/OBU payloads
- External ME hints: enableExternalMEHints + meExternalHints/Counts

RECOMMENDED SETTINGS (HIGH-LEVEL)
- Recording/Archiving: HQ/UHQ + VBR + big VBV + B-frames + Lookahead + B-ref + finite GOP + AQ
- Game-casting/transcoding: HQ/UHQ + CBR + medium VBV + B-frames + Lookahead + B-ref + finite GOP + AQ
- Low-latency: LL/ULL + CBR + very low VBV + uni B + infinite GOP + AQ + LTR + intra refresh + non-ref P + force IDR
- Lossless: Lossless tuning info

MEMORY REDUCTION
- Avoid B-frames, large ref counts, 2-pass, AQ/weighted pred, lookahead, temporal filter, UHQ

PERF NOTES
- Keep pipeline on GPU; avoid PCIe transfers; use enough IO buffers
- Split Frame Encoding can shift bottleneck to decoder