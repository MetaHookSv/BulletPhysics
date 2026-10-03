# BulletPhysics

[English](README.md)

BulletPhysics 是 MetaHookSv 的 GoldSrc/SvEngine 客户端物理插件，提供布娃娃、
晃动骨骼、移动固体碰撞、水浮力、藤壶/喷火怪交互以及物理配置编辑器。

## 快速开始

从 [GitHub Releases](https://github.com/MetaHookSv/BulletPhysics/releases)
下载 `BulletPhysics-windows-x86.7z`（推送 `v*` 标签时构建）。将解压后的
`svencoop/` 和 `svencoop_downloads/` 合并到 Sven Co-op 安装目录，在 MetaHook 的
`metahook/configs/plugins.lst` 启用 `BulletPhysics.dll`，通过 MetaHook 启动游戏。

VGUI2Extension 提供调试/编辑 UI，应在 BulletPhysics 之前加载。使用 Renderer 时，
保持原插件列表中的 Renderer 在 BulletPhysics 之前的顺序。
物理数据包含示例配置和碰撞 OBJ；对应的玩家模型需另行安装。

## 兼容性

Windows x86、OpenGL，默认固定 SDK 构建要求 MetaHook API 115 或更新版本。随包 gamedata 覆盖 17 个
二进制版本，包括 Sven Co-op 10257 和 8948。客户端视角实体槽只存在于 10257；
8948 正常保持空指针，由现有保护跳过相关处理。六个旧 Half-Life 版本只发布引擎数据，
无法满足必需客户端符号，详见[安装说明](docs/zh-CN/installation.md)。
数据覆盖和模拟回归测试不代表真实游戏兼容性已验证。

## 文档

- [构建说明](docs/zh-CN/build-instruction.md)
- [安装说明](docs/zh-CN/installation.md)
- [功能与配置](docs/zh-CN/features.md)

## 许可证

采用 [MIT License](LICENSE)，依赖保留各自的许可证。
