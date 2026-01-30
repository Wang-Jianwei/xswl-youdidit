# 快速开始

本页涵盖如何构建、运行与测试本项目的基本步骤（Windows / Linux / macOS）。

## 环境要求

- CMake 3.10+
- 支持 C++11 的编译器（GCC / Clang / MSVC / MinGW）
- 可选：`nlohmann/json` 已作为 header-only 集成

## 构建（推荐）

在仓库根目录执行：

Windows (PowerShell)：

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -- -j 4
```

Linux / macOS：

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -- -j $(nproc)
```

## 运行测试

- 使用 CTest：

```bash
cd build
ctest --output-on-failure
```

- 或直接运行生成的测试二进制（位于 `build/tests` 或 `build/bin`，视 CMake 配置而定）

## 运行示例

构建后示例位于 `build/examples`，可直接运行：

```bash
./build/examples/example_basic_usage
```

## 常见任务脚本

- Windows：`build_and_test.ps1`
- Linux/macOS：`build_and_test.sh`

---

如需我把此文档补充更详细的环境安装步骤或示例运行说明（含输出样例），我可以继续完善。