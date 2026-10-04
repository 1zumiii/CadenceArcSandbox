# CadenceArc Sandbox

简体中文 | [English](README.en.md)

CadenceArc Sandbox 是一个基于 Unreal Engine 5.7 的 C++ 宿主项目，用于开发、集成和验证 [CadenceArc](https://github.com/1zumiii/CadenceArc) 连招解析框架。

CadenceArc 根据可配置的动作图解析语义化的输入 Tag，并向外部执行系统发出动作请求。框架核心不依赖 Gameplay Ability System、动画蒙太奇、碰撞检测或伤害计算。

## 仓库关系

```text
CadenceArcSandbox
`-- Plugins/
    `-- CadenceArc/    # Git 子模块
```

两个仓库各自维护历史：

- `CadenceArc` 包含可复用的运行时类型、解析器逻辑和框架自身的测试。
- `CadenceArcSandbox` 包含宿主项目、演示资产、测试用 Gameplay Tag 和本地开发工具。

Sandbox 通过子模块指针记录所使用的 CadenceArc 提交。

## Sandbox 提供的内容

- 可游玩的演示：使用 Enhanced Input 和基于 Timer 的演示执行器驱动 CadenceArc，包含按住和蓄力输入；
- 蓝图执行器示例：复用 C++ 演示角色，在事件图中处理动作请求，并发送 GAS Gameplay Event；
- 用于调试器的动作图，以及一张故意配错、用于资产校验的图；
- 运行插件自动化测试的 PowerShell 脚本。

框架的功能、当前状态和接口约定见[插件 README](Plugins/CadenceArc/README.md)。

## 演示与时间约定

启动地图和游戏地图都是 `Content/Demo/Maps/L_CadenceArcDemo`。`ACadenceArcDemoCharacter` 通过 `UCadenceArcInputConfig` 把 Enhanced Input 的 Input Action 映射为语义化的 Gameplay Tag。`UCadenceArcInputConfig` 继承插件的 `UCadenceArcInputActionSet`，并补充 Mapping Context、移动和重置输入。

插件的 `UCadenceArcInputBinderComponent` 负责绑定输入：

- 每个映射的 `Started`、`Completed` 和 `Canceled` 分别转为角色上 `UCadenceArcComponent` 的按下、松开和取消；
- 绑定时把配置中的输入方式写入组件；
- 角色失去控制器时，取消按住中的输入。

角色实现了 `ICadenceArcInputContextProvider`，按住 W 时提供 `Dir.Forward` 上下文。同一个 Tag 或 Input Action 映射多次时，只绑定第一条并输出警告，因为组件按 Tag 配对按下和松开。

`InputMode` 决定按键如何进入解析器：

| 输入方式 | 行为 |
| --- | --- |
| `PressOnly` | 按下时提交 |
| `HoldRelease` | 按下时申请按住资格，在松开或自动释放时结算；当前节点没有该 Tag 的 `Released` 转移时拒绝申请 |
| `HoldIfAvailable` | 当前节点有该 Tag 的 `Released` 转移时等待松开，否则按下立即提交；不预判松开时的条件能否满足 |

角色上的 `UCadenceArcComponent` 来自插件，持有解析器和动作图。它为每次调用填入 World 游戏时间，逐帧推进按住时间，配对按下和松开，并通过 `OnActionRequested` 发出所有动作请求。

`UCadenceArcDemoExecutorComponent` 只负责执行：订阅 `OnActionRequested`，用 Timer 模拟动作，并向组件报告开始、缓冲窗口和完成。这套模拟动作的 Timer 位于 Sandbox，解析器本身不读取引擎时间。`UCadenceArcComponent` 在 `EndPlay` 时取消仍在追踪的输入，不会补发松开。

演示内容位于 `Content/Demo`：

| 资产 | 说明 |
| --- | --- |
| `Maps/L_CadenceArcDemo` | C++ 演示：`BP_CadenceArcDemoCharacter`，使用基于 Timer 的执行器 |
| `Maps/L_CadenceArcBlueprintDemo` | 蓝图示例：`BP_CadenceArcBlueprintDemo` 继承演示角色，关闭 C++ 执行器（`bAutoExecute = false`），在事件图中执行请求并发送真实的 GAS Gameplay Event |
| `Input/DA_CadenceArcInputConfig` | Light 为 `PressOnly`，Heavy 为 `HoldIfAvailable`，并包含 Mapping Context、移动和重置输入 |
| `Graphs/DA_ComboGraphTreeConditional` | 32 个节点的分支树，带蓄力档位，部分转移设置了上下文、停顿和优先级条件。两张地图都使用这张图 |
| `Graphs/DA_ComboGraphCombo` | 接近实际游戏的 Light/Heavy 连招，包含共享终结技、蓄力档位和回到第一招的循环 |
| `Graphs/DA_ComboGraphStress` | 用于布局压力测试的密集图 |

`Content/Tests/DA_ComboGraphInvalid` 是故意配错的图，用于下文的资产校验检查。

输入配置在角色蓝图中指定，动作图在角色的 `CadenceArc` 组件上设置。`MaxBufferedInputAgeSeconds = 0` 表示不限制缓冲时长；设为正数时，完成回调消费缓冲时会检查缓冲输入是否超过这个时长。

解析器的每次调用和结果都可以在编辑器的 **Arc Debugger** 和 **Arc History** 标签页中查看（**Tools > Debug**），截图见[插件的调试器文档](https://github.com/1zumiii/CadenceArc/blob/master/Docs/Debugger.md)。因此，演示执行器只在屏幕上用红字显示解析器无法记录的问题：同一个 Actor 上缺少 CadenceArc 组件，以及执行器时序配置无效。握手失败、调试场景中的拒绝和推进时间失败只以警告写入 `LogCadenceArcDemo`，可以在 Output Log 和 `Saved/Logs/CadenceArcSandbox.log` 中查看。

## 手动检查

- **资产校验**：对 `Content/Demo/Graphs` 中的动作图和 `Content/Tests/DA_ComboGraphInvalid` 运行 Unreal 的数据校验。有效的图应通过校验，错误的图应报告具体的配置问题。
- **PIE 冒烟测试**：确认真实输入、缓冲窗口、连招衔接和日志输出都正常。
- **按住输入**：在有 Heavy 松手档位的节点上，短按触发普通档，满蓄力后松开触发蓄力档；按住超过上限时，蓄力攻击自动释放，之后的物理松开不再产生动作。在没有 Heavy 松手转移的节点上，Heavy 按下立即提交，能否产生动作取决于对应的按下转移及其条件。
- **蓝图示例**：打开 `Maps/L_CadenceArcBlueprintDemo` 并打出一段连招。屏幕上会显示每个请求以及为它收到的 GAS 事件，Arc Debugger 的表现与 C++ 演示相同。
- **调试器**：打开 Arc Debugger 和 Arc History，选择 PIE 中的解析器，然后打出一段连招。已提交的节点、候选请求、预备边和历史记录都应随输入变化；被拒绝的输入应显示为红色记录，并附带原因。

精确的过期边界、无效时间和 Last Input Wins 都由自动化测试覆盖，不需要手动控制亚秒级时序。如果想直观地观察缓冲过期，可以把动作时长设为 6 秒、缓冲窗口设为动作开始后的第 1 秒至第 5 秒、`MaxBufferedInputAgeSeconds` 设为 2 秒：窗口开头的输入会过期，接近窗口末尾的输入不会过期。

## 开始使用

连同插件一起克隆 Sandbox：

```powershell
git clone --recurse-submodules https://github.com/1zumiii/CadenceArcSandbox.git
```

如果克隆时没有包含子模块：

```powershell
git submodule update --init --recursive
```

按需生成项目文件，以 Development Editor 配置编译 `CadenceArcSandboxEditor` 目标，然后用 Unreal Engine 5.7 打开 `CadenceArcSandbox.uproject`。

修改插件前，先确认子模块位于分支上，而不是处于游离的提交：

```powershell
git -C Plugins/CadenceArc switch master
git -C Plugins/CadenceArc pull --ff-only
```

## 运行测试

关闭 Unreal Editor，然后在 Sandbox 根目录执行：

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunCadenceArcTests.ps1
```

脚本会依次完成以下工作：

1. 读取项目的 `EngineAssociation`；
2. 从 Windows 注册表找到对应的 Unreal Engine 安装位置；
3. 冷编译 `CadenceArcSandboxEditor` 目标；
4. 以 `-culture=en` 运行所有名称匹配 `CadenceArc` 的测试；
5. 输出简要的结果摘要；
6. 返回编译或测试的退出码。

常用的可选参数：

```powershell
# 只运行部分测试
.\Scripts\RunCadenceArcTests.ps1 -Filter "CadenceArc.Resolver.Handshake"

# 编译产物与当前源码一致时，跳过构建
.\Scripts\RunCadenceArcTests.ps1 -SkipBuild

# 手动指定引擎位置
.\Scripts\RunCadenceArcTests.ps1 -EngineRoot "E:\Games\UE_5.7"
```

完整的 Unreal 日志位于：

```text
Saved/Logs/CadenceArcSandbox.log
```

### Rider 外部工具

Rider 的 Unreal 测试面板可能把本地化输出的成功测试标记为 Aborted。可以在 `Settings -> Tools -> External Tools` 中配置一个本地快捷方式：

```text
Name:              Run CadenceArc Tests
Program:           C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe
Arguments:         -NoLogo -NoProfile -ExecutionPolicy Bypass -File "<sandbox 绝对路径>\Scripts\RunCadenceArcTests.ps1"
Working directory: <sandbox 绝对路径>
```

外部工具的配置与本机路径有关，不应提交。PowerShell 脚本本身属于本仓库。

## 开发流程

两个仓库都有改动时，按以下顺序提交：

1. 提交并推送 `Plugins/CadenceArc`。
2. 回到 Sandbox 根目录。
3. 提交更新后的 `Plugins/CadenceArc` 子模块指针，以及 Sandbox 的其他改动。
4. 推送 `CadenceArcSandbox`。

这样可以避免 Sandbox 引用一个其他人无法获取的插件提交。

## 仓库边界

Sandbox 专用的地图、输入 Tag 和演示资产放在本仓库。可复用的图类型、解析器行为、校验逻辑和框架自身的测试放在 CadenceArc 插件仓库。

以下内容不属于框架核心：

- Gameplay Ability 和蒙太奇的执行；
- 角色、武器、碰撞和伤害系统；
- WarriorRPG 专用的集成代码。

## 环境要求

- Unreal Engine 5.7
- 对应版本的 Unreal Engine C++ 工具链
- Gameplay Abilities 插件（项目中已启用；蓝图示例和插件的 GAS 执行器模块使用）
- Git
- Git LFS
