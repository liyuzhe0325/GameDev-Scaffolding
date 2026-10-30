# GDKM-Role R4｜关卡设计 × 建筑空间 × AI 导航 × 环境美术

日期：2026-10-30。状态：**基于 ScreenSkills 职业资料及 Unity 官方能力的研究提案**。未检查《巨楼》真实工程；不存在实际完成的导航/建筑测试。

## 核心发现：一处游戏空间具有四种不同描述

1. **设计空间**：玩家路线、可见性、遭遇、风险节奏、绕行策略（关卡设计）。
2. **建筑领域空间**：楼层、房间、墙、门洞、梯井、管道、连通关系及事实权威（项目自身领域模型）。
3. **物理/视觉空间**：网格、Collider、灯光、材质、可见/遮挡关系（环境美术和技术美术）。
4. **可行动空间**：角色体型、NavMesh/Link、动态障碍、特殊动作、规则权限（AI/Gameplay）。

这不是四套应该独立维护的权威空间数据库！应明确每一事实的权威来源及如何派生、校验和同步。

## 横向岗位细分（专业能力节点，不代表必须分别配置 Agent）

| ID | 角色/专业 | 详细工作动作 | 交付物 | 外部事实/自动检查 |
|---|---|---|---|---|
| LD-01 | Gameplay Designer | 定义探索、躲避、绕行、风险反馈的设计目标 | 目标玩法与约束 | 规则规格、试玩指标 |
| LD-02 | Level Designer | 设计空间流线、入口出口、灰盒、遭遇、光线引导、触发区；反复试玩 | 灰盒可玩场景/流线图 | 可达性分析、试玩录屏/路径 |
| LD-03 | Building/System Programmer | 规定房间、开口、门、通道的逻辑含义、状态与权威 | 空间/构件/连通接口 | 概念映射、引用/状态审计 |
| LD-04 | Environment Artist | 将灰盒转成符合氛围且不改变关键可玩几何的环境 | 模块、美术场景、材质 | 网格尺寸、Collider、预算检查 |
| LD-05 | Technical Artist | 建模导入约定、遮挡/性能规范和资产校验 | 自动导入检查/性能记录 | Unity Importer/Profiler、Blender |
| LD-06 | AI Programmer | 选择导航策略、agent 类型、路径/特殊转移与动态阻塞 | 导航配置及行为 | NavMesh/Link、路径状态、实际运行 |
| LD-07 | QA/Playtest | 检查玩家和不同类型 NPC 的全部关键路径、异常通行 | 复现及回归证据 | 路径扫描、碰撞探测、录像 |

岗位来源（职责层面）：
- Level designer：https://www.screenskills.com/job-profiles/browse/games/design/level-designer/
- Environment artist：https://www.screenskills.com/job-profiles/browse/games/art/environment-artist-games/
- AI programmer：https://www.screenskills.com/job-profiles/browse/games/programming/artificial-intelligence-ai-programmer/

注：表中操作级分工与工具化建议是本项目综合方案，不是 ScreenSkills 原文规定。

## 生产步骤与交付契约

### 空间设计 → 逻辑建筑
- 先定义玩家要经过、能够绕开或必须承担风险的关键节点，不先绑定到具体 Unity 组件。
- 用建筑领域模型表达房间/开口/楼梯/管道/特殊链接；明确从灰盒到真实关卡如何追溯 ID。
- 所谓“联通”要区分静态几何连通、门状态许可、角色体型限制、游戏规则许可和 AI 的实际路径可行性。

### 逻辑建筑 → 环境美术
- 各空间模块应携带尺度/接口/通行净空/碰撞约定；美术改动不能隐性改变关键动线。
- 可以优化视觉遮挡、灯光和压迫感，但要回归关键路线/门洞/角色碰撞与摄像机可读性。
- 多种尺度的 NPC 使用同一洞口时，必须分开测试不同体型。

### 建筑/环境 → AI 导航
- Unity 6.5 的 com.unity.ai.navigation 2.0 系列支持 NavMeshSurface、NavMeshLink、Modifier、动态障碍与运行时构建。
- 不能把 NavMesh 当作建筑事实数据库：NavMesh 是**从几何、Agent 类型、障碍和配置派生的可通行表示**，不回答楼层、门洞归属和剧情权限。
- 特殊转移（攀爬、匍匐、跳落、穿门）应明确定义动作完成条件，必要时用 NavMeshLink/自定义导航策略；不是说任何管道都能由普通 NavMesh 自动解决。
- 不同 Agent 可能需要不同 NavMeshSurface/agent 参数；大怪物和匍匐小怪可通行集合不必相同。

