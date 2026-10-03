---
title: project_overview
type: note
permalink: bulletphysics/project-overview
---

# BulletPhysics 独立工程

## 范围与来源

从 MetaHookSv `fe80b6d60bfb487b52aed7ea7ec0492e7b27a5d2` 的 `Plugins/BulletPhysics`、
`Build/svencoop/bulletphysics` 和 downloads 物理数据子集迁入，工程约定参照独立 Renderer。
127 个源码/头文件、17 个 UI/本地化文件、13 个物理配置与 2 个 OBJ 保留原样；
源码行为改动仅为 Sven 8948/10257 的视角实体槽可选解析。

## 架构与入口

- `src/plugins.cpp`：IPluginsV4 生命周期、宿主 API 与依赖接入。
- `src/privatehook.cpp`、`src/exportfuncs.cpp`：引擎/客户端 gamedata、hook、帧与 Studio 骨骼同步。
- `src/BasePhysicManager.*`：配置、对象、组件和资源管理。
- `src/BulletPhysicManager.*` 与 `src/Bullet*`：Bullet world、刚体、约束和行为。
- `src/Viewport.*`、`src/Physic*`：VGUI2Extension 调试/编辑 UI。
- `assets/svencoop/bulletphysics`、`assets/svencoop_downloads`：运行资源。

## 依赖边界与契约

只消费 MetaHook 的公共 API/HLSDK/SourceSDK/VGUI 源码，不构建宿主；VGUI2Extension 只消费
公共接口头，运行时提供 UI。未提供源码路径时通过 FetchContent 获取固定提交。
Bullet/GLEW 静态构建；ScopeExit、tinyobjloader、Chocobo1Hash 为固定提交子模块。
VC-LTL 使用校验后的 5.3.1 包。SDL、Capstone、FreeImage 不属于本插件依赖。

插件保持 IPluginsV4/CreateInterface 和原调用约定；物理接口为内部头，没有单独的公共 SDK。
私有符号通过宿主 ResolveGameSymbol/QueryGameSymbolScalar 解析，消费变化需同步 manifest。
gamedata 裁剪至 `metahook/gamedata/bulletphysics`，宿主与其他 catalog 合并。

`g_ViewEntityIndex_SCClient` 对运行时可选，manifest 仅在 10257 要求；8948 本来没有它。
现有三处骨骼同步使用均保护空指针。两个 Sven 版本仍要求 portal 状态和 pitchdrift。

## 知识入口

- [[BulletPhysics]]、[[BulletPhysics Plugin Overview]]：架构和生命周期。
- [[bulletphysics_physics_config]]、[[bulletphysics_bonematrix_physics_sync]]：配置和骨骼同步。
- [[PrivateSymbols]]：当前符号清单及保留的历史定位记录。
- [[build_and_verification]]：构建入口、验证证据和未验证范围。
- [[CodeStyles]]：源代码约定，修改时以具体文件风格为准。

MCP 配置使用 `bulletphysics` 项目名。只有项目实际绑定至本目录 `memory/` 时才能通过 MCP
写入；没有对应绑定时直接读写本地笔记。源项目 `metahooksv` 仅作为来源查阅。
