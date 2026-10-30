# GDKM-Role 横向地图 R2：跨专业接口与外部工具

研究日期：2026-10-30。状态：岗位职责已依据一手职业资料核查；具体工作步骤、自动化路径和设计建议属于 GameDev Scaffolding 综合推导，尚未在《巨楼》实测。

## 一、角色不等于 Agent

一个岗位可由多个能力组成，一个 Agent 也可调用多个岗位的专业程序。研究重点不只是“谁做什么”，而是：输入、输出、判断权、交接、可自动验证的信息、未知风险，以及与现有领域模型的关系。

| 岗位 | 现实岗位职责证据 | 可细分的工作动作（研究提案） | 可以外部化的认知/验证 |
|---|---|---|---|
| Gameplay Designer | 定义游戏规则、动作与参数；在技术限制下修正规格 | 将体验目标转成操作规则；列玩家可观察结果；设计反例；修订参数 | 规则表、状态机、自动数值测试、试玩遥测 |
| Gameplay Programmer | 让玩家互动与规则在引擎运行；与关卡、AI、艺术及 QA 合作 | 定位已有系统→确认权威状态→选择扩展点→实现→跨系统测试 | LSP/Roslyn、代码引用图、Git diff、Unity PlayMode |
| Level Designer | 根据玩法规格构建动作、事件、空间、条件与挑战；与程序和艺术合作 | 建目标动线→灰盒场景→触发/通行规则→遭遇→试玩→迭代 | 空间连接图、导航可达性、场景检查、试玩轨迹 |
| Technical Artist | 在美术与程序间解决资源技术问题，为艺术团队提供工作流与工具 | 建资产规格→导入检查→批量修复→性能验证→交付 | Blender Python、Unity Importer/AssetPostprocessor、Profiler |
| Technical Animator | 动画与程序间处理工具、性能、技术复杂度 | 骨架约束→动画约定→引擎映射→动作状态→兼容测试 | 骨骼/权重校验、动画导入诊断、运行时姿态测试 |
| Tools Engineer | 面向设计师、美术师、程序员构建/维护合适工具 | 记录真实瓶颈→查现成工具→定义最小能力→验证使用成效 | 工具目录、使用日志、失败统计、自动化测试 |
| QA Tester | 按测试要求重复验证具体行为、记录缺陷、修复后复测 | 设计可复现步骤→记录期望/实际→问题归因→回归 | Unity Test Framework、日志、录像、缺陷追踪 |

## 二、跨专业交付契约：新增“管道内追逐玩家的怪物”

这是推演案例，不是已经完成的实际交付。

| 来源专业 | 交付物 | 接收方 | 需要独立验证的契约 |
|---|---|---|---|
| Gameplay Design | 怪物追逐条件、脱离规则、玩家反制机会 | AI/Gameplay/Level | 规则无冲突、可被玩家理解 |
| Level Design | 管道类型、出入口、宽高、转角、特殊导航节点 | AI/Gameplay/Art | 所有必经入口可达、角色包围盒可通过 |
| Character Art | 角色模型、尺寸、骨架导出物 | Tech Art/Animation | 尺寸、朝向、拓扑、材质及面数符合要求 |
| Technical Animation | 匍匐/转向/攀爬动作及约定 | Gameplay/AI | 骨架映射、动画位移与移动逻辑一致 |
| Gameplay/AI Programming | 追击行为、状态与路径切换 | Level/QA | 复用现有导航与实体状态；无重复空间事实源 |
| Tech Art | 自动导入检查及预算符合性证据 | 全链路 | 资源自动校验成功；异常可定位 |
| QA / Playtest | 可复现失败、状态覆盖、玩家反馈 | 所有相关专业 | 运行结果与目标体验一致，并能够回归 |

说明：实际团队中的岗位合并、职权和交接顺序取决于工作室规模；此处给的是功能级能力切分，不是通用固定组织图。

## 三、发现问题的触发规则（提案）

- 新增敌人后，空间可达性无法确认 → 主动调用关卡/导航图而不是先编写另一套路径系统。
- 新资源导入失败或规格不一致 → 先调用技术美术/导入检查，不立即重做模型。
- 动画滑步 → 联合读取移动速度、动画位移约定与实际运行，而非单独修改动画参数。
- 代码出现新 Manager/Registry → 查询既有权威状态与领域职责，判断是否重复。
- 工作依赖大量人工点击 → 统计高频重复步骤，再由工具工程判断值得自动化与否。
- 单测通过但行为仍不正确 → 交给运行时验证与 QA 复现，不据此宣告完成。

## 四、已查证的岗位与工具来源

- ScreenSkills Games careers taxonomy：https://www.screenskills.com/job-profiles/browse/games/
- Gameplay designer：https://www.screenskills.com/job-profiles/browse/games/design/gameplay-designer/
- Gameplay programmer：https://www.screenskills.com/job-profiles/browse/games/programming/gameplay-programmer/
- Level designer：https://www.screenskills.com/job-profiles/browse/games/design/level-designer/
- Technical artist：https://www.screenskills.com/job-profiles/browse/games/technical-art/technical-artist/
- Technical animator：https://www.screenskills.com/job-profiles/browse/games/animation/technical-animator/
- Tools engineer：https://www.screenskills.com/job-profiles/browse/games/programming/tools-engineer/
- Games tester：https://www.screenskills.com/job-profiles/browse/games/quality-assurance/games-tester/
- Unity AssetPostprocessor：https://docs.unity.com/en-us/engine/6000.0/script-reference/unityeditor/assetpostprocessor

## 五、尚缺的研究证据

ScreenSkills 可以确认实际岗位与合作关系，但它不是足够细的生产操作手册。接下来需要找工作室的一手技术讲解、开发日志、演讲及真实 PR/工具实现，确认各类交付契约在真实项目中如何落实。对 Unity API 也需要在指定 Unity 6.5 版本实测。
