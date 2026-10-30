# 深度专题 01｜在 Unity 既有工程中新增功能：代码语义、序列化、领域状态和影响验证

**性质**：机制级研究；非《巨楼》实测。日期 2026-10-30。所有工程结构均为示例，不能当成 GBIS 代码事实。

## 研究问题与反例

给拥有建筑结构、门洞与连通关系的工程增加开锁门：为何编码 Agent 会建出独立 DoorRegistry/ConnectionGraph，怎样凭外部证据在变更前阻止无依据的重复抽象？

**错误根因不等于‘类多’**：合法的静态连通图和依角色/时间变化的导航图可以并存。危险的是两个模块都拥有同一个事实并能独立更改、没有清楚的同步契约。

## 机制 I：代码分析分成三个层次

1. **语法事实**：利用 Roslyn SyntaxTree / Tree-sitter 得到类型、成员、语法节点；不推断跨文件同名方法是同一符号。
2. **C# 编译语义**：通过 Workspace（Solution→Project→Document）、Compilation、SemanticModel、ISymbol、SymbolFinder 获得真实符号身份和引用；需要能正确加载 Unity 生成的工程/asmdef 定义及条件编译，否则结果可能缺失。
3. **领域语义**：'Opening'、'Door'、'Passage' 是几何、构件、权限还是导航边？编译器无法确定。必须建立带证据、可修订的概念/职责表，不能把方法命名近似当作重复事实的证据。

## 机制 II：Unity 项目不仅是 C# 类型图

Unity 项目中多个引用空间相互交叠：

- **程序集引用**：asmdef 与编译后代码能引用哪些程序集。
- **源码引用**：属性、调用、事件、继承、泛型；静态可提取部分，反射/动态装配不完备。
- **Unity Asset 依赖**：AssetDatabase.GetDependencies 返回资产路径层的依赖，而不是 Scene 里具体哪个 GameObject 使用该材质或脚本。
- **Unity 对象引用**：MonoBehaviour、Prefab 嵌套与 override、Scene 引用、ScriptableObject、GUID/fileID，需要 Unity 编辑器级对象解析。
- **运行时关系**：Instantiate、动态加载、AddComponent、生成式建筑、运行中激活的 door/blocker；仅靠静态项目图无法覆盖。

Unity 序列化是强约束：自定义可序列化类默认 inline，多个引用可能在反序列化后变成独立副本；SerializeReference 引用共享范围限定在 host 内；跨 host 共享应考虑 ScriptableObject 或其他明确对象所有权。**Unity 官方明确建议避免序列化重复数据或缓存数据，以降低不一致和迁移风险**。这对 Agent 意外创建重复状态非常相关。

原始证据：
- https://docs.unity.com/en-us/engine/6000.5/manual/scripting/compilation-and-code-reload/script-serialization/rules
- https://docs.unity.com/en-us/engine/6000.6/script-reference/unityeditor/assetdatabase/getdependencies
- https://unity.com/blog/engine-platform/understanding-unitys-serialization-language-yaml

## 机制 III：跨层改动的事实所有权矩阵

| 事实 | 候选权威 | 可派生的使用者 | 不应自动假定 |
|---|---|---|---|
| 墙体与洞口几何 | 建筑/构件数据 | 环境渲染、碰撞、路径生成 | DoorManager 需要维护第二套洞口表 |
| 门扇开闭、锁定、损坏 | 门构件或交互状态 | 动画、音效、物理通行 | Animator 参数就是权威状态 |
| 哪些空间永久相邻 | 建筑拓扑 | 可达性判定、规划 | 导航图可反向改写建筑拓扑 |
| 当前角色是否能通过 | 计算：拓扑+门状态+体型+规则+障碍 | AI 路径与交互反馈 | 只有一个全局布尔 isPassable 就足够 |
| 玩家已发现的通道 | 玩家认知/进度 | 地图 UI、提示 | 玩家认知等于几何现实 |

上表是候选领域模型，需要先查看实际代码与设计决定才能落地。缓存/派生图合理，但需记录更新源、时序、失效策略与一致性测试。

