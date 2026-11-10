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
- **Memory:** read-only process working set, peak working set, private usage, pagefile usage, system physical memory, pagefile availability, and memory-load telemetry. A one-second worker samples memory outside any game hook. This is telemetry only; it does not free RAM or claim to improve performance.
- **Monitor:** currently a safe disabled stub. The previously proposed `Main::Update` call-site IDs/offsets were not verified and were removed. Upstream CommonLibSSE-NG exposes `RE::Main::RenderWorld(bool)` with relocation IDs 100424 (SE) and 107142 (AE); this is only a candidate render callback, not proof of one call per presented frame. Its call frequency and entry/call-site instructions still need executable-level validation, so it is not hooked. The feature defaults to off and reports why it is unavailable. No FPS values are fabricated.
- **UI:** optional SKSE Menu Framework page registered through the documented AddSectionItem callback API and ImGui text export. It remains optional if the framework is absent. Runtime testing is still required.
- **Config:** INI at Data/SKSE/Plugins/SkyrimFPSMemoryCore.ini. Memory telemetry and the optional menu page default on; the unverified frame monitor defaults off.

## Configuration

The INI file contains these settings:

    [Features]
    EnableFrameMonitor=0
    EnableMemoryTelemetry=1
    EnableMenuFrameworkPage=1

Each optional feature can be disabled independently. No launcher or manual activation is required.

## Safety and performance rules

1. No guessed engine offsets, blind patches, or unvalidated hooks.
2. No reductions to texture resolution, shadows, draw distance, LOD, vegetation, object counts, or other visual quality.
3. Monitoring code does not write to actors or gameplay state.
4. No frame hook is installed until its target function, call-site, instruction, and calling convention are verified for that exact runtime.
5. Memory queries run on a low-frequency worker; they do not run inside a game update hook.
6. File I/O and formatting are kept out of any future frame hook.
7. No periodic EmptyWorkingSet or arbitrary working-set trimming.
8. Do not claim FPS or RAM improvement until a repeatable before/after test demonstrates it on the target runtime.
9. Do not ship a DLL until CI builds it and the target runtime is validated in-game.

## Remaining work before release

- Validate the upstream `RE::Main::RenderWorld(bool)` candidate against the exact pinned CommonLib submodule and the Skyrim SE 1.5.97 executable: confirm the function prologue/call-site, caller, call frequency, and whether it runs once per rendered world frame. If it does not provide a reliable frame metric, use a properly captured DXGI Present source instead. Do not label render-callback cadence as display FPS.
- Validate the SKSE Menu Framework page against the installed 3.x framework release.
- Build the plugin on Windows with the pinned CommonLibSSE-NG dependency and resolve all compiler/linker failures.
- Confirm AE runtime coverage against supported Address Library versions; unsupported versions remain disabled.
- Profile frame-time spikes and memory behavior on a repeatable test route/save before choosing optimizations.
- Implement only low-risk, measurable reductions in redundant plugin-owned work. Each optimization needs an enable/disable setting, runtime guard, logging, and a before/after benchmark.
- Keep Skyrim VR disabled until independently validated.
