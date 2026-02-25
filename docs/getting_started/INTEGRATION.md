# 宿主工程集成指南

本文提供两种推荐集成方式：

1. 源码集成（`add_subdirectory`）
2. 安装包集成（`find_package`）

并给出依赖模式（`auto/system/vendor`）的选型建议，适配“宿主工程已有同名第三方库”的场景。

---

## 方式一：源码集成（add_subdirectory）

适用场景：

- 你可以直接访问本库源码目录
- 需要和宿主工程一起调试/联调
- 希望最少安装步骤

宿主工程 `CMakeLists.txt` 示例：

```cmake
cmake_minimum_required(VERSION 3.10)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 假设把本库放在 third_party/xswl-youdidit
# 如果你希望关闭可选子模块，可先设置下面这些变量：
#   - XSWL_YOUDIDIT_BUILD_WEB    OFF
#   - XSWL_YOUDIDIT_BUILD_TESTS  OFF
#   - XSWL_YOUDIDIT_BUILD_EXAMPLES OFF
# 请在 add_subdirectory 之前设置，以便选项生效。
set(XSWL_YOUDIDIT_BUILD_WEB    OFF CACHE BOOL "" FORCE)
set(XSWL_YOUDIDIT_BUILD_TESTS  OFF CACHE BOOL "" FORCE)
set(XSWL_YOUDIDIT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(third_party/xswl-youdidit)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE xswl::youdidit)
```

`main.cpp` 最小示例：

```cpp
#include <xswl/youdidit/youdidit.hpp>

int main() {
    xswl::youdidit::TaskPlatform platform;
    platform.set_name("my-platform");
    return platform.name().empty() ? 1 : 0;
}
```

---

## 方式二：安装包集成（find_package）

适用场景：

- 你希望以已安装库的方式复用
- 适合 CI/CD 和发布环境
- 宿主工程与本库解耦更清晰

### 第一步：构建并安装本库

在本库根目录执行：

```bash
cmake -S . -B build -DXSWL_YOUDIDIT_DEPS_MODE=auto
cmake --build build
cmake --install build --prefix /your/install/prefix
```

Windows PowerShell 示例：

```powershell
cmake -S . -B build -DXSWL_YOUDIDIT_DEPS_MODE=auto
cmake --build build --config Release
cmake --install build --config Release --prefix D:/libs/xswl-youdidit
```

### 第二步：在宿主工程中 find_package

宿主工程 `CMakeLists.txt` 示例：

```cmake
cmake_minimum_required(VERSION 3.10)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 方式 A：命令行传入 -DCMAKE_PREFIX_PATH=/your/install/prefix
# 方式 B：在此处 append 到 CMAKE_PREFIX_PATH

find_package(xswl-youdidit CONFIG REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE xswl::youdidit)
```

---

## 依赖模式如何选

本库支持 `XSWL_YOUDIDIT_DEPS_MODE`：

- `auto`（默认，推荐）：优先复用宿主依赖，缺失时回退 vendored，支持离线构建
- `system`：强制使用宿主/系统依赖，不回退 vendored
- `vendor`：强制使用本库 `third_party`，最稳妥离线

### 冲突规避建议

- 宿主已统一管理第三方：优先 `system`
- 宿主依赖不稳定或离线环境：优先 `auto`
- 强隔离、可重复构建优先：使用 `vendor`

`system` 模式示例：

```bash
cmake -S . -B build \
  -DXSWL_YOUDIDIT_DEPS_MODE=system \
  -DXSWL_YOUDIDIT_TL_OPTIONAL_INCLUDE_DIR=/path/to/includes \
  -DXSWL_YOUDIDIT_TL_EXPECTED_INCLUDE_DIR=/path/to/includes \
  -DXSWL_YOUDIDIT_SIGNALS_INCLUDE_DIR=/path/to/includes \
  -DXSWL_YOUDIDIT_NLOHMANN_INCLUDE_DIR=/path/to/includes \
  -DXSWL_YOUDIDIT_HTTPLIB_INCLUDE_DIR=/path/to/includes
```

---

## 快速自检（推荐）

本仓库已内置 smoke 检查：

- Linux/macOS（或 Git Bash）：`./build_and_test.sh --smoke`
- Windows PowerShell：`.\build_and_test.ps1 -Smoke`

它会执行：安装本库 → 创建最小 consumer → `find_package` 编译，验证集成链路可用。