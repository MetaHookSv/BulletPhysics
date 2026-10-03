# BulletPhysics 文档

[返回 README](../../README.zh-CN.md) | [English](../en/features.md)

### 功能

1. 玩家死亡时将玩家模型转化为布娃娃

2. 玩家被藤壶或喷火怪抓住时转化为布娃娃

3. 布娃娃受固体实体运动影响

4. 布娃娃可以受水的浮力影响

5. 晃动骨骼

![](../assets/6.png)

![](../assets/7.png)

### 兼容性

源码面向 GoldSrc 和 SvEngine 系列。当前数据覆盖、旧版本客户端数据限制和安装要求见
[安装说明](installation.md)。两个 Sven 版本的可选视角实体槽不同，8948 不发布 10257 的该槽。

### 控制台参数

| 参数 | 默认值 | 用途 |
| --- | --- | --- |
| `bv_simrate` | 64 | 模拟频率，限制在 32–128 |
| `bv_syncview` | 1 | 物理摄像机同步 |
| `bv_force_updatebones` | 1 | 强制更新骨骼 |
| `bv_debug_draw` | 0 | 启用物理调试绘制 |
| `bv_debug_draw_wallhack` | 0 | 穿墙绘制 |
| `bv_debug_draw_level_*` | 1 | 过滤静态/动态/布娃娃/刚体/约束/行为绘制 |

允许 cheats 且 VGUI2Extension UI 可用时，`bv_open_debug_ui` 打开编辑器。
`bv_reload_all`、`bv_reload_objects`、`bv_reload_configs`、`bv_save_configs` 管理对象和配置。

### 当前物理配置

插件先读取 `<model>_physics.txt`（KeyValues），再兼容 `<model>_ragdoll.txt`。
迁入的 KeyValues 示例为 [GFL_M14_physics.txt](../../assets/svencoop_downloads/models/player/GFL_M14/GFL_M14_physics.txt)。
调试编辑器可以保存配置，对应模型文件需另外安装。后续章节说明旧版 ragdoll 格式。

### 如何制作玩家布娃娃

