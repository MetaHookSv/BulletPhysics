# BulletPhysics

[中文文档](README.zh-CN.md)

BulletPhysics is a MetaHookSv client physics plugin for GoldSrc/SvEngine games.
It provides ragdolls, jiggle bones, collision with moving brushes, buoyancy,
barnacle/gargantua interactions and an in-game physics configuration editor.

## Quick start

Download `BulletPhysics-windows-x86.7z` from
[GitHub Releases](https://github.com/MetaHookSv/BulletPhysics/releases).
Merge both `svencoop/` and `svencoop_downloads/` into the Sven Co-op installation.
Enable `BulletPhysics.dll` in MetaHook's `metahook/configs/plugins.lst` and launch through MetaHook.

The physics data includes example configurations and collision OBJ files; install
their corresponding player models separately.

## Compatibility

|        Engine               |      |
|        ----                 | ---- |
| GoldSrc_blob   (3248~4554)  | √    |
| GoldSrc_legacy (4554~6153)  | √    |
| GoldSrc_new    (8684 ~)     | √    |
| SvEngine       (8832 ~)     | √    |
| GoldSrc_HL25   (>= 9884)    | √    |
| GoldSrc_CoF    (5936)       | √    |

## Documentation

- [Build instruction](docs/en/build-instruction.md)
- [Installation](docs/en/installation.md)
- [Features and configuration](docs/en/features.md)

## License

Licensed under the [MIT License](LICENSE); dependencies retain their own licenses.
