# Mesh Rendering and PBR Renderer Plan

This document outlines the plan for adding mesh rendering and a physically based renderer (PBR) to the Goblin Instrumentality Project using custom HLSL shaders on the existing D3D12 backend.

## Reference Renderers

Two well-documented renderers serve as primary inspiration for the material model and pipeline design.

### glTF 2.0 Default Renderer (Khronos)

The glTF 2.0 specification defines a metallic-roughness PBR workflow as its default material model:

- Base color (RGBA factor + optional sRGB texture), metallic \[0,1\], roughness \[0,1\].
- Channel-packed metallic (blue) / roughness (green) in a single linear texture.
- Additional maps: normal, occlusion, emissive. Energy-conserving diffuse/specular split.

Reference: [glTF 2.0 Specification — Materials](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#materials), [Khronos PBR Overview](https://www.khronos.org/gltf/pbr).

### Google Filament Renderer

Filament is a real-time PBR engine that implements the Cook-Torrance microfacet BRDF with carefully chosen term functions:

| BRDF Term | Function | Purpose |
|-----------|----------|---------|
| **D** (Normal Distribution) | GGX / Trowbridge-Reitz | Microfacet orientation; realistic highlight tails |
| **G** (Geometry / Visibility) | Smith GGX height-correlated | Self-shadowing and masking attenuation |
| **F** (Fresnel) | Schlick approximation | Increased reflectance at grazing angles |

The full specular BRDF:

```
f_specular = (D * G * F) / (4 * NdotL * NdotV)
```

The diffuse term uses a Lambertian model: `f_diffuse = base_color / pi`.

Filament also defines extended material models (clear coat, anisotropy, subsurface, cloth) as potential future stretch goals.

Reference: [Filament PBR Documentation](https://google.github.io/filament/main/filament.html), [Filament Source](https://github.com/google/filament).

## Current State

The project currently:

- Creates a D3D12 device, swap chain, command allocator, and pre-recorded command lists.
- Clears offscreen render targets with solid colors and copies them to the swap chain.
- Encodes frames via NVENC using D3D12 interop.

There is no vertex/index buffer, no root signature, no pipeline state object, and no shader compilation. All rendering is done through `ClearRenderTargetView` and `CopyResource`.

## Mesh Data Source

Blender is the authoring tool for mesh assets. A custom Blender exporter will be developed to emit mesh data in a project-specific binary format optimized for direct GPU upload. Until the exporter is available, early phases use procedurally generated geometry (hardcoded triangles, cubes, spheres) to validate the rendering pipeline independently of the asset toolchain.

## Components Overview

The mesh rendering and PBR pipeline requires the following high-level components:

| Component | Responsibility |
|-----------|---------------|
| **Mesh** | Owns vertex and index buffers on the GPU. Provides draw commands (`IASetVertexBuffers`, `IASetIndexBuffer`, `DrawIndexedInstanced`). |
| **Pipeline** | Encapsulates root signature, compiled shaders, and PSO. Loads `.hlsl` files at runtime, compiles via `D3DCompile`, caches bytecode to disk, creates the pipeline state. |
| **Material** | Holds per-material constant buffer data (base color, metallic, roughness, emissive, occlusion, normal scale) and texture SRV bindings. Bound to the pipeline before each draw call via root CBV and descriptor table. |
| **Depth Buffer** | Per-frame `D32_FLOAT` depth-stencil texture and DSV heap. Cleared each frame, enables correct occlusion for overlapping geometry. |
| **Camera** | Computes view and projection matrices from position, target, FOV, and aspect ratio. Updated per-frame and written to the frame constant buffer (b0). |
| **Lights** | Directional and point light parameters packed into the frame constant buffer. Consumed by the pixel shader for diffuse and specular evaluation. |
| **Frame Constants** | Per-frame constant buffer (b0) containing MVP matrix, model/normal matrices, camera position, and light data. Mapped to an upload heap and updated each frame. |

These components interact during each frame as follows: the camera and lights update the frame constant buffer, the pipeline is set on the command list, each mesh binds its material and issues draw calls, and the depth buffer resolves occlusion.

## Planned Architecture

### Pipeline Overview

```
Blender ──► Custom Exporter ──► Binary Mesh Data
                                        │
                                        ▼
Mesh Data ──► Upload Heap ──► Vertex/Index Buffers (DEFAULT heap)
                                        │
HLSL Files ──► D3DCompile ──► Bytecode ──┤
                  ▲                     │
                  │               Cache (disk)
                  │                     │
                                        ▼
                            Root Signature + PSO
                                        │
                    ┌───────────────────►│
                    │                    ▼
              CBV (per-frame)    Input Assembler
              ┌───────────┐            │
              │ MVP matrix │            ▼
              │ camera pos │     Vertex Shader
              │ light data │            │
              └───────────┘            ▼
                                 Rasterizer
                                        │
              SRV heap ────────► Pixel Shader (PBR)
              ┌───────────┐            │
              │ base color │            ▼
              │ metallic/  │     Render Target
              │ roughness  │            │
              │ normal map │            ▼
              │ occlusion  │     Copy to Swap Chain
              │ emissive   │            │
              └───────────┘            ▼
                                 Present + Encode
```

### Mesh Rendering Pipeline (D3D12)

#### Vertex Format

A single interleaved vertex struct matching the PBR input requirements:

```
struct Vertex {
    float3 position;   // POSITION
    float3 normal;     // NORMAL
    float2 texcoord;   // TEXCOORD0
    float4 tangent;    // TANGENT (xyz = tangent direction, w = handedness)
};
```

Stride: 48 bytes per vertex.

#### Root Signature

The root signature binds GPU resources to shader registers:

| Slot | Type | Register | Content |
|------|------|----------|---------|
| 0 | Root CBV | b0 | Per-frame constants (MVP, camera position, light data) |
| 1 | Descriptor Table | t0-t4 | PBR texture SRVs (base color, metallic-roughness, normal, occlusion, emissive) |
| 2 | Static Sampler | s0 | Trilinear wrap sampler |

The root signature allows input assembler input layout (`D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT`).

#### Pipeline State Object (PSO)

A single `D3D12_GRAPHICS_PIPELINE_STATE_DESC` containing:

- Vertex shader and pixel shader bytecode.
- Input layout matching the `Vertex` struct.
- Root signature pointer.
- Render target format (`DXGI_FORMAT_B8G8R8A8_UNORM`, matching current `RENDER_TARGET_FORMAT`).
- Depth-stencil format (`DXGI_FORMAT_D32_FLOAT`).
- Rasterizer state: solid fill, back-face culling, clockwise front face.
- Blend state: opaque (no alpha blending initially).
- Depth-stencil state: depth test enabled, depth write enabled, less-equal comparison.
- Primitive topology: triangle list.

#### Resource Buffers

| Buffer | Heap Type | Purpose |
|--------|-----------|---------|
| Vertex buffer | DEFAULT | Per-mesh vertex data, uploaded once via upload heap |
| Index buffer | DEFAULT | Per-mesh index data (uint16 or uint32) |
| Constant buffer | UPLOAD | Per-frame MVP matrix, camera position, light parameters |
| Depth buffer | DEFAULT | `DXGI_FORMAT_D32_FLOAT` texture, one per back buffer |

Upload heaps are used as staging; vertex and index data are copied to DEFAULT heap resources via the command list and discarded after the initial upload.

### HLSL Shaders

#### Vertex Shader (`mesh_vs.hlsl`)

Transforms vertices from object space to clip space and passes world-space position, normal, texcoord, and tangent frame to the pixel shader. Reads a per-frame constant buffer (b0) containing the MVP matrix, model matrix, normal matrix, camera position, and light parameters.

#### Pixel Shader (`mesh_ps.hlsl`)

Implements the Cook-Torrance specular BRDF (GGX normal distribution, Smith height-correlated visibility, Schlick Fresnel) plus Lambertian diffuse, following [Filament's PBR equations](https://google.github.io/filament/main/filament.html) and the [glTF 2.0 metallic-roughness model](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#materials). Samples up to five PBR textures (base color, metallic-roughness, normal, occlusion, emissive) and falls back to scalar material factors when textures are not bound.

### Material Data

Following the glTF metallic-roughness model, a material is defined by a constant buffer containing: base color factor (float4), emissive factor (float3), metallic factor, roughness factor, occlusion strength, normal scale, and a texture presence bitmask. The struct is ordered for HLSL constant buffer packing alignment (float4 first, float3 packed with a trailing float, then remaining scalars).

When a texture is not present, the shader falls back to the scalar factors alone (e.g., `base_color_factor` as a flat color). This mirrors the [glTF specification behavior](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#materials).

### Depth Buffer

A `DXGI_FORMAT_D32_FLOAT` depth-stencil texture is created per back buffer with a corresponding DSV descriptor heap. The depth buffer is cleared at the start of each frame and used during the draw call.

### Shader Compilation

HLSL shaders are loaded from `.hlsl` files at runtime and compiled using `d3dcompiler_47.dll` via the `D3DCompile` function. At startup, the pipeline reads each shader source file from disk, calls `D3DCompile` with the appropriate target profile (`vs_5_0`, `ps_5_0`) and entry point to produce bytecode blobs, and passes them to `CreateGraphicsPipelineState`. The `d3dcompiler_47.dll` ships with Windows and is loaded via `#pragma comment(lib, "d3dcompiler.lib")`.

Compiled bytecode is cached to disk (e.g., `mesh_vs.cso`, `mesh_ps.cso`). On launch, the pipeline checks the `.cso` timestamp against the `.hlsl` source: if valid, load cached bytecode; if stale, recompile and update. Fast startup in production, rapid iteration in development.

## Implementation Phases

### Phase 1: Hardcoded Triangle

**Goal**: Render a single colored triangle using the D3D12 graphics pipeline.
- Minimal vertex struct (position + color, no PBR yet), root signature with one root CBV (MVP).
- PSO with pass-through VS and flat-color PS; vertex buffer on upload heap (3 vertices).
- Replace `ClearRenderTargetView` path with `IASetVertexBuffers` + `DrawInstanced`.
- **Validate**: Output on screen and through NVENC encode path.

### Phase 2: Mesh Loading and Depth

**Goal**: Render an indexed mesh with depth testing.
- Full `Vertex` struct (position, normal, texcoord, tangent); DEFAULT heap buffers with upload staging.
- Depth buffer + DSV descriptor heap; perspective camera with MVP constant buffer.
- Hardcoded procedural mesh (cube or sphere); design binary mesh format for the Blender exporter.
- **Validate**: Depth-correct rendering.

### Phase 3: Diffuse Lighting

**Goal**: Lambertian diffuse with one directional light.
- VS: world-space position + normal output.
- PS: `max(NdotL, 0) * base_color / pi` for a single directional light.
- Frame CBV: base color, light direction/color (no textures yet).
- **Validate**: Light/shadow side visible on procedural mesh.

### Phase 4: Specular BRDF (Cook-Torrance)

**Goal**: Add specular reflection using the Cook-Torrance microfacet model.
- PS: implement `D_GGX`, `V_SmithGGX`, `F_Schlick` individually.
- Combine: `f = f_diffuse * (1 - F) * (1 - metallic) + f_specular`.
- Add metallic/roughness scalars to the constant buffer.
- **Validate**: Specular highlights, metallic vs. dielectric appearance.

### Phase 5: Material Constant Buffer

**Goal**: Consolidate material parameters into a dedicated constant buffer.
- `MaterialParams` struct: base_color_factor, metallic, roughness, emissive, occlusion_strength, normal_scale, texture_flags.
- Per-material root CBV (b1) alongside per-frame CBV (b0).
- **Validate**: Per-object material variation.

### Phase 6: Base Color Texture

**Goal**: Bind a base color texture to the pixel shader.
- CBV/SRV/UAV descriptor heap; base color texture from embedded pixel data.
- Descriptor table (t0) + static sampler (s0) in root signature.
- PS: sample and multiply with `base_color_factor`.
- **Validate**: Textured diffuse output.

### Phase 7: Metallic-Roughness Texture

**Goal**: Add the packed metallic-roughness map.
- Load metallic-roughness texture (green = roughness, blue = metallic) at t1.
- PS: sample and multiply with scalar factors.
- **Validate**: Per-texel roughness/metallic variation.

### Phase 8: Normal, Occlusion, and Emissive Maps

**Goal**: Complete the PBR texture set.
- Normal map (t2): TBN matrix from tangent frame, perturb normal, scale by `normal_scale`.
- Occlusion map (t3): modulate ambient term by `occlusion_strength`.
- Emissive map (t4): add `emissive_factor * emissive_map` to final color.
- `texture_flags` bitmask for fallback to scalar factors when maps are unbound.
- **Validate**: Full PBR texture pipeline.

### Phase 9: Scene and Camera

**Goal**: Interactive camera and multiple objects.
- Perspective projection with configurable FOV/aspect ratio; orbit camera via window messages.
- Multiple meshes with per-object model matrices (instanced constant buffer).
- Multiple lights (point + directional) packed into the frame constant buffer.

## Constraints

All implementation must follow the existing project rules:

- **No external libraries**: Mesh data is procedural initially; the Blender exporter (see Mesh Data Source) will provide production assets. Textures are embedded or generated in code.
- **RAII**: All GPU resources (buffers, heaps, PSOs, root signatures) allocated in constructors, released in destructors.
- **Naming**: PascalCase methods, snake_case variables, CAPS_CASE constants.
- **Error handling**: `Try |` for D3D12 API calls returning HRESULT; null-check for HANDLE returns.
- **No namespaces, no smart pointers, no C++ casts**.
- **Static linking** with `/MT` runtime.
- **Shader compilation**: Runtime via `D3DCompile` from `d3dcompiler_47.dll`; shader source loaded from `.hlsl` files; compiled bytecode cached to `.cso` files on disk.

## File Structure

Planned new files under `src/`:

```
src/
  shaders/
    mesh_vs.hlsl              Vertex shader source (loaded at runtime)
    mesh_ps.hlsl              Pixel shader source (loaded at runtime)
  graphics/
    d3d12_mesh.h              Mesh class (vertex/index buffers, draw)
    d3d12_mesh.cpp
    d3d12_pipeline.h          Root signature, PSO creation, shader compilation + cache
    d3d12_pipeline.cpp
    d3d12_depth_buffer.h      Depth buffer + DSV heap
    d3d12_depth_buffer.cpp
    d3d12_camera.h            Camera (view/projection matrices)
    d3d12_camera.cpp
    d3d12_material.h          Material parameters + texture bindings
    d3d12_material.cpp
```
