# GDKM-Role R3｜角色资产—绑定—动画—技术美术—Gameplay 交付链

日期：2026-10-30。**状态：基于官方文档的专业流程综合研究，尚未对《巨楼》工程运行工具验证。**

## 一、工作对象而不是岗位孤岛

目标产物不是分别“完成模型、骨架、动画和控制器”，而是**一个在实际 Unity 场景中可操作、可运动、可交互且能复现问题的角色**。

同一个角色至少有五个信息层：
1. 外观资产：网格、材质、尺度、坐标约定。
2. 形变结构：骨架、蒙皮权重、可变形范围。
3. 动作资源：Animation Clip、循环、位移、接触时机、动作语义。
4. 引擎状态：Avatar、Animator Controller、Avatar Mask、Prefab/引用。
5. 玩法实体：角色逻辑状态、碰撞体、移动控制、攻击与交互。

**领域权威问题**：不同信息层可以分属不同系统，但一项事实必须注明权威来源。例：角色世界位移到底由 Gameplay 控制器决定、动画 Root Motion 决定，还是经明确的协同机制决定？没有此约定时，常见结果是滑步与重复位移。

## 二、细分岗位与真实可交付项

| ID | 能力/典型专业 | 工作动作 | 上游输入 | 下游交付及检查 |
|---|---|---|---|---|
| CHAR-01 | 角色/概念设计 | 身材比例、装备轮廓、动作需求与视觉目标 | 世界观、镜头、玩法 | 角色规格/动作需求（需人工确认） |
| CHAR-02 | 角色美术 | 网格、UV、材质、统一朝向/尺度 | 角色规格 | 导出网格与参数；可用 Blender/Unity 检查 |
| CHAR-03 | Rigging/皮肤绑定 | 建骨架、命名、权重、姿态与形变测试 | 模型、预计动作 | 骨架结构/权重/形变样本 |
| CHAR-04 | 动画师 | Idle/Walk/Run/Crawl/Attack、起止帧、循环、动作阶段 | Rig 与设计目标 | Clip、位移约定、事件时机与预览 |
| CHAR-05 | 技术动画 | 骨架重定向、Avatar 映射、IK、状态过渡与引擎兼容 | Rig、Clip、角色控制规范 | 可复用 Avatar/Clip/Controller 集成约定 |
| CHAR-06 | 技术美术 | 模型导入、材质映射、资源预算、自动检查 | 各美术交付 | 导入预设、验证结果、异常报告 |
| CHAR-07 | Gameplay/AI 程序 | 输入/AI 状态、移动、碰撞、战斗、技能、动作驱动 | 动作语义与引擎约定 | 可运行逻辑、状态同步与测试 |
| CHAR-08 | QA/玩法测试 | 状态覆盖、复现滑步/穿模/错位、体验反馈 | 已集成场景 | 复现步骤、日志/录像、回归结果 |

角色在小团队中可由一人承担多个工作项；这些是能力节点而非强制独立 Agent。

## 三、必须明确的交付契约

### 契约 A：模型→绑定
- 统一**单位、前向/上方向、原点、角色尺寸、姿势**；不得凭经验硬编码。
- Mesh、Armature、命名、变换、材质和装备插槽约定可工具检查。
- 蒙皮并非“存在权重即可”；须有关键姿势形变质量样本。

### 契约 B：绑定→动画
- 采用哪种骨架标准、能否 Humanoid 重定向、特殊骨是否需独立驱动。
- 每个 Clip 写明用途、是否循环、是否 in-place、根节点轨迹、预期速度区间（若适用）。
- 动作触发与 Gameplay 事件（攻击命中等）必须定义统一时序来源。

### 契约 C：动画→Gameplay
- 首先确定**角色移动的权威来源**：脚本驱动 / Root Motion 驱动 / 明确协调方案，不允许在不知情下叠加。
- 区分移动状态与动画视觉状态的关系：速度、转向、起停、姿态、层级。
- 动画命名和参数名不能散落在多个脚本中手工复制而无检查。
- 测试动作在真实 Character Controller / Collider / 地形中是否稳定。

