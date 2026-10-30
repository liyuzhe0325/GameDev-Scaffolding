# Research Depth Gate｜停止蜻蜓点水式职业扫描

日期：2026-10-30。状态：研究方法修订。R1–R7 主要属于职业索引和初步工序提案，**不能标成已掌握的专业能力**。

## 为什么修订

原先反复采用“岗位简介→列举工具→提出流程→提交一份文档→换岗位”，造成广度增长但证据与能力深度不足。尤其未回答技术机制、失败边界、真实工作样本、实测结果及与其他岗位的接口细节。这与项目想避免的 LLM 局部脚手架膨胀具有同构风险。

## 每个专业研究的最低深度门槛

1. **真实工作对象**：具体处理哪些工件、数据、状态，不停留在岗位职责；
2. **知识基础**：该专业用到的核心理论、术语、原理、选择条件与反例；
3. **可执行工序**：从原始输入到最终交付的真实动作，包含中间失败与返工；
4. **工具技术内部机制**：软件架构/API、输入输出、版本兼容、限制、真实使用成本；
5. **可验证判断**：哪些事实由编译器/引擎/测试等独立证实，哪些只可推断；
6. **跨专业契约**：状态权威、数据来源、交接规范和可能造成的重复模型；
7. **一手案例**：带出处的工程实践、失败与修复过程，明确案例与我们的推论；
8. **实验/基准**：可复现的目标、对照、指标、失败条件；实测与设想必须分开；
9. **边界/冲突**：什么时候该专业方法不适用，方案之间的权衡；
10. **可交付知识单元**：足以直接用于 Agent 的问题触发条件、查询协议、验证步骤，而非泛泛建议。

## 证据标签

- VERIFIED-SOURCE：已从一手资料核对事实与原始出处。
- INFERRED：经过推导但尚无直接案例/试验支持。
- EXPERIMENT-PLANNED：提出实验，尚未实施。
- EXPERIMENT-OBSERVED：有实际运行记录、环境与版本证据。
- SUPERSEDED：被更新证据或决策替代。

严禁用“已完成 Rn”混淆“已建立目录”“已完成深度研究”“已通过工程验证”。

## 深度样例：已有代码库新增功能

以《巨楼》门与建筑为例，把核心疑问逐层分析：

- **静态语法**：类、字段、接口、方法在哪里；
- **编译语义**：符号、引用、依赖与调用者；
- **领域语义**：这些类型对应同一事实还是不同模型；
- **职责权威**：谁拥有永久结构、动态状态、派生数据；
- **运行事实**：开门、闭门、锁门后导航、碰撞、事件是否实际一致。

Roslyn Workspace 能维护项目、文档、语义模型以及更新事件，但不能凭空决定业务概念是否重复。Aider Repository Map 只提供高价值导航摘要，不是完整架构真相。领域驱动设计强调多个限界上下文及它们的明确关系，故多图并存不等于错误。依赖验证可检测某些违反架构约束的引用关系，不能检测所有语义重叠。

## 深度研究引用

- Roslyn Workspace（官方）：https://github.com/dotnet/roslyn/blob/main/docs/wiki/Roslyn-Overview.md
- Aider Repository Map（原项目）：https://aider.chat/docs/repomap.html
- Martin Fowler, Bounded Context：https://martinfowler.com/bliki/BoundedContext.html
- Martin Fowler, Domain Driven Design：https://martinfowler.com/bliki/DomainDrivenDesign.html
- Visual Studio dependency validation（微软官方）：https://learn.microsoft.com/en-us/visualstudio/modeling/validate-code-with-layer-diagrams?view=visualstudio

## 执行方式改变

暂停逐岗位依次发表 R8、R9。先选择一个已有职业节点做专题深入，直到能回答机制、工具内部运作、真实案例、可复现实验、失败边界，再扩展。对原有 R1–R7 统一标注 **INDEX / INITIAL MAPPING**，避免误导。
