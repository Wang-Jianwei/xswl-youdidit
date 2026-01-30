# 贡献指南 (中文)

感谢你对项目的贡献！请遵循以下流程可以让审查更顺畅：

## 基本流程

1. Fork 仓库并创建分支：`git checkout -b feat/your-feature` 或 `docs/restructure` 等以功能命名的分支。
2. 在本地运行测试：`cmake --build build --target test` 或 `ctest` 确保没有回归。
3. 提交：使用清晰的 commit message（如 `docs: reorganize docs` 或 `fix: 修正链接`）。
4. 发起 PR，描述变更目的与影响范围，若涉及文档，请在 PR 描述中列出更新的文件和主要改动点。

## 代码规范

- 本项目以 C++11 为目标，遵循 `API.md` 中的编码规范（命名、noexcept、Pimpl 等）。
- 写单元测试覆盖新逻辑或修复的边界情况。

## 文档

- 文档以中文为主（必要时可添加英文摘要），使用简洁明了的标题与目录层次。
- 新增或重命名文档请更新 `docs/README.md`。

## 其他

- 如果你对改动有疑问，请先在 issue 中讨论实现思路。
- 对于大型结构性改动（例如重构或接口变更），建议先提交设计草案（RFC/issue），并征求核心维护者意见。