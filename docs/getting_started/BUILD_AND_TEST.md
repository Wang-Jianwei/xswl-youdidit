# 构建与测试指南

本文档介绍如何使用 xswl-youdidit 项目提供的构建测试脚本。

## 脚本概览

| 脚本 | 功能 | 速度 | 用途 |
|------|------|------|------|
| `quick_test.sh` | 快速编译与基础测试 | 🚀 最快 | 快速验证基础功能 |
| `build_and_test.sh` | 完整构建与测试 | ⚡ 中等 | 完整验证与选项控制 |
| *(清理功能已合并)* | 清理构建产物 | - | 使用 `./build_and_test.sh --clean` |
| `analyze_tests.py` | 测试结果分析 | - | 生成详细测试报告 |

## 快速开始

### 最快速度验证（推荐新手）

```bash
./quick_test.sh
```

输出示例：
```
==> 快速编译...
✓ 编译完成
==> 运行单元测试...
✓ test_types
✓ test_task
✓ test_claimer
✓ test_task_platform
✓ test_web
==> 运行集成测试...
✓ integration_test_workflow
✓ integration_test_web_api
✓ 所有测试通过！
```

### 完整构建与选择性测试

```bash
# 运行所有测试和示例
./build_and_test.sh --all

# 清空后重新构建
./build_and_test.sh --clean

# 仅运行单元测试
./build_and_test.sh --unit

# 仅运行集成测试
./build_and_test.sh --integration

# 构建并运行示例
./build_and_test.sh --examples

# 自定义并行编译数
./build_and_test.sh -j 8
```

## build_and_test.sh 详细选项

### 基础用法

```bash
./build_and_test.sh [选项...]
```

### 完整选项列表

| 选项 | 说明 | 备注 |
|------|------|------|
| `--help` | 显示帮助信息 | - |
| `--clean` | 清空构建目录并重新构建 | 用于完全清理编译 |
| `--unit` | 仅运行单元测试 | 快速检查核心功能 |
| `--integration` | 仅运行集成测试 | 检查模块协作 |
| `--examples` | 构建并运行示例 | 演示具体用法 |
| `--all` | 运行所有测试与示例 | 完整验证 |
| `-j N` | 指定并行编译数 | 默认为 `$(nproc)` |

### 常见用场景

#### 场景 1：第一次设置项目

```bash
# 从 git 克隆后
git submodule update --init --recursive

# 完整构建与验证
./build_and_test.sh --clean --all
```

#### 场景 2：快速验证修改（开发流程）

```bash
# 每次代码修改后
./quick_test.sh
```

#### 场景 3：提交前完整检查

```bash
# 确保所有功能无误
./build_and_test.sh --all
```

#### 场景 4：调试特定模块

```bash
# 仅测试单元测试
./build_and_test.sh --unit

# 运行后查看详细报告
python3 analyze_tests.py build
```

## 测试结果分析

### 使用 analyze_tests.py 生成详细报告

```bash
# 默认分析 build 目录
python3 analyze_tests.py

# 指定自定义构建目录
python3 analyze_tests.py /path/to/build
```

### 输出示例

```
开始运行测试套件...

🧪 运行单元测试...
  ✅ test_types: PASSED
  ✅ test_task: PASSED
  ✅ test_claimer: PASSED
  ✅ test_task_platform: PASSED
  ✅ test_web: PASSED

🔗 运行集成测试...
  ✅ integration_test_workflow: PASSED
  ✅ integration_test_web_api: PASSED

📚 运行示例程序...
  ✅ example_basic_usage: PASSED
  ✅ example_multi_claimer: PASSED
  ✅ example_web_monitoring: PASSED


============================================================
📊 测试报告
============================================================
测试时间: 2026-01-27T09:32:37.722265

总体状态: PASSED
测试总数: 9
通过数: 9
失败数: 0

单元测试: 7/7 通过
集成测试: 2/2 通过
示例程序: 3/3 通过
============================================================

✅ 测试结果已保存至: build/test_results.json
```

### JSON 测试报告

脚本会生成 `build/test_results.json`，包含详细的测试结果：

```json
{
  "timestamp": "2026-01-27T09:32:37.722265",
  "unit_tests": {
    "test_types": {
      "name": "test_types",
      "status": "PASSED",
      "returncode": 0
    }
  },
  "integration_tests": {},
  "examples": {},
  "summary": {
    "total_tests": 9,
    "passed_tests": 9,
    "overall_status": "PASSED"
  }
}
```

