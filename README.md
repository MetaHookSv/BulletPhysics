# BulletPhysics

[中文文档](README.zh-CN.md)

BulletPhysics is a MetaHookSv client physics plugin for GoldSrc/SvEngine games.
It provides ragdolls, jiggle bones, collision with moving brushes, buoyancy,
barnacle/gargantua interactions and a physics configuration editor.

## Quick start

Download `BulletPhysics-windows-x86.7z` from
[GitHub Releases](https://github.com/MetaHookSv/BulletPhysics/releases) (built on `v*` tag pushes).
Merge both `svencoop/` and `svencoop_downloads/` into the Sven Co-op installation.
Enable `BulletPhysics.dll` in MetaHook's `metahook/configs/plugins.lst` and launch through MetaHook.

VGUI2Extension supplies the debug/editor UI. Load it before BulletPhysics; when
Renderer is enabled, keep Renderer before BulletPhysics as in the original plugin list.
The physics data includes example configurations and collision OBJ files; install
their corresponding player models separately.

## Compatibility

Windows x86 and OpenGL, with MetaHook API 115 or later for the default pinned SDK build. The packaged gamedata
covers 17 identities, including Sven Co-op 10257 and 8948. The optional client
view-entity slot exists only in 10257; 8948 retains the normal guarded path.
Six legacy Half-Life identities only publish engine data, so they cannot satisfy
the required client symbols. See [Installation](docs/en/installation.md).
Catalog coverage and simulated regression tests do not establish real-game compatibility.

## Documentation

- [Build instruction](docs/en/build-instruction.md)
- [Installation](docs/en/installation.md)
- [Features and configuration](docs/en/features.md)

## License

Licensed under the [MIT License](LICENSE); dependencies retain their own licenses.
