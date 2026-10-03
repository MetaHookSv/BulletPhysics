[返回 README](../../README.zh-CN.md) | [English](../en/installation.md)

# 安装说明

1. 安装兼容的 MetaHook（默认构建 API 115+），并为 UI 安装 VGUI2Extension。
2. 将发布包中的 `svencoop/` 和 `svencoop_downloads/` 合并到 Sven Co-op 根目录。
3. 在 `svencoop/metahook/configs/plugins.lst` 启用 `BulletPhysics.dll`，保持
   VGUI2Extension 和使用时的 Renderer 在 BulletPhysics 之前。
4. 通过 MetaHook 启动游戏。允许 cheats 时，`bv_open_debug_ui` 打开编辑器。

其他 GoldSrc mod 将 `svencoop/` 内容放入对应 mod 目录，并把示例物理数据放入模型搜索路径。
模型名称和骨骼必须与配置匹配，对应玩家模型不随包发布。

DLL/PDB 位于 `metahook/plugins`，UI/本地化位于 `bulletphysics`，插件 gamedata
位于 `metahook/gamedata/bulletphysics`。宿主仍需自己的主 catalog。Bullet/GLEW 静态链接。

数据包含 `cof-5936`、`hl-10210/3248/3266/3329/3647/4554/6153/8684`、
`svencoop-10257/8948` 引擎版本，以及 `cstrike/czero/czeror-8684/10210` 客户端快照。
六个不高于 6153 的旧 Half-Life 快照没有客户端模块，必需客户端符号解析仍会失败。
8948 本身没有视角实体槽是正常差异，不会因此报错。
游戏加载、物理效果与 Renderer 共存需与数据覆盖分别验证。
