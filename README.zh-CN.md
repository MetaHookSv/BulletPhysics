# BulletPhysics

[English](README.md)

BulletPhysics 是 MetaHookSv 的 GoldSrc/SvEngine 客户端物理插件，提供布娃娃、晃动骨骼、移动固体碰撞、水浮力、藤壶/喷火怪交互以及游戏内的物理配置编辑器。

## 快速开始

从 [GitHub Releases](https://github.com/MetaHookSv/BulletPhysics/releases) 下载 `BulletPhysics-windows-x86.7z`。

将解压后的`svencoop/` 和 `svencoop_downloads/` 合并到 Sven Co-op 安装目录，在 MetaHook 的 `metahook/configs/plugins.lst` 启用 `BulletPhysics.dll`，然后通过 MetaHook 启动游戏。

物理数据包含示例配置和碰撞 OBJ；对应的玩家模型需另行安装。

## 兼容性

|        Engine               |      |
|        ----                 | ---- |
| GoldSrc_blob   (3248~4554)  | √    |
| GoldSrc_legacy (4554~6153)  | √    |
| GoldSrc_new    (8684 ~)     | √    |
| SvEngine       (8832 ~)     | √    |
| GoldSrc_HL25   (>= 9884)    | √    |
| GoldSrc_CoF    (5936)       | √    |

## 文档

- [构建说明](docs/zh-CN/build-instruction.md)
- [安装说明](docs/zh-CN/installation.md)
- [功能与配置](docs/zh-CN/features.md)
- [F5 调试（可选）](docs/zh-CN/debugging.md)

## 许可证

采用 [MIT License](LICENSE)，依赖保留各自的许可证。