### 契约 D：技术美术→整个生产线
- 不靠 LLM 逐个观察所有导入：优先探索 Unity AssetPostprocessor/Importer、Blender Python、自动测试。
- 自动检查只断言能定量核实的事实；“动作是否自然”属于需要视觉/体验证据的另一层。

## 四、用滑步问题暴露跨专业缺口

**不要在未取得事实前断言滑步一定由 Root Motion 造成。**

建议的证据序列：
1. 记录真实世界坐标位移和速度（包含帧时间）。
2. 查看 Clip 是否 in-place、Clip/Importer 的 Root Transform 设置。
3. 检查 Animator.applyRootMotion、OnAnimatorMove 与实际移动调用路径。
4. 读取 Animator 状态/Blend Tree 参数变化与角色速度。
5. 对比可见脚部接触节奏和实际位移；区分动画速度不匹配、转弯、地形或重定向问题。
6. 仅修改被证据指向的层；回归慢走、跑步、转向、停止、碰撞和不同模型。

这是一份待实施的专业排查协议；没有宣称已测量《巨楼》的滑步。

## 五、面向外部化的工具研究表

| 认知/验证任务 | 优先外部工具 | 边界 |
|---|---|---|
| 骨架、顶点组、Armature Modifier | Blender Python API / 数据检查 | 不能单凭权重数据判断美感 |
| Humanoid 有效性、Avatar 映射 | Unity ModelImporter、Avatar 配置与诊断 | Avatar 有效不代表动作自然 |
| Generic / 特殊怪物根骨约定 | Unity Rig import 设置 | 需明确不同角色类型 |
| Root Motion / Clip 导入属性 | Unity Animation Importer、Animator 运行记录 | 需实际 Play Mode 确定最终位移 |
| 重复字符串/状态引用 | Roslyn、LSP、静态分析 | 不能自动推断所有领域语义 |
| Scene/Prefab/Clip 资源关系 | AssetDatabase + Editor 自定义查询（如确需） | 运行时动态引用无法完全静态解析 |
| 实际行为 | EditMode/PlayMode、录像与 Profiler | 测试通过不等于玩家体验正确 |

## 六、工作流的最小“完成”定义

**不是**“FBX 可以成功导入”。而是：
- 指定角色/Clip 在目标工程版本的场景中能够正常加载和播放；
- 行走、跑步、停止、转弯至少经过实际运行；
- 与碰撞、地形、动画位移的协同有明确且经验证的策略；
- 记录已测试范围与未测试边界；
- 本次交付不引入第二套未经协调的角色世界位移权威。

## 七、原始参考（官方文档）

- Unity Humanoid Avatar 与重定向：https://docs.unity.com/en-us/engine/6000.0/manual/animation-section/animation-mecanim/avatar-creationand-setup
- Unity Root Motion：https://docs.unity.com/en-us/engine/6000.7/manual/animation-section/animation-mecanim/avatar-creationand-setup/root-motion （文档为后续版本，需核对 6000.5 实际差异）
- Unity Rig/Generic 与根骨：https://docs.unity.cn/Manual/FBXImporter-Rig.html
- Unity AssetPostprocessor：https://docs.unity.com/en-us/engine/6000.0/script-reference/unityeditor/assetpostprocessor
- Blender Armature Modifier：https://docs.blender.org/manual/en/latest/modeling/modifiers/deform/armature.html

## 八、下一轮必须补齐的证据

1. 查找真实工作室/技术动画负责人公开的生产经验，明确上述分工在不同团队规模中的变形。
2. 在《巨楼》Unity 6.5 实测一次真实模型导入和动画运行的数据采集，验证工具能输出哪些事实。
3. 比较通用 Humanoid 与特殊体型怪物 Generic Rig 的生产/复用成本。
4. 检查模型、动画与 Gameplay 之间的接口是否存在重复配置或硬编码。
