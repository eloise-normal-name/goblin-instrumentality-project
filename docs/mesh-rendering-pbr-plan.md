# Mesh Rendering and PBR Renderer Plan

This document outlines the plan for adding mesh rendering and a physically based renderer (PBR) to the Goblin Instrumentality Project using custom HLSL shaders on the existing D3D12 backend.

## Reference Renderers

Two well-documented renderers serve as primary inspiration for the material model and pipeline design.

### glTF 2.0 Default Renderer (Khronos)

The glTF 2.0 specification defines a metallic-roughness PBR workflow as its default material model. Key characteristics:

- **Base color** (albedo) as an RGBA factor and optional sRGB texture.
- **Metallic** scalar \[0,1\] controlling dielectric vs. conductor behavior.
- **Roughness** scalar \[0,1\] controlling microfacet spread.
- **Channel packing**: metallic in the blue channel, roughness in the green channel of a single linear texture.
- Additional maps: **normal**, **occlusion**, and **emissive**.
- Energy-conserving split between diffuse and specular contributions.

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

Filament also defines extended material models (clear coat, anisotropy, subsurface, cloth) that can serve as future stretch goals.

Reference: [Filament PBR Documentation](https://google.github.io/filament/main/filament.html), [Filament Materials Guide](https://google.github.io/filament/main/materials.html), [Filament Source](https://github.com/google/filament).

## Current State

The project currently:

- Creates a D3D12 device, swap chain, command allocator, and pre-recorded command lists.
- Clears offscreen render targets with solid colors and copies them to the swap chain.
- Encodes frames via NVENC using D3D12 interop.

There is no vertex/index buffer, no root signature, no pipeline state object, and no shader compilation. All rendering is done through `ClearRenderTargetView` and `CopyResource`.

## Mesh Data Source

Blender is the authoring tool for mesh assets. A custom Blender exporter will be developed to emit mesh data in a project-specific binary format optimized for direct GPU upload. Until the exporter is available, early phases use procedurally generated geometry (hardcoded triangles, cubes, spheres) to validate the rendering pipeline independently of the asset toolchain.

## Planned Architecture

### Pipeline Overview

```
Blender ──► Custom Exporter ──► Binary Mesh Data
                                        │
                                        ▼
Mesh Data ──► Upload Heap ──► Vertex/Index Buffers (DEFAULT heap)
                                        │
HLSL Shaders ──► Compiled Bytecode ─────┤
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

Transforms vertices from object space to clip space and passes interpolated attributes to the pixel shader:

```hlsl
cbuffer FrameConstants : register(b0) {
    float4x4 model_view_projection;
    float4x4 model;
    float4x4 normal_matrix;
    float3   camera_position;
    float    pad0;
    float3   light_direction;
    float    pad1;
    float3   light_color;
    float    light_intensity;
};

struct VSInput {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD0;
    float4 tangent  : TANGENT;
};

struct VSOutput {
    float4 position      : SV_POSITION;
    float3 world_pos     : WORLD_POSITION;
    float3 world_normal  : WORLD_NORMAL;
    float2 texcoord      : TEXCOORD0;
    float3 world_tangent : WORLD_TANGENT;
    float  tangent_sign  : TANGENT_SIGN;
};

VSOutput main(VSInput input) {
    VSOutput output;
    float4 world = mul(model, float4(input.position, 1.0));
    output.position      = mul(model_view_projection, float4(input.position, 1.0));
    output.world_pos     = world.xyz;
    output.world_normal  = mul((float3x3)normal_matrix, input.normal);
    output.texcoord      = input.texcoord;
    output.world_tangent = mul((float3x3)model, input.tangent.xyz);
    output.tangent_sign  = input.tangent.w;
    return output;
}
```

#### Pixel Shader (`mesh_ps.hlsl`)

Implements the PBR lighting model following glTF metallic-roughness and Filament's BRDF choices:

```hlsl
static const float PI = 3.14159265359;

Texture2D base_color_map         : register(t0);
Texture2D metallic_roughness_map : register(t1);
Texture2D normal_map             : register(t2);
Texture2D occlusion_map          : register(t3);
Texture2D emissive_map           : register(t4);
SamplerState linear_sampler      : register(s0);

// GGX normal distribution (Filament)
float D_GGX(float NdotH, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

// Smith GGX height-correlated visibility (Filament)
float V_SmithGGX(float NdotV, float NdotL, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;
    float v  = NdotL * sqrt(NdotV * NdotV * (1.0 - a2) + a2);
    float l  = NdotV * sqrt(NdotL * NdotL * (1.0 - a2) + a2);
    return 0.5 / max(v + l, 1e-5);
}

// Schlick Fresnel approximation (Filament / glTF)
float3 F_Schlick(float3 f0, float VdotH) {
    return f0 + (1.0 - f0) * pow(1.0 - VdotH, 5.0);
}
```

The pixel shader samples all PBR textures, constructs the TBN matrix from the interpolated tangent frame, perturbs the normal using the normal map, and evaluates the Cook-Torrance specular BRDF plus Lambertian diffuse for each light.

### Material Data

Following the glTF metallic-roughness model, a material is defined by:

```
struct MaterialParams {
    float4 base_color_factor;      // multiplied with base_color_map       (16 bytes, offset 0)
    float3 emissive_factor;        // multiplied with emissive_map         (12 bytes, offset 16)
    float  metallic_factor;        // multiplied with metallic channel     ( 4 bytes, offset 28)
    float  roughness_factor;       // multiplied with roughness channel    ( 4 bytes, offset 32)
    float  occlusion_strength;     // scales occlusion_map contribution    ( 4 bytes, offset 36)
    float  normal_scale;           // scales normal_map perturbation       ( 4 bytes, offset 40)
    uint   texture_flags;          // bitmask indicating which textures    ( 4 bytes, offset 44)
};
```

The struct is ordered for HLSL constant buffer packing: `float4` first, then `float3` packed with a trailing `float` to fill the 16-byte row, followed by remaining scalars.

When a texture is not present, the shader falls back to the scalar factors alone (e.g., `base_color_factor` as a flat color). This mirrors the glTF specification behavior.

### Depth Buffer

A `DXGI_FORMAT_D32_FLOAT` depth-stencil texture is created per back buffer with a corresponding DSV descriptor heap. The depth buffer is cleared at the start of each frame and used during the draw call.

### Shader Compilation

HLSL shaders are compiled offline using `dxc.exe` (bundled with the Windows SDK) to DXIL bytecode. The compiled `.cso` files are embedded as byte arrays in header files, avoiding runtime file I/O and external dependencies:

```
dxc /T vs_6_0 /E main /Fo mesh_vs.cso mesh_vs.hlsl
dxc /T ps_6_0 /E main /Fo mesh_ps.cso mesh_ps.hlsl
```

A build step in CMake invokes `dxc` and generates headers containing the bytecode arrays. The PSO creation code references these arrays directly.

## Implementation Phases

### Phase 1: Hardcoded Triangle

Goal: Render a single colored triangle using the D3D12 graphics pipeline.

- Define a minimal vertex struct (position + color, no PBR yet).
- Create a root signature with a single root CBV for the MVP matrix.
- Create a PSO with a simple pass-through vertex shader and flat-color pixel shader.
- Allocate a vertex buffer on the upload heap (3 vertices).
- Replace `ClearRenderTargetView` draw path with `IASetVertexBuffers` and `DrawInstanced`.
- Validate output on screen and through NVENC encode path.

### Phase 2: Mesh Loading and Depth

Goal: Render an indexed mesh with depth testing.

- Define the full `Vertex` struct (position, normal, texcoord, tangent).
- Create vertex and index buffers on DEFAULT heap with upload-heap staging.
- Add a depth buffer and DSV descriptor heap.
- Load a hardcoded mesh (e.g., a cube or sphere generated procedurally in code).
- Add a perspective camera with a constant buffer for the MVP matrix.
- Validate depth-correct rendering.
- Design the binary mesh format that the future Blender exporter will target (vertex layout, index format, material references).

### Phase 3: Diffuse Lighting

Goal: Add basic Lambertian diffuse shading with a single directional light.

- Write the vertex shader with world-space position and normal output.
- Write a pixel shader that evaluates `max(NdotL, 0) * base_color / pi` for a single directional light.
- Pass base color and light direction/color as scalar constants in the frame constant buffer (no textures).
- Validate lit shading on the procedural mesh (light vs. shadow side visible).

### Phase 4: Specular BRDF (Cook-Torrance)

Goal: Add specular reflection using the Cook-Torrance microfacet model.

- Implement `D_GGX` (normal distribution function) in the pixel shader.
- Implement `V_SmithGGX` (height-correlated visibility) in the pixel shader.
- Implement `F_Schlick` (Fresnel approximation) in the pixel shader.
- Combine diffuse + specular: `f = f_diffuse * (1 - F) * (1 - metallic) + f_specular`.
- Add metallic and roughness scalar factors to the constant buffer.
- Validate specular highlights and metallic vs. dielectric appearance on the procedural mesh.

### Phase 5: Material Constant Buffer

Goal: Consolidate material parameters into a dedicated constant buffer.

- Define `MaterialParams` struct (base_color_factor, metallic_factor, roughness_factor, emissive_factor, occlusion_strength, normal_scale, texture_flags).
- Add a per-material root CBV (b1) alongside the per-frame CBV (b0).
- Update the pixel shader to read material factors from the material constant buffer.
- Validate that material parameters can be varied per-object.

### Phase 6: Base Color Texture

Goal: Bind a base color texture to the pixel shader.

- Create a shader-visible CBV/SRV/UAV descriptor heap.
- Load a base color texture from raw pixel data embedded in code (procedurally generated or manually parsed).
- Add a descriptor table (t0) and static sampler (s0) to the root signature.
- Sample and multiply with `base_color_factor` in the pixel shader.
- Validate textured diffuse output.

### Phase 7: Metallic-Roughness Texture

Goal: Add the packed metallic-roughness map.

- Load a metallic-roughness texture (green = roughness, blue = metallic).
- Bind to register t1 in the existing descriptor table.
- Sample and multiply with scalar metallic/roughness factors in the pixel shader.
- Validate per-texel roughness and metallic variation on the procedural mesh.

### Phase 8: Normal, Occlusion, and Emissive Maps

Goal: Complete the PBR texture set.

- Add normal map (t2): construct the TBN matrix from interpolated tangent frame, perturb shading normal, scale by `normal_scale`.
- Add occlusion map (t3): modulate ambient/indirect term by `occlusion_strength`.
- Add emissive map (t4): add `emissive_factor * emissive_map` to final color.
- Use `texture_flags` bitmask to fall back to scalar factors when a map is not bound.
- Validate full PBR texture pipeline.

### Phase 9: Scene and Camera

Goal: Interactive camera and multiple objects.

- Add a perspective projection with configurable FOV and aspect ratio.
- Add an orbit camera controlled by window messages (mouse drag).
- Support multiple meshes with per-object model matrices (instanced constant buffer).
- Add multiple lights (point and directional) packed into the frame constant buffer.

## Constraints

All implementation must follow the existing project rules:

- **No external libraries**: Mesh data is procedural initially; a custom Blender exporter (future work) will provide production assets in a project-specific binary format. Textures are embedded or generated in code.
- **RAII**: All GPU resources (buffers, heaps, PSOs, root signatures) allocated in constructors, released in destructors.
- **Naming**: PascalCase methods, snake_case variables, CAPS_CASE constants.
- **Error handling**: `Try |` for D3D12 API calls returning HRESULT; null-check for HANDLE returns.
- **No namespaces, no smart pointers, no C++ casts**.
- **Static linking** with `/MT` runtime.
- **Shader compilation**: Offline via `dxc`, bytecode embedded in headers.

## File Structure

Planned new files under `src/`:

```
src/
  shaders/
    mesh_vs.hlsl              Vertex shader source
    mesh_ps.hlsl              Pixel shader source
  graphics/
    d3d12_mesh.h              Mesh class (vertex/index buffers, draw)
    d3d12_mesh.cpp
    d3d12_pipeline.h          Root signature + PSO creation
    d3d12_pipeline.cpp
    d3d12_depth_buffer.h      Depth buffer + DSV heap
    d3d12_depth_buffer.cpp
  generated/
    mesh_vs.h                 Compiled vertex shader bytecode
    mesh_ps.h                 Compiled pixel shader bytecode
```
