# Implementation roadmap

## Compatibility target

- Build target: CommonLibSSE-NG with runtime-specific code paths.
- First runtime to validate: Skyrim SE 1.5.97, matching the user's installed version.
- AE support is not claimed until the plugin is built and checked against each supported Address Library/runtime combination.
- Skyrim VR is a separate target and is not currently supported.
- Unknown or unvalidated runtimes must load with unsupported features disabled.
- The plugin never requires downgrading, replacing, or modifying the user's Skyrim installation.

## Implemented modules

- **Core:** SKSE lifecycle, runtime detection, logging, and initialization.
- **Memory:** read-only process working set, peak working set, private usage, process commit charge, system physical memory, system commit-limit/available-commit counters, and memory-load telemetry. A one-second worker samples memory outside any game hook. This is telemetry only; it does not free RAM or claim to improve performance.
- **Monitor:** enabled only on the exact Skyrim SE 1.5.97.0 runtime. AE is explicitly disabled until each supported runtime generation is validated. On the supported runtime it uses the primary game's DXGI `IDXGISwapChain::Present` callback. It patches vtable slot 8 (Present) after validating the renderer, swap chain, object memory, and vtable slot; it temporarily changes that data page to `PAGE_READWRITE` and restores the previous protection. It preserves and calls the original Present function, and counts only calls whose swap-chain pointer matches the game's primary swap chain. It does not patch Skyrim executable code or use guessed Address Library offsets. The callback reads a steady-clock timestamp and updates atomic interval/count counters. The metric is DXGI Present-call cadence, not physical display scan-out FPS. The feature defaults on; if validation or installation fails, it fails closed and the menu reports that no samples are available. The vtable slot may be shared by DXGI objects in the process, so coexistence with ReShade/ENB/other hooks still needs in-game testing.
- **UI:** optional SKSE Menu Framework page registered through the documented AddSectionItem callback API and ImGui text export. It remains optional if the framework is absent. Runtime testing is still required.
- **Config:** INI at Data/SKSE/Plugins/SkyrimFPSMemoryCore.ini. Frame monitoring, memory telemetry, and the optional menu page default on; unsupported runtimes fail closed.

## Configuration

The INI file contains these settings:

    [Features]
    EnableFrameMonitor=1
    EnableMemoryTelemetry=1
    EnableMenuFrameworkPage=1

Each optional feature can be disabled independently. No launcher or manual activation is required.

## Safety and performance rules

1. No guessed engine offsets, blind patches, or unvalidated hooks.
2. No reductions to texture resolution, shadows, draw distance, LOD, vegetation, object counts, or other visual quality.
3. Monitoring code does not write to actors or gameplay state.
4. The Present callback runs on the render thread and must not read game state, allocate, lock, or log. It only timestamps calls for the primary swap chain, updates atomic counters, and forwards every call to the saved original Present function.
5. Memory queries run on a low-frequency worker; they do not run inside a game update hook.
6. No file I/O, formatting, allocation, locking, or logging occurs in the Present callback.
7. No periodic EmptyWorkingSet or arbitrary working-set trimming.
8. Do not claim FPS or RAM improvement until a repeatable before/after test demonstrates it on the target runtime.
9. Do not ship a DLL until CI builds it and the target runtime is validated in-game.

## Remaining work before release

- Validate `RE::BSGraphics::Renderer::GetRuntimeData().renderWindows[0].swapChain` against the exact pinned CommonLib submodule and the Skyrim SE 1.5.97 runtime. Confirm that the swap-chain object remains valid, test device-reset/swap-chain recreation behavior, and test coexistence with ReShade/ENB/other DXGI hooks. Keep the metric labelled as Present-call cadence, not physical display FPS.
- Validate the SKSE Menu Framework page against the installed 3.x framework release.
- Build the plugin on Windows with the pinned CommonLibSSE-NG dependency and resolve all compiler/linker failures.
- Add AE runtimes to the monitor allowlist only after each exact runtime has a successful build and in-game swap-chain/overlay validation; `REL::Module::IsAE()` alone must never enable the hook.
- Profile frame-time spikes and memory behavior on a repeatable test route/save before choosing optimizations.
- Implement only low-risk, measurable reductions in redundant plugin-owned work. Each optimization needs an enable/disable setting, runtime guard, logging, and a before/after benchmark.
- Keep Skyrim VR disabled until independently validated.