### AI/Gameplay → QA
- 用 PathComplete/PathPartial/PathInvalid 判断**当前配置下的路径计算结果**；CalculatePath 可能返回部分路径，不能只看 bool。
- 通过 Physics.CheckCapsule/OverlapCapsule 等对关键狭窄空间做物理净空抽检；一次检测不证明完整动作过程中无碰撞。
- 门开闭、门锁、障碍移动、NavMesh carving/update 的同步状态需在 PlayMode 复测。
- 真实玩家体验问题（路标不清、恐惧感不足、视角误导）不能仅凭路径可达性判定。

## “门与建筑”案例里的三种错误

**错误 A：重复拓扑**：Building 持有 RoomGraph，DoorManager 又从头维护一份 RoomGraph，两者没有权威与同步定义。

**错误 B：错把视觉状态当成逻辑权限**：门扇动画已经开启，但游戏逻辑/导航仍锁定，或反之。

**错误 C：只检查静态连通**：房间逻辑连通、门洞存在，但当前 agent 的 NavMesh 不可达（半径过大、Link 端点未接入、Carving 更新延迟、Collider 阻挡）。

**提案**：从建筑领域事实出发，派生角色/时点相关的通行查询；导航系统维护自己的派生表示和成本，而不是第二套“房间是什么”的权威模型。具体设计必须在阅读 GBIS 代码后决定。

## 外部工具与功能分工

| 问题 | 推荐优先用现成工具 | 局限 |
|---|---|---|
| 代码中谁定义通行与房间？ | LSP/Roslyn/代码图谱 | 不自动理解领域语义 |
| Scene 里哪些门/Prefab 引用了组件？ | Unity AssetDatabase + Editor API | 动态生成对象需运行时补充 |
| 当前 NPC 是否到达目标？ | NavMesh.CalculatePath + PathStatus | 运行成本，不能证明动作/体验 |
| 门是否造成路径封堵？ | NavMeshObstacle carving 或更新 NavMesh（按场景选） | 更新时机、性能和 agent 尺寸限制 |
| 管道是否有物理净空？ | Physics 查询 + Collider | 需要覆盖整个运动轨迹与姿态 |
| 环境美术是否改坏动线？ | 灰盒基准 + 路径回归测试 | 不能替代玩家试玩 |
| 玩家能否看懂路线？ | 试玩录像、观察、用户研究 | 非确定性，需专业判断 |

## 《巨楼》低成本验证实验建议（尚未执行）

建一个由三间房、两道门、一条狭窄管道、一条跨层通道构成的独立测试场景，提供：
- 玩家角色、普通巡逻 NPC、可匍匐 NPC（不同包围盒和移动策略）；
- 四个状态：开门/关门/锁门/临时堵塞；
- 三类检查：建筑连通逻辑、NavMesh 路径状态、物理碰撞/实际动作；
- 预期差异：门逻辑连通不等于此时可通过；普通 NPC 不该穿管道；会匍匐的 NPC 也需真实姿态与动作验证。
- 将预期行为和真实结果落盘，比较纯 LLM 设计与外部空间工具辅助设计的错误数量。

成功标准是三套表示**一致且来源可追溯**，并非强制所有空间语义都塞进一个 Manager。

## 一手工具参考与适用性
- Unity 6000.5 兼容 AI Navigation 2.0.14–2.0.16：https://docs.unity.com/en-us/engine/6000.5/manual/packages-list/packages-all/pack-safe/com-unity-ai-navigation
- Unity 官方 AI Navigation 视频教程（含链接、垂直表面和动态布局）：https://legacy-activation.unity3d.com/resources/ai-navigation-video-tutorial-series
- NavMesh.CalculatePath（注意 partial）：https://docs.unity3d.com/jp/current/ScriptReference/AI.NavMesh.CalculatePath.html
- NavMeshObstacle carving：https://docs.unity3d.com/cn/6000.0/ScriptReference/AI.NavMeshObstacle.html
- Physics.CheckCapsule：https://docs.unity3d.com/cn/2023.2/ScriptReference/Physics.CheckCapsule.html

## 证据状态与后续
已核对官方资料中角色职责和 API 功能；未证明上述“衍生图”就是当前《巨楼》最适合的架构。下一轮需补关卡灰盒→环境美术→重新试玩的真实开发团队复盘，并审查项目现有建筑/资源数据再确定职责契约。