## 机制 IV：分层的影响分析查询协议

### A. 先问“目标行为”
玩家接近门；解锁/打开；在不同角色与情境下能否穿越；门状态变化后导航与碰撞是否一致。

### B. 追溯事实
查询角色、门、建筑、空间、交互、导航、序列化、Prefab/Scene。每个查询记录符号完整名称、路径、仓库 SHA、是否有动态边未能覆盖。

### C. 画最小影响切片
- 代码：写状态的方法→所有读取者→外部事件/Unity 生命周期；
- 资产：Prefab 的组件/引用/变体和 Scene 的实际实例；
- 运行：门的激活、导航更新、角色移动和动画。

### D. 至少比较两种方案
延伸既有建筑构件和门交互机制 vs 新建门子领域。不是规定“禁止新类”，而是逐项比较事实源数量、边界、序列化兼容、迁移、测试和性能。

### E. 验证矩阵
| 场景 | 结构事实 | 动态状态 | 预期验证 |
|---|---|---|---|
| 无门洞 | 不相邻 | 任意 | 不可穿 |
| 有开口、门锁闭 | 静态相邻 | 锁闭 | 当前角色不能穿，行为/反馈一致 |
| 门开启 | 静态相邻 | 开启 | 根据体型/其他障碍决定可通行 |
| 门关合中 | 静态相邻 | 过渡 | 明确更新时机、碰撞与导航同步 |
| 特殊小体型 NPC | 相同开口 | 相同门状态 | 通行结果可能与大体型 NPC 不同 |
| 资源实例覆盖 | 相同 Prefab 类型 | 不同实例状态 | 实际 Scene/Prefab override 不串扰 |

测试通过只能覆盖已测试的行为；应该列明没有覆盖的动态载入、序列化迁移和导航策略。

## 工具与非工具的清晰边界

| 观察问题 | 首选外部机制 | 仍需推理 |
|---|---|---|
| 哪些类被哪些代码引用 | Roslyn / LSP / ripgrep | 确认条件编译、反射等遗漏 |
| 哪些资源依赖哪些资源 | Unity AssetDatabase | Scene 内对象级/运行时关系 |
| Prefab 的具体对象/override | Unity Editor 对象扫描/PrefabUtility | 动态创建和设计语义 |
| asmdef 有无循环/越层引用 | 程序集图及自定义架构规则 | 合理领域边界 |
| 两个 Registry 是否重复同一事实 | 状态读写查询 + 人工概念表 | 语义、更新策略、技术权衡 |
| 运行时是否真的可穿门 | Unity PlayMode + 场景冒烟 | 是否符合玩家体验 |

## 第一次应由 Codex 执行的只读技术实证

只读查询：列出 Unity 版本、asmdef、建筑/门/开口/导航相关符号、它们的读写者、Prefab/Scene 绑定及证据路径；再写出现有结构中的冲突**候选**，不得直接进行重构。对每一项结论记录三种状态：已由工具确认、代码推断、尚待运行测试。

**成功标准**：发现真实的现有系统与被遗漏依赖；明确事实权威；不会为只读分析又发明大型框架；结果能引导下一次有证据的最小修改。

## 技术/案例资料

- Microsoft Roslyn SDK / Workspace 概览：https://learn.microsoft.com/en-us/dotnet/csharp/roslyn-sdk/work-with-workspace
- Unity 6000.5 序列化规则：https://docs.unity.com/en-us/engine/6000.5/manual/scripting/compilation-and-code-reload/script-serialization/rules
- Unity 资产依赖：https://docs.unity.com/en-us/engine/6000.6/script-reference/unityeditor/assetdatabase/getdependencies
- Unity Prefab YAML 原理：https://unity.com/blog/engine-platform/understanding-unitys-serialization-language-yaml
- Martin Fowler 限界上下文：https://martinfowler.com/bliki/BoundedContext.html

## 尚未解决
缺少对真实 GBIS 源码的审阅、Unity 6000.5 中实际工具运行数据、具体跨实例序列化和性能测试。不能把本研究写成“已解决门与建筑的现有架构问题”。
