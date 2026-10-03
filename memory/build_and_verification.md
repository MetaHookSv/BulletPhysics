---
title: build_and_verification
type: note
permalink: bulletphysics/build-and-verification
---

# 构建与验证

## 构建入口与依赖

Windows MSVC x86、CMake 3.21+，仅 Debug/Release。`scripts/build-BulletPhysics-x86-*.bat`
配置到 `build/x86/<配置>`，构建并安装到 `install/x86/<配置>`。完整参数见双语构建文档。

`cmake/Dependencies.cmake` 提供环境变量/显式源码路径与 FetchContent 固定提交回退，
先校验显式路径再下载。原 vcxproj 167 个编译项显式列于 `cmake/Sources.cmake`。
VC-LTL 5.3.1 SHA-256 与 Renderer 一致，缓存位于 `thirdparty/cache`，Debug/Release 共用。
Bullet/GLEW 使用父工程 VC-LTL；Bullet 保留单精度，关闭额外程序与 SDK 安装。

## gamedata 和安装

manifest 覆盖 17 个版本，声明必需、可选和 Sven/CS/CZ 条件符号。构建同步与校验默认开启，
原始缓存支持离线。Sven 8948 不要求视角实体槽；10257 在发布门禁中要求它。
六个旧 Half-Life 版本只有 engine 数据，不能据此宣称客户端能加载。

| 安装目录 | 内容 |
| --- | --- |
| `svencoop/metahook/plugins` | BulletPhysics.dll/PDB |
| `svencoop/metahook/gamedata/bulletphysics` | 裁剪后的 catalog |
| `svencoop/bulletphysics` | UI 与本地化 |
| `svencoop_downloads/models` | 物理配置与引用 OBJ |

## 回归与 CI

`BULLETPHYSICS_BUILD_TESTS=ON` 注册 CTest，调用真实 Client_FillAddress 并模拟宿主 API。
覆盖 10257、8948、旧指针清除及其他必需符号缺失，Release 保留断言。
共用 CI action 获取 MetaHook/VGUI2Extension main 并记录 SHA，构建 Release、运行测试、
校验安装 catalog，再将两个运行根目录打包并执行 `7z t`。
