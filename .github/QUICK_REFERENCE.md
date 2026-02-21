# C++ Quick Reference - Must Follow

**Scan this before writing any code. Violations cause build failures or runtime crashes.**

## Naming Conventions
- **Methods/APIs**: `PascalCase` (e.g., `WaitForGpu`, `BeginFrame`) ✅ CI enforced
- **Variables**: `snake_case` without trailing `_` (e.g., `fence_event`, `device`) ✅ CI enforced  
- **Constants**: `CAPS_CASE` (e.g., `BUFFER_COUNT`, `MVP_BUFFER_ALIGNMENT`)

## RAII - Resource Management
- **Constructor/destructor only** - NO `Initialize()` or `Shutdown()` methods ✅ CI enforced
- All resource allocation in constructor; all cleanup in destructor
- Construction must fully succeed or throw - no two-phase initialization
- In destructors, avoid pointless container cleanup like `vector.clear()` when the container is about to be destroyed anyway
- ✓ Good: Resource acquired in constructor, released in destructor
- ✗ Bad: `Texture::Initialize()` called after construction

**Example** ([pipeline.cpp](../src/graphics/pipeline.cpp#L12-L39)):
```cpp
struct FileHandle {
	HANDLE handle = INVALID_HANDLE_VALUE;
	explicit FileHandle(HANDLE value) : handle(value) {}
	~FileHandle() { if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle); }
	// Move semantics, deleted copy...
};
```

## Error Handling
- **Use `Try |` pattern** for D3D12/NVENC calls that return HRESULT/status ⚠️ CI warns
- Chain calls: `Try | function1() | function2() | function3();`
- For HANDLEs: Direct `handle = call(); if (!handle) throw;`

## Struct Initialization
- **Designated initializers only** - never init-then-assign ❌ NOT CI-enforced
- ✓ Good: `Type var{.field = val, .other = val2};`
- ✗ Bad: `Type var{}; var.field = val; var.other = val2;`
- **Why**: Init-then-assign caused NVENC crash (wrong pointer type assignment)

## Member Initialization
- **Initialize inline in class body**, not constructor initializer list ❌ NOT CI-enforced
- ✓ Good: `ComPtr<ID3D12Device> device;` in class, init inline or in constructor body
- ✗ Bad: Moving initialization logic to constructor initializer list during refactors
- Constructor list: Only for parameter wiring (`width(width)`) and simple constructors

## ComPtr & Smart Pointers
- **ComPtr**: Use for ownership in members; pass raw pointers to functions ❌ NOT CI-enforced
- `ComPtr<ID3D12Device> device;` (owns) vs `void Func(ID3D12Device* device)` (borrows)
- **`*&com_ptr` pattern is intentional here**: WRL `ComPtr` overloads unary `operator&` and it is **not** the built-in object address operation. Keep `*&device.device` where used for borrowed `T*` flow in this repo; do not simplify it to `&device.device`.
- For COM out-params, use the explicit `ComPtr` address APIs (`GetAddressOf` / `ReleaseAndGetAddressOf`) based on whether release is desired.
- **NO `unique_ptr`/`shared_ptr`/`make_unique`** - use RAII with inline members

## Prohibited Patterns
- ✅ **C++ casts** (`static_cast`, etc.) - use C-style `(Type)value` instead
- ✅ **Namespaces** - keep all code in global namespace
- ❌ **Comments** - use self-documenting code with meaningful names
- ❌ **Threading** - no multi-threaded code or concurrency primitives
- ❌ **External libraries** - Windows API and C++ standard library only

## Style
- Format with `.clang-format` (tabs, 4-wide, 100-column) ✅ CI enforced
- Markdown files (`.md`): use spaces, not tabs
- Prefer `auto` when it avoids repeating the type
- Single-statement conditionals: omit braces; multi-statement: require braces
- Remove trivial wrappers; keep code minimal and direct

## Main Loop Waiting
- Prefer event-driven waits in steady-state main loops (`WaitFor*`/`MsgWaitForMultipleObjects`) instead of polling completion methods every iteration
- Call completion handlers like `ProcessCompletedFrames` in response to their signaled events whenever possible
- Poll only when no waitable signal path exists (or in explicit shutdown/drain flows)
- Keep steady-state CPU occupancy minimal: submit async work quickly (GPU command lists, overlapped writes), respond to signals promptly, and return to waiting

---

**Legend**: ✅ CI enforced = Build fails | ⚠️ CI warns = Warning only | ❌ NOT CI-enforced = Requires careful review
