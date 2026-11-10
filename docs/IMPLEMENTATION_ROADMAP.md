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
- **Monitor:** uses the primary game's DXGI `IDXGISwapChain::Present` callback. It shadows the swap-chain object's 18-entry `IDXGISwapChain` vtable and replaces only slot 8 on that object; it does not patch Skyrim executable code or use guessed Address Library offsets. The callback reads a steady-clock timestamp, updates atomic interval/count counters, and forwards to the original Present function. The metric is DXGI Present-call cadence, not physical display scan-out FPS. The feature defaults on; if the renderer, swap chain, or writable object vptr is unavailable, installation fails closed and the menu reports that no samples are available. In-game and overlay compatibility testing is still required.
- **UI:** optional SKSE Menu Framework page registered through the documented AddSectionItem callback API and ImGui text export. It remains optional if the framework is absent. Runtime testing is still required.
- **Config:** INI at Data/SKSE/Plugins/SkyrimFPSMemoryCore.ini. Memory telemetry and the optional menu page default on; the unverified frame monitor defaults off.

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
4. The Present callback runs on the render thread and must not read game state, allocate, lock, or log. It only records a timestamp and atomic counters before forwarding to the original Present function.
5. Memory queries run on a low-frequency worker; they do not run inside a game update hook.
6. File I/O and formatting are kept out of any future frame hook.
7. No periodic EmptyWorkingSet or arbitrary working-set trimming.
8. Do not claim FPS or RAM improvement until a repeatable before/after test demonstrates it on the target runtime.
9. Do not ship a DLL until CI builds it and the target runtime is validated in-game.

## Remaining work before release

- Validate `RE::BSGraphics::Renderer::GetRuntimeData().renderWindows[0].swapChain` against the exact pinned CommonLib submodule and the Skyrim SE 1.5.97 runtime. Confirm that the swap-chain object remains valid, test device-reset/swap-chain recreation behavior, and test coexistence with ReShade/ENB/other DXGI hooks. Keep the metric labelled as Present-call cadence, not physical display FPS.
- Validate the SKSE Menu Framework page against the installed 3.x framework release.
- Build the plugin on Windows with the pinned CommonLibSSE-NG dependency and resolve all compiler/linker failures.
- Confirm AE runtime coverage against supported Address Library versions; unsupported versions remain disabled.
- Profile frame-time spikes and memory behavior on a repeatable test route/save before choosing optimizations.
- Implement only low-risk, measurable reductions in redundant plugin-owned work. Each optimization needs an enable/disable setting, runtime guard, logging, and a before/after benchmark.
- Keep Skyrim VR disabled until independently validated.