1. 创建名为 `[model_name]_ragdoll.txt` 的文件到 `Sven Co-op\steamapps\common\Sven Co-op\svencoop(_addons, _download)\models\[model_name]\` 目录下, 比如: `\Sven Co-op\svencoop_addon\models\player\GFL_M14\GFL_M14_ragdoll.txt`. 

2. "[model_name]_ragdoll.txt" 格式如下 :

* [旧格式示例：GI_Ganyu_ragdoll.txt](../../assets/svencoop_downloads/models/player/GI_Ganyu/GI_Ganyu_ragdoll.txt)

#### [DeathAnim] 指定哪些动画会将玩家转换为布娃娃

##### 例子: 

`12 120`

** 玩家播放12号动画序列且播放到第120帧时，将玩家转换为布娃娃。播放帧数的可选值：0 ~ 255

** 帧数的取值范围是0~255，而不是HLMV中显示的帧数！你需要将HLMV中的帧数以如下形式转换： (帧数 * 255 / 最大帧数)

** 只有activity以 "DIE_" 开头的动作才会被当作死亡动画处理。

动画序列可以通过HLMV或[HLAM](https://github.com/SamVanheer/HalfLifeAssetManager)查看

#### [RigidBody] （刚体）

##### 例子: 

`Head`   `15  6`  `sphere`  `-4.0`   `5.0   0.0`   `6`

"Head" 指刚体的内部名称

"15" 指当前刚体绑定第15号骨骼

"6" 指当前刚体朝向第6号骨骼

"sphere" 指刚体的形状，可选值：sphere （球体）或capsule（胶囊体）

"-4.0" 刚体以15号骨骼的位置为原点，向朝向6号骨骼方向上的偏移距离

"5.0" 指刚体大小

"0.0" 对capsule（胶囊体）来说指高度，对sphere（球体）无作用

"6" 指当前刚体的质量（mass）

** 这条例子创建了一个名为 "Head" 的刚体在 15号骨骼 处, 向6号骨骼偏移 "-4.0" 。刚体的大小是 5.0  **

使用 HLMV 或 HLAM 查看骨骼索引，再通过 `bv_debug_draw 1` 检查配置的物理形状。

#### [Constraint] （约束）

##### 例子: 

`Spine  Head`   `conetwist` `6 15`   `0 6 0`     `0  0 -5.5`      `0.05 0.05 0.2`

"Head" 指该约束体控制的第一刚体名称

"Spine" 指该约束体控制的第二刚体的名称

"conetwist" 指当前约束体类型为锥形扭转约束，可选值有point（点约束），conetwist（锥形扭转约束）和hinge（铰链约束）、point_collision、conetwist_collision、hinge_collision

"6" 指该约束体使用6号骨骼的朝向（即旋转矩阵）作为初始朝向（前提："0" "6" "0"三个数字不是全0）

"15" 指该约束体使用15号骨骼的朝向（即旋转矩阵）作为初始朝向（前提："0" "0" "-5.5"三个数字不是全0）

"0" "6" "0" 指刚体"Head"为中心进行的偏移量，使用的是本地坐标系而非世界坐标系，该约束的朝向（即旋转矩阵）则是使用的6号骨骼的朝向（前提："0" "6" "0"三个数字不是全0）

"0" "0" "-5.5" 指刚体"Spine"为中心进行的偏移量，使用的是本地坐标系而非世界坐标系，该约束的朝向（即旋转矩阵）则是使用的15号骨骼的朝向（前提："0" "0" "-5.5"三个数字不是全0）

"0.05" "0.05" "0.2" 指约束的限制参数

** 这条例子为 "Head" 和 "Spine" 两个刚体创建了一个 "锥形扭转约束", 上下摇摆弧度限制到0.05 PI（Π）, 左右摇摆弧度限制到 0.2 PI（Π） **

#### [Barnacle] （藤壶）

##### 例子: 

`LLeg2`   `dof6`     `0  8  0`     `40000`    `4`   `16`

"LLeg2" 指藤壶拉起布娃娃时控制的刚体对象

"dof6" 指约束体类型( dof6 或 slider ) 或者 chew (每 X 秒施加一个向世界坐标系Z方向向上的固定大小的力，模拟藤壶咬人时的上下运动).

"0" "8" "0" 指约束体在 LLeg2 在本地坐标系上的偏移

"40000" 指布娃娃被向上拉时施加的力的大小，力越大加速度越大，也与重力跟布娃娃总质量有关系。布娃娃Z轴超过玩家实体真实Z轴时会被强制减速，太大的加速度会导致布娃娃上下抖动，具体最佳数值需要自己不断测试。

"4" 指约束体最高能把该刚体吊升至相对藤壶坐标多高的位置，4 指 藤壶Z坐标+4处。

"16" 指初始状态下最高的吊升高度相对4的偏移，16指初始吊升高度 = 4 - 16 = -12。

** 这个例子为 “LLeg2” 创建了一个 "6自由度" 类型的约束, 并且以40000 N的力向上吊起 "LLeg2" 。最高可以升至 (藤壶Z坐标 - 12) 的高度，并且后续可以利用chewlimit继续抬升至最大高度。 **

##### 例子2:

`Pelvis`  `chewforce`     `0  0  0`     `8000`     `1.0`  `0`

"Pelvis" 指力施加到的刚体对象

"0" "0" "0" 指力的施加位置，本地坐标系，0 0 0指刚体中心

"8000" 指力的大小

"1.0" 指每秒1次施加作用力

0 在此处无作用

** 该例子会在藤壶播放咀嚼动画(barnacle_chew)时对 "Pelvis" 施加8000大小的力，每秒一次。 **

** 警告：要正确产生藤壶抓取布娃娃的物理效果，布娃娃必须存在名为 "Pelvis" 的刚体 **

##### 例子3: 

`LLeg2`  `chewlimit`     `0  0  0`     `0`     `1.0`  `3`

"LLeg2" 指作用的刚体对象

"0" "0" "0" 在此处无作用

0 在此处无作用

"1.0" 指每秒1次施加作用力

3 指每1秒会让该刚体最大高度提升3，直至真正的最大值。

** 该例子会在藤壶播放咀嚼动画(barnacle_chew)时每秒对 "Pelvis" 提升一次的最大高度。 **

** 警告：要正确产生藤壶抓取布娃娃的物理效果，布娃娃必须存在名为 "Pelvis" 的刚体 **

#### [WaterControl]

"WaterControl" 控制布娃娃与水的交互，每一行代表一个检测点，当检测点在水中时刚体会受到浮力和阻力。

##### 例子 : 

`Head`    `0  0  0`    `0.95`  `0.5`  `0`

`Head` 是刚体内部名称

`0 0 0` 刚体的本地坐标系(0, 0, 0)坐标的位置, 当该坐标接触水时整个刚体会受到向上的浮力。

`0.95` 表示刚体受到的浮力为它所受重力的 95%.

`0.5` 表示刚体在水中受到的阻力，取值范围0 ~ 1，越大阻力越强。

`0` 在此处无作用

*** 每一个刚体可以添加多个检测点

##### 例子 :

```
[WaterControl]
Head    0  0  0    0.95  0.5  0
Spine   0  0  0    0.95  0.5  0
Pelvis  0  0  0    0.95  0.5  0

LArm    0  0  0    0.95  0.5  0
LArm2   0  0  0    0.95  0.5  0
LHand   0  0  0    0.95  0.5  0

RArm    0  0  0    0.95  0.5  0
RArm2   0  0  0    0.95  0.5  0
RHand   0  0  0    0.95  0.5  0

LLeg    0  0  0    0.475 0.5  0
LLeg    8  0  0    0.475 0.5  0
LLeg2   0  0  0    0.475 0.5  0
LLeg2   10 0  0    0.475 0.5  0

RLeg    0  0  0    0.475 0.5  0
RLeg    8  0  0    0.475 0.5  0
RLeg2   0  0  0    0.475 0.5  0
RLeg2   10 0  0    0.475 0.5  0

FakehairL1    0 0 0    0.95  0.5  0
FakehairL2    0 0 0    0.95  0.5  0
FakehairL3    0 0 0    0.95  0.5  0
FakehairR1    0 0 0    0.95  0.5  0
FakehairR2    0 0 0    0.95  0.5  0
FakehairR3    0 0 0    0.95  0.5  0
```
