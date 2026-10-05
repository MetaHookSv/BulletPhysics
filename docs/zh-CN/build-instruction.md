[返回 README](../../README.zh-CN.md) | [English](../en/build-instruction.md)

# 构建说明

## 环境要求与入口

Windows、Visual Studio 2022 C++ 桌面工作负载、Windows SDK、CMake 3.21+、Git 和
Python 3.9+。首次依赖与 gamedata 获取需要网络。

```bat
scripts\build-BulletPhysics-x86-Debug.bat
scripts\build-BulletPhysics-x86-Release.bat
```

脚本使用 `Visual Studio 17 2022 -A Win32` 完成配置、构建和安装，支持从仓库外调用，
失败时返回非零退出码。配置只有 Debug/Release。构建目录为 `build/x86/<配置>`，
安装目录为 `install/x86/<配置>`，不会自动部署到游戏。

## 依赖

显式路径优先并在下载前校验；空路径获取固定提交，但 MetaHook 跟踪最新 `main`。
同名环境变量提供首次配置默认值，后续使用带引号的 `-DNAME=value` 修改缓存。

| 参数 | 源码目录要求 | 默认版本 |
| --- | --- | --- |
| `METAHOOK_SOURCE_PATH` | MetaHook API、HLSDK/SourceSDK/VGUI 源码与 `IPanel2.h` | 最新 `main` |
| `VGUI2EXTENSION_SOURCE_PATH` | `include/Interface` 下的公共接口 | `cd7ef6e3b7fb51d3c98e6d7dadec02dd1fa08c4f` |
| `GLEW_SOURCE_PATH` | 提供 `libglew_static` 的 glew-cmake | `56ed32d4a929f993f0e6b7f905af9be4d38fda04` |
| `BULLET3_SOURCE_PATH` | hzqst Bullet3 fork，含 `src/btBulletDynamicsCommon.h` | `1ece383aeb5a148533ad5c0cd829cd10d38117c3` |

```bat
scripts\build-BulletPhysics-x86-Release.bat ^
  "-DMETAHOOK_SOURCE_PATH=D:/MetaHook" ^
  "-DVGUI2EXTENSION_SOURCE_PATH=D:/VGUI2Extension" ^
  "-DGLEW_SOURCE_PATH=D:/glew-cmake" ^
  "-DBULLET3_SOURCE_PATH=D:/bullet3"
```

符号 API 最低要求 112，但固定 SDK 为 API 115，LoadEngine 要求宿主至少达到编译所用 SDK
版本；指定更高版本 SDK 时，宿主要求也可能提高。

外部源码只读。MetaHook 只提供 SDK，VGUI2Extension 只提供头文件；Bullet/GLEW 在本工程
构建树中静态编译。ScopeExit、tinyobjloader、Chocobo1Hash 为固定提交子模块，配置时初始化。
本工程不需要 SDL、Capstone 或 FreeImage 构建。

VC-LTL 5.3.1 通过 SHA-256 校验后缓存到 `thirdparty/cache/`，Debug/Release 共用。
采用 C++20、`/MTd` / `/MT`，Release 启用 LTO。Bullet 使用单精度，关闭 SDK 安装、
demos、extras 和 Python bindings。

## gamedata

`scripts/manifests/bulletphysics.json` 声明 17 个版本的模块、kind、必需符号与条件分组。
`g_ViewEntityIndex_SCClient` 只在 10257 必需；8948 本来就不发布它。客户端 Studio virtual
保持可选。CS/CZ 客户端快照与对应引擎版本的 catalog 配合使用。

`BULLETPHYSICS_SYNC_GAMEDATA` 默认 `ON`，DLL 构建前同步、裁剪并校验上游数据，
安装到 `svencoop/metahook/gamedata/bulletphysics`，由宿主合并。
原始缓存位于 `build/x86/<配置>/gamedata-sync`，预热后支持离线构建。
设为 `OFF` 时仅安装 `BULLETPHYSICS_GAMEDATA_DIR` 中已有的数据。

## 回归测试

```bat
scripts\build-BulletPhysics-x86-Release.bat -DBULLETPHYSICS_BUILD_TESTS=ON
ctest --test-dir build/x86/Release -C Release --output-on-failure
```

测试位于 `tests/`，直接链接插件自身的目标文件，只替代引擎与 `vgui2.dll` 提供的宿主服务：

- `physic_config_tests`：`*_physics.txt` 保存/读取往返、对象类型与默认值、模型完整性校验、
  旧版 `*_ragdoll.txt` 解析与编辑器另存迁移、配置注册表与编辑器工具函数。
- `shipped_physic_assets_tests`：解析 `assets/svencoop_downloads` 中所有物理配置，检查组件引用、
  OBJ 网格与 Bullet 碰撞体，并确认编辑器另存不丢失数据。
- `bullet_backend_tests`：GoldSrc/Bullet 变换换算、碰撞体与约束参数映射、旧版关节迁移、`bv_simrate` 限幅。

Release 保留断言，不需要游戏 DLL 或 GL context。

## CI 与打包

main 的 push/PR/手动 LiveBuild 和 `v*` 标签 Release 共用 Windows x86 action。
CI 获取 MetaHook/VGUI2Extension 的 main 并记录 SHA，构建 Release、运行 CTest、
校验安装后的 gamedata，将 `svencoop` 和 `svencoop_downloads` 打包为
`BulletPhysics-windows-x86.7z` 并执行 `7z t`。本地自动获取时，MetaHook 取最新 `main`，其余依赖使用固定提交。
