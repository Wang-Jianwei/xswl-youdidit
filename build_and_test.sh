#!/bin/bash

# xswl-youdidit 构建与测试脚本
# 用法: ./build_and_test.sh [选项]
# 选项:
#   --help          显示帮助信息
#   --clean         清空构建目录并重新构建
#   --unit          仅运行单元测试
#   --integration   仅运行集成测试
#   --examples      构建并运行示例程序
#   --all           运行所有测试与示例
#   -j N            指定并行构建数（默认 $(nproc)）

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_ROOT/build"

# 默认参数
PARALLEL=$(nproc)
CLEAN_BUILD=false
RUN_UNIT=true
RUN_INTEGRATION=true
RUN_EXAMPLES=false

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        --help)
            echo "xswl-youdidit 构建与测试脚本"
            echo ""
            echo "用法: $0 [选项]"
            echo ""
            echo "选项:"
            echo "  --help          显示本帮助信息"
            echo "  --clean         清空构建目录并重新构建"
            echo "  --unit          仅运行单元测试"
            echo "  --integration   仅运行集成测试"
            echo "  --examples      构建并运行示例程序"
            echo "  --all           运行所有测试与示例"
            echo "  -j N            指定并行构建数（默认 $(nproc)）"
            exit 0
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --unit)
            RUN_UNIT=true
            RUN_INTEGRATION=false
            RUN_EXAMPLES=false
            shift
            ;;
        --integration)
            RUN_UNIT=false
            RUN_INTEGRATION=true
            RUN_EXAMPLES=false
            shift
            ;;
        --examples)
            RUN_UNIT=false
            RUN_INTEGRATION=false
            RUN_EXAMPLES=true
            shift
            ;;
        --all)
            RUN_UNIT=true
            RUN_INTEGRATION=true
            RUN_EXAMPLES=true
            shift
            ;;
        -j)
            PARALLEL="$2"
            shift 2
            ;;
        *)
            echo "未知选项: $1"
            echo "使用 --help 查看帮助信息"
            exit 1
            ;;
    esac
done

# 打印带颜色的消息
print_step() {
    echo -e "${BLUE}==>${NC} $1"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}!${NC} $1"
}

# 清理构建目录
if [ "$CLEAN_BUILD" = true ]; then
    print_step "清空旧的构建目录..."
    rm -rf "$BUILD_DIR"
fi

# 创建构建目录
if [ ! -d "$BUILD_DIR" ]; then
    print_step "创建构建目录: $BUILD_DIR"
    mkdir -p "$BUILD_DIR"
fi

# 进入构建目录
cd "$BUILD_DIR"

# 读取 CMake 写入的前缀文件（若存在）
if [ -f "easy_executable_prefix.txt" ]; then
    EASY_PREFIX="$(cat easy_executable_prefix.txt)"
else
    EASY_PREFIX="easy-"
fi

# 配置 CMake
print_step "配置 CMake..."
if cmake "$PROJECT_ROOT"; then
    print_success "CMake 配置成功"
else
    print_error "CMake 配置失败"
    exit 1
fi

# 编译项目
print_step "编译项目 (并行数: $PARALLEL)..."
if cmake --build . -j "$PARALLEL"; then
    print_success "编译成功"
else
    print_error "编译失败"
    exit 1
fi

# 运行单元测试
if [ "$RUN_UNIT" = true ]; then
    print_step "运行单元测试..."
    echo ""
    
    tests=(
        "test_types"
        "test_task"
        "test_task_builder"
        "test_claimer"
        "test_task_platform"
        "test_thread_safety"
        "test_web"
    )
    
    failed_tests=()
    
    for test in "${tests[@]}"; do
        if [ -f "./tests/easy-$test" ]; then
            exe="./tests/easy-$test"
        elif [ -f "./tests/$test" ]; then
            exe="./tests/$test"
        else
            continue
        fi
        echo -n "  运行 $test... "
        if "$exe" > /dev/null 2>&1; then
            print_success "通过"
        else
            print_error "失败"
            failed_tests+=("$test")
        fi
    done
    
    if [ ${#failed_tests[@]} -eq 0 ]; then
        print_success "所有单元测试通过"
        echo ""
    else
        print_error "以下单元测试失败:"
        for test in "${failed_tests[@]}"; do
            echo "    - $test"
        done
        echo ""
    fi
fi

# 运行集成测试
if [ "$RUN_INTEGRATION" = true ]; then
    print_step "运行集成测试..."
    echo ""
    
    int_tests=(
        "integration_test_workflow"
        "integration_test_web_api"
    )
    
    failed_int_tests=()
    
    for test in "${int_tests[@]}"; do
        if [ -f "./tests/easy-$test" ]; then
            exe="./tests/easy-$test"
        elif [ -f "./tests/$test" ]; then
            exe="./tests/$test"
        else
            continue
        fi
        echo -n "  运行 $test... "
        if "$exe" > /dev/null 2>&1; then
            print_success "通过"
        else
            print_error "失败"
            failed_int_tests+=("$test")
        fi
    done
    
    if [ ${#failed_int_tests[@]} -eq 0 ]; then
        print_success "所有集成测试通过"
        echo ""
    else
        print_error "以下集成测试失败:"
        for test in "${failed_int_tests[@]}"; do
            echo "    - $test"
        done
        echo ""
    fi
fi

# 构建并运行示例
if [ "$RUN_EXAMPLES" = true ]; then
    print_step "构建示例程序..."
    if cmake --build . --target example_basic_usage example_multi_claimer example_web_monitoring -j "$PARALLEL"; then
        print_success "示例程序编译成功"
        echo ""
        
        examples=(
            "example_basic_usage"
            "example_multi_claimer"
            "example_web_monitoring"
        )
        
        for example in "${examples[@]}"; do
            if [ -f "./examples/easy-$example" ]; then
                exe="./examples/easy-$example"
            elif [ -f "./examples/$example" ]; then
                exe="./examples/$example"
            else
                continue
            fi
            print_step "运行示例: $example"
            if "$exe"; then
                print_success "$example 执行成功"
            else
                print_warning "$example 执行返回非零退出码"
            fi
            echo ""
        done
    else
        print_error "示例程序编译失败"
        exit 1
    fi
fi

# 运行完整测试套件（使用 ctest）
if [ "$RUN_UNIT" = true ] && [ "$RUN_INTEGRATION" = true ]; then
    print_step "使用 CTest 运行完整测试套件..."
    if ctest --output-on-failure; then
        print_success "完整测试套件通过"
    else
        print_warning "部分测试失败（详见上方输出）"
    fi
    echo ""
fi

# 生成摘要
print_step "构建测试完成"
echo ""
echo "摘要:"
echo "  构建目录: $BUILD_DIR"
echo "  并行构建数: $PARALLEL"
[ "$RUN_UNIT" = true ] && echo "  单元测试: 已运行"
[ "$RUN_INTEGRATION" = true ] && echo "  集成测试: 已运行"
[ "$RUN_EXAMPLES" = true ] && echo "  示例程序: 已运行"
echo ""
print_success "完成！"
