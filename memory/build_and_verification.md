---
title: build_and_verification
type: note
permalink: bulletphysics/build-and-verification
---

# 构建与验证

## 构建入口与依赖

Windows MSVC x86、CMake 3.21+，仅 Debug/Release。`scripts/build-BulletPhysics-x86-*.bat`
配置到 `build/x86/<配置>`，构建并安装到 `install/x86/<配置>`。完整参数见双语构建文档。

`cmake/Dependencies.cmake` 提供环境变量/显式源码路径与 FetchContent 回退（MetaHook 跟踪最新 `main`），
先校验显式路径再下载。原 vcxproj 167 个编译项显式列于 `cmake/Sources.cmake`。
VC-LTL 5.3.1 SHA-256 与 Renderer 一致，缓存位于 `thirdparty/cache`，Debug/Release 共用。
Bullet/GLEW 使用父工程 VC-LTL；Bullet 保留单精度，关闭额外程序与 SDK 安装。

## gamedata 和安装

manifest 覆盖 23 个版本，声明必需、可选和 Sven/CS/CZ 条件符号。构建同步与校验默认开启，
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

2026-10-15，MetaHookSv #926，CS/CZ 内层玩家绘制入口接入：

- 聚合树 `build/x86/Debug` 构建 BulletPhysics 和三个测试目标成功；CTest 3/3 通过，
  `BulletPhysics-format-check` 通过，构建同步的 19 个 manifest catalog 校验通过。
- `bullet_backend_tests` 新增 hook 路由回归：存在内层时只挂内层、缺少内层时回退外层、
  无客户端入口时不安装，以及重复安装/卸载的幂等性。
- 使用已发布 GoldSrc_VibeSignatures 数据重新裁剪 catalog；CS/CZ 组包含
  `GameStudioRenderer__StudioDrawPlayer`。上游 finder 已存在，无需复制槽位或新增扫描回退。
- CZ 10210 Windows / `cs_italy_cz`：真实 DLL+catalog 加载后，WinDbg 确认内层 hook
  非空、外层 hook 为空。玩家 #4 的 render entity sentinel 为 4，稳定帧 origin 为
  `(-528,-1568,-203.969)`，AABB 为 `(-541.109,-1579.849,-235.927)` 至
  `(-500.925,-1556.973,-176.154)`，R_CullBox=0。已观察到 CT BOT 模型可见，并检查
  转向、移动、回合切换后的画面；不是完整任期模式路线或所有模型/死亡状态回归。
- 长时间调试暂停后曾遇到整帧黑色截图，重启进程后恢复；视觉检查使用重启后的进程。
- CS 1.6 10210 Windows / `cs_italy`：删除用户指定的旧 AVX2 DLL 后确认加载新 baseline。
  本地玩家更新栈为 `UpdateBonesForRagdoll -> GameStudioRenderer__StudioDrawPlayer ->
  GameStudioRenderer_StudioSetupBones`，sentinel=1、flags=0x20。原版 CS 无 BOT；
  此项仅验证本地玩家骨骼路径，未验证多人/demo 中的其他玩家显示。
- 两次调试均已清除断点、分离调试器并退出测试游戏。新 baseline DLL/PDB 和 catalog
  留在本机 czero/cstrike 测试安装中；不代表已发布插件版本。未实机验证旧引擎或 CZDS。

2026-10-18，MetaHookSv #930，Blue Shift / Opposing Force 客户端 catalog：

- 触发信号：bshift / gearbox 加载 BulletPhysics 时，`g_iUser1` 报客户端模块不在 gamedata。
- 根因：上游原先缺少扩展客户端数据，插件 manifest 也未选择这两个模组。仅发布上游数据
  不会使构建同步器打包它们；消费端需同时加入 bshift / gearbox 的 8684、10210 四个版本。
- 上游 Release Build [39046628417](https://github.com/HLND2T/GoldSrc_VibeSignatures/actions/runs/39046628417)
  与 Deploy Pages [39048030744](https://github.com/HLND2T/GoldSrc_VibeSignatures/actions/runs/39048030744)
  全部成功（含部署后 CDN 字节校验）后，使用线上 index 全新同步、裁剪了 23 个快照。
- `validate-gamedata.py --manifest` 和 `sync-gamedata.py --validate-only` 均通过；四个新增版本
  均保留必需的 `g_iUser1` / `g_iUser2`。bshift 的可选客户端 StudioRenderer 符号正常保留；
  gearbox Windows 使用已有引擎端 hooks，不要求不存在的客户端 StudioRenderer 符号。
- 验证范围：发布数据的同步、裁剪、完整性和 manifest 覆盖；未进行游戏内启动或物理效果验证。
