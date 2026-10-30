# PIL｜项目认知层工具调研 R1

状态：候选技术评估，不代表安装、可用性或 Unity 6.5 项目兼容已验证。

## 设计边界
工具负责可重复的事实采集、结构检索、异常证据和验证；LLM 负责因果解释、领域建模、提出方案与例外判断；用户负责重要创作意图和授权。

## 查询与证据的三级模型
1. **全局概览**：模块/程序集/资源图、主要职责、重要接口；尽量不塞完整源文件。
2. **按需结构子图**：指定领域及其调用者、状态读写、依赖与资源绑定；保留具体路径和版本。
3. **权威原始证据**：关键源文件、Prefab/Scene、运行日志、测试与实际场景；不能仅以摘要推断。

## 工具候选及角色
| 工具 | 主要用法 | 优势 | 缺口/注意 |
|---|---|---|---|
| ripgrep / git grep | 快速查名称、常量、硬编码 | 简单透明 | 不理解语义引用 |
| Roslyn / C# LSP | 类型与符号、真实引用 | C# 项目语义可靠 | Unity 生成工程和包引用必须完整 |
| Aider repo map | 大范围摘要、有限 Token 上下文 | 低成本项目导航 | 摘要不是完整代码事实 |
| Codebase-Memory（候选） | 持久图谱、影响查询 | 可复用结构索引 | 实验质量和语言解析需在目标项目重测 |
| Unity AssetDatabase | 文件/资源依赖 | 引擎原生事实 | 不覆盖所有运行时创建和语义关系 |
| Roslyn Analyzer / ArchUnitNET | 静态约束与架构违例 | 可写成回归检查 | 不擅长发现语义相似的重复职责 |
| Unity Test Framework / Profiler | 可运行行为与性能 | 观察实际结果 | 覆盖率与体验仍需分析 |

## 不建议第一版自建
- 新的 C# 编译器索引；
- 新的 Unity 资源数据库；
- 为每个专业分别实现独立事实图；
- 用 LLM 自动维护一份“自称精确”的代码树。

## 值得自建的最小薄层（需实证后决定）
- 聚合现有工具结果的稳定查询接口；
- 给每个事实附上仓库提交、具体文件和时间戳；
- 从领域概念映射到代码符号和 Unity 资源的人工可修订表；
- 由 Git diff 触发轻量架构异常检查。

## 第一个工程研究假设
在同任务、同模型、同代码基线下，使用完整代码地图与架构侦察协议的 Agent，比未配备者产生更少的重复权威状态和不必要新模块；必须同时观察质量、Token、耗时及实际运行结果。

## 来源
- https://aider.chat/2023/10/22/repomap.html
- https://arxiv.org/abs/2603.27277
- https://docs.unity.cn/Manual/roslyn-analyzers.html
- https://github.com/microsoft/Microsoft.Unity.Analyzers
- https://docs.unity3d.com/ScriptReference/AssetDatabase.GetDependencies.html

待办：逐项测量 Unity 6.5 + URP 环境下的实际兼容性、索引增量更新和启动成本。