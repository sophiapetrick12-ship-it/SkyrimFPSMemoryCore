# Implementation roadmap

## Compatibility target

- One CommonLibSSE-NG plugin build with runtime-specific code paths.
- Primary validation target: Skyrim SE 1.5.97.
- Additional targets: Skyrim SE/AE runtimes supported by the pinned CommonLibSSE-NG and their matching Address Library entries.
- Skyrim VR is a separate compatibility target; do not claim it is supported until its hook and UI paths are independently validated.
- Detect the exact runtime at startup. Unknown or unvalidated runtimes must load safely with unsupported features disabled.
- Never require the user to downgrade, replace, or modify their Skyrim installation.

## Modules

- **Core** — SKSE lifecycle, runtime detection, logging and safe initialization.
- **Memory** — process telemetry first. No periodic EmptyWorkingSet or arbitrary working-set trimming. Memory optimization must be based on measured allocations/cache behavior and must not evict assets or lower visual quality.
- **Monitor** — actual Skyrim main-loop timing and process memory. Do not substitute ImGui/menu FPS for game FPS.
- **UI** — optional SKSE Menu Framework page, registered automatically when its supported API is present. The plugin must still load if the framework is absent.
- **Config** — persisted settings with conservative defaults and explicit per-feature enable/disable.

## Safety and performance rules

1. No guessed engine offsets, blind patches, or unvalidated hooks.
2. No reductions to texture resolution, shadows, draw distance, LOD, vegetation, object counts, or other visual quality.
3. Monitoring code performs no writes to actors or engine gameplay state.
4. Each runtime-specific hook is gated by exact runtime detection and validated relocation/call-site resolution. Failure disables that feature, not the whole plugin.
5. Sampling on the game thread must be constant-time and allocation-free where practical. File I/O and formatting stay off the game thread.
6. Do not claim FPS or RAM improvement until a repeatable before/after test demonstrates it on the target runtime.
7. Do not ship a DLL until CI builds it and the target runtime is validated in-game.

## Current implementation

The first memory component captures the current process working set, peak working set, private usage and pagefile usage through the Windows process API. This is read-only telemetry; it does not yet optimize or clean RAM.

## Frame timing design (research complete; implementation pending)

- A public Skyrim performance-monitor project demonstrates a `RE::Main::Update` hook and monotonic-clock measurement, while separately recording `RE::BSTimer::delta` and `realTimeDelta`.
- The resulting metric is Skyrim main-loop cadence, not guaranteed GPU-present/display FPS. The UI and logs must label it accurately.
- Do not copy another plugin's fixed relocation IDs/offsets without independently validating each supported runtime and call-site signature.
- Resolve the hook through CommonLibSSE-NG/Address Library for each exact runtime. If a relocation is absent, ambiguous, or fails validation, disable FPS sampling for that runtime.
- SE 1.5.97 must be explicitly resolved and tested before the hook is enabled there. AE runtime support is added per validated runtime, not by assuming all AE versions share one address.
- Keep sampling lightweight; buffer data in memory and move disk writes to a worker thread.

## SKSE Menu Framework integration (official API checked; implementation pending)

- The official QTR-Modding API header exposes `IsInstalled`, `SetSection`, `AddSectionItem`, and `GetMenuFrameworkVersion`; page registration uses the exported `AddSectionItem` function.
- The current Nexus listing reports framework 3.18. Use the official API header/exports rather than a guessed older header or invented API version.
- Detect the loaded framework module and verify required exports/version before registering. Do not make the framework a hard dependency.
- Register the plugin section/page at a safe SKSE lifecycle stage after the framework is loaded.
- Keep rendering limited to the framework's supported UI callback; never use menu framerate as game FPS.

## Actual optimization work still required

- Profile frame-time spikes and memory behavior on a repeatable test route/save before choosing optimizations.
- Prefer low-risk, measurable reductions in redundant plugin-owned work and avoid changing vanilla rendering quality or gameplay state.
- Treat process memory counters as diagnostic measurements, not proof of memory optimization.
- Each optimization needs an enable/disable setting, runtime guard, logging, and a before/after benchmark.
