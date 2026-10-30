# GDED｜经验蒸馏记录规范 R1

## 原则
**经验不是“好建议”列表，而是有原始证据、结果和适用条件的决策事件。** 禁止把我们的推导描述为开发者亲口陈述。

## 单条记录字段
```yaml
id: GDED-0001
title: "待填写"
status: candidate # candidate | source-verified | cross-validated | superseded
source:
  url: ""
  author_or_organization: ""
  published_at: ""
  exact_location: "" # 页码、章节、时间戳、commit、PR
evidence:
  observed_facts: []
  author_claims: []
  our_inferences: []
context:
  project_scale: ""
  discipline: []
  lifecycle_stage: []
  constraints: []
decision:
  trigger_or_problem: ""
  considered_options: []
  selected_action: ""
result:
  reported_outcome: ""
  independent_verification: ""
  failures_or_tradeoffs: []
transfer:
  reusable_rule: ""
  trigger_conditions: []
  non_applicable_conditions: []
  related_tools: []
  related_roles: []
  related_steps: []
review:
  reviewed_at: ""
  confidence: unassessed
```

## 三条经验增长渠道
1. 广域发现：主动收集公开的真实开发记录，不能把搜索结果摘要视为最终证据。
2. 任务触发：当前项目出现未知、失败、方案冲突时专项查找相似案例。
3. 个人实测：从用户项目中的实际变更、测试、试玩和回退形成高情境关联的案例。

## 质量要求
一手资料优先；多来源交叉验证；记录反例、时效性、情境差异；保留原始链接和原文定位；不使用“行业标准”替代证据。