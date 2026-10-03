[Back to README](../../README.md) | [中文](../zh-CN/build-instruction.md)

# Build instruction

## Requirements

- Windows, Visual Studio 2022 C++ desktop workload and Windows SDK
- CMake 3.21+, Git and Python 3.9+ (for gamedata)
- Network access for the initial dependency and gamedata downloads

## Build

```bat
scripts\build-BulletPhysics-x86-Debug.bat
scripts\build-BulletPhysics-x86-Release.bat
```

Both scripts configure with `Visual Studio 17 2022 -A Win32`, build and install.
They work from outside the repository and propagate failure exit codes. Only
Debug and Release are provided. Outputs are `build/x86/<configuration>` and
`install/x86/<configuration>`; installation does not deploy to a running game.

## Dependencies

Explicit paths take precedence and are validated before downloads. Empty source
paths fetch fixed commits; the same-named environment variables supply defaults
on first configure. Use quoted `-DNAME=value` arguments to change cached values.

| Parameter | Required source contents | Default commit |
| --- | --- | --- |
| `METAHOOK_SOURCE_PATH` | MetaHook API, HLSDK/SourceSDK/VGUI sources and `IPanel2.h` | `4d23b6fecd79dc949aabc2e145480cd1328d4a35` |
| `VGUI2EXTENSION_SOURCE_PATH` | Public interface headers under `include/Interface` | `cd7ef6e3b7fb51d3c98e6d7dadec02dd1fa08c4f` |
| `GLEW_SOURCE_PATH` | glew-cmake with `libglew_static` | `56ed32d4a929f993f0e6b7f905af9be4d38fda04` |
| `BULLET3_SOURCE_PATH` | hzqst Bullet3 fork, including `src/btBulletDynamicsCommon.h` | `1ece383aeb5a148533ad5c0cd829cd10d38117c3` |

For example:

```bat
scripts\build-BulletPhysics-x86-Release.bat ^
  "-DMETAHOOK_SOURCE_PATH=D:/MetaHook" ^
  "-DVGUI2EXTENSION_SOURCE_PATH=D:/VGUI2Extension" ^
  "-DGLEW_SOURCE_PATH=D:/glew-cmake" ^
  "-DBULLET3_SOURCE_PATH=D:/bullet3"
```

The symbol APIs require at least API 112. The pinned SDK is API 115, and LoadEngine
requires the host API version used by that build; using a newer SDK can raise that requirement.

External trees are read-only. MetaHook provides SDK code, not a launcher build;
VGUI2Extension provides headers, not a plugin build. Bullet and GLEW are built
statically in this project's build tree. ScopeExit, tinyobjloader and Chocobo1Hash
are pinned submodules initialized during configure. No SDL, Capstone or FreeImage
build is required.

VC-LTL 5.3.1 is downloaded with SHA-256 verification to `thirdparty/cache/`, shared
by Debug/Release. The build uses C++20 and `/MTd` / `/MT`; Release enables LTO.
Bullet stays single precision and its SDK, demos, extras and Python bindings are disabled.

## gamedata

`scripts/manifests/bulletphysics.json` declares module ownership, kind, required
symbols and conditional coverage for 17 identities. `g_ViewEntityIndex_SCClient`
is required only for 10257; 8948 does not publish it. Client Studio virtuals remain
optional. CS/CZ client snapshots use the matching engine identity's catalog.

`BULLETPHYSICS_SYNC_GAMEDATA` defaults to `ON`: sync and validation run before
the DLL build, pruning upstream data into the build tree and then installing it
under `svencoop/metahook/gamedata/bulletphysics`. The host merges this nested catalog.
Raw data is cached in `build/x86/<configuration>/gamedata-sync` for offline builds.
With `OFF`, only existing data at `BULLETPHYSICS_GAMEDATA_DIR` is installed.

## Regression tests

```bat
scripts\build-BulletPhysics-x86-Release.bat -DBULLETPHYSICS_BUILD_TESTS=ON
ctest --test-dir build/x86/Release -C Release --output-on-failure
```

The tests live in `tests/` and link the plugin's own object files, replacing only the
host services that the engine and `vgui2.dll` provide:

- `physic_config_tests`: `*_physics.txt` save/load round trips, object types and defaults,
  model integrity checks, legacy `*_ragdoll.txt` parsing and editor-save migration, the
  configuration registry and editor helpers.
- `shipped_physic_assets_tests`: loads every physics configuration under
  `assets/svencoop_downloads`, checks component references, OBJ meshes and Bullet colliders,
  and confirms an editor save keeps all data.
- `bullet_backend_tests`: GoldSrc/Bullet transform conversion, collider and constraint
  parameter mapping, legacy joint migration and the `bv_simrate` clamp.

Assertions remain enabled in Release. The tests need no game DLL or GL context.

## CI and packaging

LiveBuild (`main` push/PR/manual) and Release (`v*` tags) share the Windows x86
composite action. CI clones the `main` branches of MetaHook and VGUI2Extension,
records their SHAs, builds Release with tests, runs CTest and validates installed
gamedata. It archives both `svencoop` and `svencoop_downloads` as
`BulletPhysics-windows-x86.7z` and runs `7z t`. Local automatic dependencies stay pinned.
