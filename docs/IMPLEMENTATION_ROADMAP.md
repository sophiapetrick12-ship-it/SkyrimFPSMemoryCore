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
