# AGENTS.md

This file provides guidance and important rules working with code in this repository.

## When coding / building plan

- Use a progressive disclosure approach for agent coding in this repository: start from high-level information in the Basic Memory knowledge base first, and only locate/read specific files or symbols when necessary, instead of expanding a large amount of context at once.

### Basic Memory knowledge base (project-scoped, `memory/`)

- Notes live in `memory/` (markdown with YAML frontmatter: `title`/`type`/`permalink`), tracked in git.
- Notes use the `bulletphysics/` permalink prefix to distinguish them from the source repository.

### High-level information in this repository (read corresponding notes first)

- Project overview, dependency boundaries and entry points: `project_overview`
- Physics configuration lifecycle and the ragdoll configuration format: `physics_config`
- Studio bone / Bullet rigid-body synchronization: `bonematrix_physics_sync`
- Build commands, dependency pinning, gamedata sync, verification status: `build_and_verification`
- Engine-private symbol inventory (only the symbols resolved today): `PrivateSymbols.md`
- Coding conventions: `CodeStyles`

### When notes are insufficient: source entry points (query and read on demand)

- Build: `CMakeLists.txt`, `cmake/Sources.cmake` (explicit compile list), `cmake/Dependencies.cmake` (source-path resolution and FetchContent fallback), `cmake/VCLTL.cmake`, `scripts/build-BulletPhysics-x86-{Debug,Release}.bat`
- Plugin sources: `src/`; lifecycle entry `src/plugins.cpp`, engine/client hooks and gamedata `src/privatehook.cpp` and `src/exportfuncs.cpp`, manager contract `src/ClientPhysicManager.h` with the backend-agnostic `src/BasePhysicManager.*` and the Bullet `src/BulletPhysicManager.*`, VGUI2Extension debug/editing UI `src/Viewport.*`, `src/Physic*`, `src/AnimControl*`
- Runtime assets: `assets/svencoop/bulletphysics/` (`.res` layouts and localization) and `assets/svencoop_downloads/models/` (physics configurations and collision OBJ meshes), installed alongside the DLL
- gamedata: `scripts/manifests/bulletphysics.json` (same schema as MetaHook), `scripts/sync-gamedata.py`, `scripts/validate-gamedata.py`; the build-time sync prunes the upstream catalog into the nested `metahook/gamedata/bulletphysics/` directory, which the host launcher merges
- Tests: `tests/` (`physic_config_tests`, `shipped_physic_assets_tests`, `bullet_backend_tests`), run by CTest with `BULLETPHYSICS_BUILD_TESTS=ON`. They link the plugin's own objects; `tests/test_support.*` supplies the engine/`vgui2.dll` services and recompiles `BasePhysicManager.cpp` to reach its file-local serializers
- Docs: `README.md` / `README.zh-CN.md`, prose pages under `docs/en/` and `docs/zh-CN/`
- External sources, all read-only inputs: `METAHOOK_SOURCE_PATH` (public API, HLSDK, SourceSDK, VGUI), `VGUI2EXTENSION_SOURCE_PATH` (public interface headers only; the plugin is not built here), `GLEW_SOURCE_PATH`, `BULLET3_SOURCE_PATH`. Empty paths fall back to FetchContent (MetaHook tracks the latest `main`; the other dependencies use fixed commits); only `thirdparty/ScopeExit`, `thirdparty/tinyobjloader` and `thirdparty/Chocobo1Hash` are submodules.
- Build output: `build/x86/<configuration>/`; install output: `install/x86/<configuration>/`. Neither is tracked, and nothing is deployed to the game automatically.

## Repository rules

- Preserve the MetaHook plugin exports, ABI, calling conventions, physics behavior and resource formats. Match the naming, indentation, encoding and comment style of the files you touch.
- Resolve engine and client private symbols through the host gamedata contract (`GamedataResolvePtr`/`ResolveGameSymbol`). Required symbols stay fatal; intentionally optional virtuals and the Sven view-entity slot remain optional, 8948 never publishes `g_ViewEntityIndex_SCClient`, and the existing null guards are the intended path for it.
- Consume the `BULLETPHYSICS_*_INCLUDE_DIRS` paths produced by the dependency preparation functions; do not bypass the normalized results by using the raw cache inputs.
- When gamedata usage changes, update `scripts/manifests/bulletphysics.json` in the same change.
- Do not modify external sources or third-party sources. MetaHook provides the SDK sources and VGUI2Extension the interface headers; this repository builds neither plugin.
- Preserve the explicit compile lists in `cmake/Sources.cmake` and the pinned dependency commits in `cmake/Dependencies.cmake`.
- Regression tests keep assertions enabled even in Release (`/UNDEBUG`); documentation and configuration text are not assertion targets.
- Verification distinguishes build/simulated tests from a real game run. Claims about in-game physics behavior or visual compatibility must not be made without evidence. Documentation changes need content and path checks, not a DLL rebuild, and actual results belong in `build_and_verification`.
