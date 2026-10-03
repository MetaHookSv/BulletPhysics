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

## 本次验证记录

实测日期：2026-10-03。Windows + VS2022/MSVC 19.44 + Windows SDK 10.0.26100，
CMake 3.31.12、Python 3.12.5。默认 SDK 的 METAHOOK_API_VERSION 为 115；
符号接口最低要求 112，插件 LoadEngine 仍按编译所用 SDK 版本要求宿主。

| 项目 | 实际结果 |
| --- | --- |
| Debug 配置/编译/链接/安装 | 本地 MetaHook/VGUI2Extension/GLEW/Bullet 源码路径；最终脚本退出 0 |
| Release 配置/编译/链接/安装 | 从仓库外调用入口，四个源码路径均为空，默认 FetchContent；脚本退出 0 |
| 固定依赖提交 | 实际 checkout SHA 与 Dependencies.cmake 四个 pin 逐一一致 |
| CTest | Debug、Release 分别 1/1 通过；退出 0 |
| 安装 catalog | 两种配置均通过 `validate-gamedata.py ... --manifest scripts/manifests/bulletphysics.json`，17 snapshots；退出 0 |
| 负路径 | 四种无效显式源码路径均退出 1 并给出预期诊断；未设 Configuration 的通用脚本退出 2 |
| manifest 行为 | 8948 无槽正常；移除 10257 必需槽、8948 pitchdrift、HL size_of_frame 或破坏 portal kind 均被拒绝 |
| PE | 两种配置均为 x86、唯一导出 CreateInterface；Bullet/GLEW 静态链接，CRT 导入 msvcrt |
| 源码与编译清单 | 127 个文件中 126 个与源逐字节一致，仅 privatehook.cpp 作可选解析修正；167 编译项与源一致且路径均存在 |
| 资源 | 32 个文件与源、Debug/Release 安装树逐字节一致 |
| 发布包 | `build/artifacts/BulletPhysics-windows-x86.7z`，3,740,501 字节；52 文件，`7z t` 退出 0，解压后逐文件与 Release 安装树一致 |
| 文档/配置 | 本地 Markdown 链接均解析；memory frontmatter、两份 MCP 项目名、GitHub YAML 解析检查通过 |
| Review | 独立只读审查未发现功能性移植缺陷；已修正文档残留 bv_debug 与默认宿主 API 要求 |

首次构建的回归测试缺少与生产 translation unit 同时链接的 engine/SDK 替身，补齐后两种
配置通过。产物检查发现 Bullet 的全局 CMAKE_DEBUG_POSTFIX 污染插件命名，已以插件
DEBUG_POSTFIX 空值固定为 BulletPhysics.dll，并重新构建安装和核验 PE。相关日志在
`build/x86/<配置>/`；它们是本地生成物，不随源码或发布包交付。

未执行 hosted GitHub Actions；本地已执行构建、CTest、catalog 校验和打包步骤，工作流
另经 YAML 解析与静态审查。真实游戏加载、OpenGL context、UI、ragdoll 和 Renderer 共存
尚未验证。历史源仓结果不能用于证明新的源码/API 变更。
