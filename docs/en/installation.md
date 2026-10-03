[Back to README](../../README.md) | [中文](../zh-CN/installation.md)

# Installation

1. Install a compatible MetaHook launcher (API 115+ for the default build) and VGUI2Extension for the UI.
2. Merge the archive's `svencoop/` and `svencoop_downloads/` into the Sven Co-op root.
3. Enable `BulletPhysics.dll` in `svencoop/metahook/configs/plugins.lst`.
   Keep VGUI2Extension and, when used, Renderer before BulletPhysics.
4. Launch through MetaHook. With cheats allowed, `bv_open_debug_ui` opens the editor.

For another GoldSrc mod, put the `svencoop/` contents into its mod directory and
place the example physics files in the mod's model search path. Model filenames
and bones must match the configuration. Corresponding player models are not included.

The DLL/PDB live in `metahook/plugins`, UI/localization in `bulletphysics`, and the
plugin's gamedata in `metahook/gamedata/bulletphysics`. The host's primary gamedata
catalog is still required. Bullet and GLEW are statically linked.

The catalog contains engine identities `cof-5936`, `hl-10210/3248/3266/3329/3647/4554/6153/8684`,
`svencoop-10257/8948`, plus client snapshots `cstrike/czero/czeror-8684/10210`.
The six old Half-Life snapshots through 6153 publish no client module, so required
client resolution still fails there. 8948's absent view-entity slot is expected and
does not cause that failure. Actual game loading, physics and Renderer coexistence
must be verified separately from catalog coverage.
