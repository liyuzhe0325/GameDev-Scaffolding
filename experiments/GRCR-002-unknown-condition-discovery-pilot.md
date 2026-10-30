# GRCR-002｜未知条件发现：资料增强前后对照（人工试点）

日期：2026-10-30。**实验等级：人工研究演示，不是独立 Agent 盲测、统计实验或 Unity 实机验证。** 目的只在于展示搜索如何修正初始条件模型。

## 固定目标
怪物在狭窄管道内追逐玩家；实际抵达、移动不穿模、玩家可观察。未指定必须采用 NavMesh。

## 第一阶段：初始候选条件（检索前）
C1 能识别/跟踪目标。
C2 能寻找目标路线。
C3 体型与管道几何相容。
C4 有管道移动及动作。
C5 能切换普通/管道追逐状态。
C6 玩家能感知追逐。

**初始条件的问题**：C2–C5 是宽泛主题而非独立可验证的谓词；没有区分当前项目是否已满足和是否必需。

## 第二阶段：使用官方资料做专项检索

| 官方证据 | 发现/修正的条件 | 判定 |
|---|---|---|
| Unity 6000.5 NavMesh.CalculatePath：true 包含 partial，必须检查 NavMeshPath.status | 若使用 NavMesh，路径查找成功不足以证明目标可达 | 方案特定的验证必要条件 |
| AI Navigation 官方文档：NavMeshLink 可桥接区域并有 Agent 类型、双向性等配置 | 若用链接跨区域，必须确认 agent 能使用链接、方向和端点 | 方案特定；不是必须采用 NavMeshLink |
| Unity 6000.0 CharacterController.Move：运动受碰撞约束且返回 CollisionFlags | 在实际体型和运动轨迹下验证通过，包括入口和转弯 | 目标条件的具体可测试化；不是必须用 CharacterController |
| Unity 6000.5 NavMeshAgent.updatePosition、OnAnimatorMove：可显式协调导航模拟与角色位移 | 若导航和 Root Motion 同时参与，明确位移所有权/同步机制 | 方案特定的架构条件 |

## 第三阶段：条件模型修订

- 仍保留 C1 目标识别，但应区分“已知目标位置”“目标实时可感知”和“追踪失败的行为”。
- C2 改为“选定方案能够在实际当前拓扑和权限下获得可执行到目标的路径”；若采用 NavMesh，检查 complete/partial/invalid。
- C3+C4 改为“角色在入口、狭窄段、转角、出口和过渡阶段，实际碰撞运动、动画姿态协同成立”；验证需真实运行，不能单凭模型尺寸。
- C5 增加“特殊段失败、堵塞、失去目标时如何恢复”，属于由专业推理提出、待证实的潜在条件，**不宣称来源直接规定**。
- 若采用 NavMeshLink / Root Motion，加入相应条件；换方案时移除这些特定依赖。
- C6 仍需玩家/观察验证，不能由导航测试单独证明。

## 本次结果（定性，不能当作统计改善）
**从宽泛条件中得到四项更具体的外部证据约束**：完整路径、连接适用性、碰撞运动、位移控制权威。另提出一项待证实风险：状态失败恢复。没有声称发现四个全新绝对必要条件，更不能据此证明 GRCR 优于普通 Agent。

## 检验是否只是堆叠条件
- NavMesh 并非绝对必要；
- NavMeshLink 并非绝对必要；
- CharacterController 并非绝对必要；
- Root Motion 并非绝对必要；
- 能抵达、能实际移动、行为可理解才是用户目标下的约束。

## 研究局限
1. 初始清单与后续审查由同一研究者整理，存在主观选择偏差。
2. 没有真实工程索引，无法判断哪些条件已经由项目满足。
3. 未证明每项约束是本游戏的真实瓶颈。
4. 未做搜索预算/Token/时间测量，也没有运行基线对照。
5. Unity 6000.5 的 API 文档只支持能力事实，不证明所有替代方案可行。

## 一手资料
- Unity 6000.5 NavMesh.CalculatePath：https://docs.unity.com/en-us/engine/6000.5/script-reference/unityengine/ai/navmesh/calculatepath
- Unity 6000.5 NavMeshAgent 属性：https://docs.unity.com/en-us/engine/6000.5/script-reference/unityengine/ai/navmeshagent
- Unity 6000.5 updatePosition：https://docs.unity.com/zh-cn/engine/6000.5/script-reference/unityengine/ai/navmeshagent/updateposition
- Unity 6000.5 OnAnimatorMove：https://docs.unity.com/en-us/engine/6000.5/script-reference/unityengine/monobehaviour/onanimatormove
- Unity CharacterController.Move：https://docs.unity.com/en-us/engine/6000.0/script-reference/unityengine/charactercontroller/move
- Unity AI Navigation 2.0.16 兼容性：https://docs.unity.com/en-us/engine/6000.5/manual/packages-list/packages-all/pack-safe/com-unity-ai-navigation

## 下一次真正需要的实验
相同模型、相同问题、相同知识起点，A 组直接搜索，B 组用 GRCR 条件图引导搜索；盲审不可缺少条件、错误必须条件、冗余扩展和验证设计。重复多个任务并用项目实际运行验证后，才能评估触手机制是否增加有效条件发现率。