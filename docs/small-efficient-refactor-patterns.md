# The Goblin Guide to Small & Efficient Code

*Living notes for feral, hyper-pragmatic refactors that reduce code size, clear away bloat, and keep the runtime fast.*

## Quick Reference (The Goblin Rules)

1. [Drop the Wrappers](#1-drop-the-wrappers) - Collapse pass-through abstractions.
2. [Sort the Clutter by Lifetime](#2-sort-the-clutter-by-lifetime) - Split resources by lifecycle, not convenience.
3. [Pack the Bag](#3-pack-the-bag) - Extract cohesive behavior into narrow local types.
4. [Point, Don't Shout](#4-point-dont-shout) - Use enum dispatch instead of callback tables.
5. [Build Tools Once](#5-build-tools-once) - Pre-record immutable state during setup.

## How to Maintain the Stash

- Add only patterns proven by real, tested changes in this repo.
- Keep each pattern to: intent, tiny example shape, and when *not* to use it.
- Prefer removing abstractions over building new ones.

## The Patterns

### 1) Drop the Wrappers

*Why have a middle-manager when you can just talk to the API?*

- **Intent:** Remove helper classes that only mirror a few underlying API calls.
- **Example shape:** Delete generic `CommandWrapper` or `AllocatorHelper` classes. Keep one direct function or an inline sequence of native API calls.
- **Benefit:** Fewer symbols, fewer indirections, easier state reasoning. The code does exactly what it says.
- **Avoid when:** The wrapper encodes non-trivial invariants or complex state tracking used in multiple call sites.

### 2) Sort the Clutter by Lifetime

*Keep the permanent fixtures separate from the temporary trash.*

- **Intent:** Separate synchronization objects from resource ownership.
- **Example shape:** Instead of one massive `FrameContext` struct, have a `FrameSync` struct (fences, events) and a `TextureArray` struct (render targets).
- **Benefit:** Each type has one job and clearer constructor contracts. When the frame dies, the sync dies, but the textures might live on.
- **Avoid when:** The split introduces duplicated setup logic with no actual gain in ownership clarity.

### 3) Pack the Bag

*Give a function one job and a clear path.*

- **Intent:** Gather a cohesive sequence (like pipeline + mesh + constant-buffer drawing) into one narrow, local unit.
- **Example shape:** A `Renderer::WriteToCommandList(...)` method that owns the specific draw-path details, rather than scattering draw calls across the main app loop.
- **Benefit:** The main app loop stays focused on orchestration. The draw path is isolated and easy to scan.
- **Avoid when:** The extraction creates a "god object" spanning unrelated responsibilities.

### 4) Point, Don't Shout

*Keep it simple. Just point at what you want.*

- **Intent:** Make waitable completion paths explicit and debuggable, avoiding complex callback webs.
- **Example shape:** Replace a vector of `std::function` callbacks with a simple `enum class WaitableComponent` and a `switch` statement in a `Complete(component)` function.
- **Benefit:** Less lambda noise, stronger exhaustiveness checking by the compiler, and much easier logging.
- **Avoid when:** Per-case behavior genuinely needs to capture dynamic state beyond the app-level context.

### 5) Build Tools Once

*Don't write a new setup loop for every frame.*

- **Intent:** Move stable, unchanging sequences (like barriers, static draws, or copies) out of the hot frame loop.
- **Example shape:** Build and close command lists once in the constructor, then just execute them per frame.
- **Benefit:** Lower per-frame CPU work and simpler runtime control flow.
- **Avoid when:** The sequence depends on per-frame dynamic state (e.g., changing vertex counts or dynamic UI).

## Goblin Tactics (Micro-patterns)

*(Note: For baseline coding standards like RAII, `Try |` error handling, and designated initializers, see `../.github/QUICK_REFERENCE.md`. The tactics below are specific to keeping code small and direct.)*

- **Straight paths only:** Keep one obvious control path per operation; remove trivial forwarding methods.
- **Guard your own stash:** Co-locate data with the *only* behavior that mutates it.
- **Batch your barriers:** When adjacent transitions are independent and happen at the same stage boundary, group them into one `ResourceBarrier(N, barriers)` call to trim boilerplate and keep state flips in one glance.

## Goblin Traps (Red Flags)

- 🚩 **Useless middle-managers:** "Utility" classes that mostly just rename API calls.
- 🚩 **Mixed bags:** Types holding unrelated resources with different lifetimes.
- 🚩 **Groundhog Day:** Repeated per-frame boilerplate that could have been precomputed once.
- 🚩 **Sneaky magic:** Hidden side effects behind generic method names like `Run` or `Process` without scope qualifiers.