## 清理构建

### 清除所有编译产物

```bash
./clean.sh
```

交互式提示：
```
将清除以下目录:
  /workspaces/xswl-youdidit/build

确认删除? (y/N)
```

## 手动命令

如果不使用脚本，也可以手动执行：

### 完整构建流程

```bash
mkdir -p build
cd build
cmake ..
cmake --build . -j$(nproc)
```

### Web 子工程（可选）

本项目把 Web 功能作为独立子工程放在 `web/` 下，可通过 CMake 选项控制：

- `-DBUILD_WEB=ON|OFF`：是否构建 Web 子工程（默认：ON）
- `-DBUILD_WEB_EXAMPLES=ON|OFF`：是否构建 `web/examples/`（Windows 上默认 OFF）
- `-DBUILD_WEB_TESTS=ON|OFF`：是否构建 `web/tests/`（Windows 上默认 OFF）

示例：在构建时启用 Web 与其示例/测试：

```bash
cmake .. -DBUILD_WEB=ON -DBUILD_WEB_EXAMPLES=ON -DBUILD_WEB_TESTS=ON
cmake --build . -j$(nproc)
```

### 强制构建 Web 与指定构建类型

我们在 `build_and_test.ps1` 中实现了自动检测逻辑：在 Windows 或旧编译器上默认会跳过 Web 模块以避免平台不兼容问题。若您确实想在当前环境强制构建 Web，可以：

```powershell
# 强制构建 Web（PowerShell）
.\build_and_test.ps1 -ForceWeb
# 或设置环境变量再运行
$env:FORCE_BUILD_WEB = '1'; .\build_and_test.ps1

# 指定构建类型（Debug / Release / RelWithDebInfo / MinSizeRel）
.\build_and_test.ps1 -BuildType Debug
```

> ⚠️ 注意（Windows）：由于部分第三方库（例如内嵌的 cpp-httplib）对 Windows SDK 有特定要求，默认在 Windows 工具链上关闭 `BUILD_WEB_EXAMPLES` 和 `BUILD_WEB_TESTS`，以避免阻塞主构建。若你需要在 Windows 上开启测试/示例，请确保所用工具链和 Windows SDK 支持 Winsock 与相关 API（或手动链接 `ws2_32` / `iphlpapi`）。

### 手动运行测试

```bash
# 单元测试
./tests/easy-test_types
./tests/easy-test_task
./tests/easy-test_task_builder
./tests/easy-test_claimer
./tests/easy-test_task_platform
./tests/easy-test_thread_safety

# Web 单元测试（若已启用）
# ./web/tests/easy-test_web

# 集成测试
./tests/easy-integration_test_workflow
# Web 集成测试（若已启用）
# ./web/tests/easy-integration_test_web_api

# 示例程序
./examples/easy-example_basic_usage
./examples/easy-example_multi_claimer
# Web 示例（若已启用）
# ./web/examples/easy-example_web_demo

# 使用 CTest
ctest --output-on-failure
```

## 故障排查

### 脚本权限问题

如果脚本无法执行：

```bash
chmod +x *.sh
```

### 编译失败

清理并重新构建：

```bash
./clean.sh
./build_and_test.sh --clean
```

### 某个测试失败

查看详细输出：

```bash
cd build
./tests/[test_name]  # 直接运行测试查看输出
```

## 性能优化建议

### 加速编译

```bash
# 使用最大可用核心数
./build_and_test.sh -j $(nproc)

# 或明确指定
./build_and_test.sh -j 16
```

### 选择性测试

```bash
# 仅运行必要的测试
./build_and_test.sh --unit      # 快速基础检查
./build_and_test.sh --integration  # 仅集成测试
```

## CI/CD 集成建议

### GitHub Actions 示例

```yaml
- name: Quick Test
  run: ./quick_test.sh

- name: Full Test with Report
  run: |
    ./build_and_test.sh --clean --all
    python3 analyze_tests.py build
```

### GitLab CI 示例

```yaml
test:
  script:
    - ./quick_test.sh
```

## 更多信息

- 完整开发计划：[DEVELOPMENT_PLAN.md](docs/architecture/DEVELOPMENT_PLAN.md)
- Phase 报告：
  - [Phase 1](docs/architecture/PHASE1_REPORT.md)
  - [Phase 2](docs/architecture/PHASE2_REPORT.md)
  - [Phase 5](docs/architecture/PHASE5_REPORT.md)
