#!/bin/bash

# xswl-youdidit 清理脚本
# 清除所有生成的构建产物
# 用法: ./clean.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

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

# 显示即将删除的目录
echo -e "${BLUE}将清除以下目录:${NC}"
echo "  $BUILD_DIR"
echo ""
echo -n "确认删除? (y/N) "
read -r response

if [[ "$response" =~ ^[Yy]$ ]]; then
    print_step "清理构建目录..."
    rm -rf "$BUILD_DIR"
    print_success "清理完成"
    print_success "所有构建产物已删除"
else
    echo "取消清理"
    exit 0
fi
