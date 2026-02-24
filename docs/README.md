# 文档总览（中文）

本仓库文档以中文为主，面向三类读者：**使用者（Getting Started / 使用示例）**、**开发者（实现细节 / API）** 和 **维护者（运维 / 脚本 / 待办）**。下文为各个文档子目录的快速索引与简短说明，便于定位所需内容。

---

## 快速开始与用户指南
- `docs/getting_started/BUILD_AND_TEST.md` — **构建与测试指南**（如何构建、运行单元/集成测试、示例运行）。
- `docs/getting_started/INTEGRATION.md` — **宿主工程集成指南**（`add_subdirectory` / `find_package` 双路径模板）。
- `docs/usage.md` — **使用示例集**（常见示例、演示代码片段、examples/ 目录示例索引）。
- `docs/contributing.md` — **贡献指南**（如何撰写文档、提交 PR、文档风格与语言规则）。

## API 与接口参考
- `docs/api/API.md` — **完整 API 参考**（类、方法、参数与返回值，带示例与行为说明）。
- `docs/web/WEB_API.md` — **Web/HTTP API 参考**（如果你集成监控或远程指标，请查看）。

## 架构与设计文档
- `docs/architecture/DEVELOPMENT_PLAN.md` — 项目开发计划与分阶段任务（Roadmap & Acceptance Criteria）。
- `docs/architecture/CONCEPTS.md` — **核心概念与设计说明**（任务模型、状态机、信号语义、线程安全策略）。
- `docs/architecture/PHASE*_REPORT.md` — 阶段性报告与验收记录（Phase 1/2/5 等）。

## Web / 监控
- `docs/web/WEB_MONITORING.md` — **监控系统设计**（部署模式、架构图、跨进程通信）。
- `docs/web/WEB_SERVER_IMPLEMENTATION.md` — Web 服务器实现总结与实现细节（性能、路由、UI）。
- `docs/web/WEB_FEATURE_SUMMARY.md` — Web 功能概览与已实现特性。

## 运维 / Codespaces / 脚本
- `docs/ops/CODESPACES_GUIDE.md` — 在 GitHub Codespaces 中快速启动并访问 Web 仪表板（端口转发等）。
- `docs/ops/CODESPACES_FIX_SUMMARY.md` — 与 Codespaces 相关的修复与说明。
- `docs/scripts/SCRIPTS_GUIDE.md` — 脚本系统技术说明（CI 集成、调试脚本）。
- `docs/scripts/BUILD_AND_TEST.md`（位于 `docs/getting_started/`）和 `docs/scripts/SCRIPTS_*` — 脚本总览、清单与示例。

## 开发者资源
- `docs/developer/DEVELOPER_GUIDE.md` — **开发者指南**（示例、Claimer API 快览、实现注意事项、C++ 特性约定）。

## 维护与改进清单
- `docs/maintenance/todo.md` — 待办事项、优先级与改进提案（包括 executor、日志/Trace、异步执行建议）。

## 工具与归档
- `docs/archive/` — 历史或已归档的过时文档（按需保留）。

---

说明：
- 我已把仓库根目录里原本分散的文档移动到 `docs/` 下的合适子目录，并修正了 README 与文档中相互引用的链接。
- 若你希望，我可以：
  - 添加 CI（GitHub Actions）用于自动 **检查 Markdown 链接有效性**；
  - 生成一个静态文档预览（本地或 GH Pages 示例）；
  - 或按需拆分 `docs/developer` 下的子主题（例如把“信号语义”单独成文档）。

---

需要我继续完成哪项？（回复：添加链接检查 / 生成预览 / 拆分信号语义 / 都要 / 先不做）