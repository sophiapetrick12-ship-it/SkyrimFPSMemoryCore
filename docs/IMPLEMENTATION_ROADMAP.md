# Implementation roadmap

## Compatibility target

- One CommonLibSSE-NG plugin build with runtime-specific code paths.
- Primary validation target: Skyrim SE 1.5.97.
- Skyrim SE 1.5.97 is the validated pre-AE target. AE runtimes are eligible only when the matching Address Library relocation exists and the runtime call instruction passes executable-segment and destination checks. Other pre-AE SE runtimes remain disabled.
- Skyrim VR remains a separate target and is not currently supported.
- Unknown or unvalidated runtimes load safely with unsupported features disabled.
- The plugin never requires downgrading, replacing, or modifying the user's Skyrim installation.

## Implemented modules

- **Core:** SKSE lifecycle, runtime detection, logging, and safe initialization.
- **Memory:** read-only process working set, peak working set, private usage, pagefile usage, system physical memory, pagefile availability, and memory-load telemetry. A one-second worker samples memory outside the game update hook.
- **Monitor:** RE::Main::Update call-site timing for Skyrim SE 1.5.97 and AE runtimes where the matching Address Library entry exists and the runtime call-site validation passes. Unsupported pre-AE SE versions and VR fail closed. The metric is explicitly labelled main-loop cadence, not GPU-present/display FPS.
- **UI:** optional SKSE Menu Framework page registered automatically at kDataLoaded, using runtime export lookup. The plugin remains usable if the framework is absent or its required API exports are unavailable.
- **Config:** persistent INI settings at Data/SKSE/Plugins/SkyrimFPSMemoryCore.ini, created with conservative enabled defaults.

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
4. The frame hook is enabled for Skyrim SE 1.5.97 and for AE only when its exact Address Library relocation resolves and the call-site checks pass. Other pre-AE SE versions and VR fail closed.
5. The hook verifies the call-site lies in executable code, checks for a CALL rel32 instruction, and verifies its destination is in the executable code segment before patching.
6. Memory queries run on a low-frequency worker; the game update hook performs only timestamp and atomic-counter work.
7. File I/O and formatting are kept out of the game update hook.
8. No periodic EmptyWorkingSet or arbitrary working-set trimming.
9. Do not claim FPS or RAM improvement until a repeatable before/after test demonstrates it on the target runtime.
10. Do not ship a DLL until CI builds it and the target runtime is validated in-game.

## Remaining work before release

- Validate the frame hook in-game on Skyrim SE 1.5.97 and confirm its frame-time values against an external frame-time capture.
- Validate the SKSE Menu Framework page on the current 3.x framework release.
- Confirm AE runtime coverage against the supported Address Library release matrix; unsupported/missing IDs continue to fail closed.
- Profile frame-time spikes and memory behavior on a repeatable test route/save before choosing optimizations.
- Implement only low-risk, measurable reductions in redundant plugin-owned work. Each optimization needs an enable/disable setting, runtime guard, logging, and a before/after benchmark.
- Keep Skyrim VR disabled until independently validated.
