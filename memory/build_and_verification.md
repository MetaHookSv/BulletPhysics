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

`BULLETPHYSICS_BUILD_TESTS=ON` 注册 `tests/` 下三个 CTest，Release 保留断言（`/UNDEBUG`）：

- `physic_config_tests`：新格式保存/读取往返（逐字段，因子按类型存储）、对象类型过滤与默认值、
  CRC 完整性校验（独立 CRC-32 对照）、旧格式解析与另存迁移、配置注册表、克隆/修改标记/排序删除。
- `shipped_physic_assets_tests`：遍历 `assets/svencoop_downloads`，校验引用、OBJ 网格、
  Bullet 碰撞体包围盒，以及另存后组件、因子与偏移不变。
- `bullet_backend_tests`：欧拉角与骨骼矩阵换算、骨骼 motion state、碰撞体/约束参数映射、
  旧关节迁移、缺失 OBJ 降级、`bv_simrate` 限幅。

测试库 `bulletphysics_test_support` 通过 `$<TARGET_OBJECTS:BulletPhysics>` 链接插件自身目标文件
（排除 `BasePhysicManager.obj`，由 `tests/test_support.cpp` 包含源码重编译以访问文件内序列化函数），
仅替代 KeyValuesSystem、控制台与 OBJ 文件读取。Bullet 以 `/GR-` 构建，测试用类型 ID 而非 `dynamic_cast`。
Release 下插件目标为 `/GL`，测试链接使用 `/LTCG`。

共用 CI action 获取 MetaHook/VGUI2Extension main 并记录 SHA，构建 Release、运行测试、
校验安装 catalog，再将两个运行根目录打包并执行 `7z t`。

## 验证记录

2026-10-03，适用于引入 `tests/` 的变更（含配置序列化、旧格式解析、OBJ 加载与 Bullet 约束修复）：
本地 Release 与 Debug 均构建三个测试目标，`ctest` 3/3 通过；对两处生产代码临时变异、
以及回退到修复前源码时，对应测试均失败。未进行游戏内运行验证。
