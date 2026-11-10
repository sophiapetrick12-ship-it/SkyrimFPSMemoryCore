# Implementation roadmap

## Compatibility baseline
- CommonLibSSE-NG is pinned by the repository submodule commit.
- The plugin is built as a single CommonLibSSE-NG multi-runtime DLL (SE/AE/VR).
- Skyrim SE 1.5.97 remains the primary validation runtime.
- No game files are changed by the build or by this source tree.

## Modules
- **Core** — plugin lifecycle, runtime checks, logging and safe initialization.
- **Memory** — read-only process telemetry first; any future memory policy must be opt-in and must not use periodic EmptyWorkingSet/working-set trimming.
- **Monitor** — real game-frame timing/FPS source; do not substitute ImGui's menu frame rate.
- **UI** — optional SKSE Menu Framework integration, detected at runtime. The plugin must still load when the framework is absent.
- **Config** — persisted user settings with conservative defaults.

## Safety gates
1. No guessed engine offsets or unverified hooks.
2. No texture, shadow, LOD, vegetation, object or visual-quality reductions.
3. No writes to game actors or engine state from monitoring code.
4. Each hook is isolated behind runtime/version validation and can fail closed.
5. Do not ship a DLL until CI builds it and the target runtime is validated.

## Current implementation
The first memory component captures the current process working set, peak working set, private usage and pagefile usage using the Windows process API. It is read-only telemetry, not a RAM cleaner.


## Frame timing design (research complete; implementation pending)
- The menu's ImGui frame-rate value is not a game FPS source and must never be displayed as Skyrim FPS.
- A candidate game-loop source is the engine's `RE::Main::Update` call site, measured with a monotonic high-resolution clock. The existing public Skyrim Performance Monitor project uses this pattern and separately records `RE::BSTimer::realTimeDelta` and `delta`.
- The measured value represents Skyrim main-loop cadence, not guaranteed GPU-present/display cadence. Do not label it as presentation FPS.
- Any hook must use Address Library relocation IDs for each supported runtime, validate the resolved call site before installing, and disable FPS sampling if validation fails. No hard-coded absolute address fallback.
- Keep sampling lightweight: capture timestamps and engine deltas only on the game thread; move formatting and disk I/O off-thread.
- SE 1.5.97 must be explicitly included in the runtime matrix before enabling the hook. AE support remains gated until its relocation is validated.

## Menu Framework integration (API verified; build wiring pending)
- SKSE Menu Framework 3.8 exposes `AddSectionItem` and API-version query exports; the supported usage is to detect the framework, set the plugin section, then register a render callback.
- The SDK's documented build requires the matching ImGui headers/import library and the Framework import library. Do not call ImGui functions without linking the matching SDK library.
- Framework integration remains optional: missing DLL/API or unsupported API version must skip registration without preventing plugin load.
- Register the page automatically during a safe SKSE lifecycle stage after the framework module is available.
