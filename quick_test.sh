#!/bin/bash

# xswl-youdidit 快速测试脚本
# 仅编译和运行核心测试，速度最快
# 用法: ./quick_test.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

print_step() {
    echo -e "${BLUE}==>${NC} $1"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

# 创建构建目录
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 快速构建
print_step "快速编译..."
cmake "$SCRIPT_DIR" > /dev/null 2>&1
cmake --build . -j$(nproc) > /dev/null 2>&1
print_success "编译完成"

# 运行快速测试
print_step "运行单元测试..."
failed=0

for test in test_types test_task test_claimer test_task_platform test_web; do
    if [ -f "./tests/$test" ]; then
        if ./tests/$test > /dev/null 2>&1; then
            print_success "$test"
        else
            print_error "$test"
            failed=$((failed + 1))
        fi
    fi
done

print_step "运行集成测试..."
for test in integration_test_workflow integration_test_web_api; do
    if [ -f "./tests/$test" ]; then
        if ./tests/$test > /dev/null 2>&1; then
            print_success "$test"
        else
            print_error "$test"
            failed=$((failed + 1))
        fi
    fi
done

if [ $failed -eq 0 ]; then
    print_success "所有测试通过！"
    exit 0
else
    print_error "$failed 个测试失败"
    exit 1
fi
