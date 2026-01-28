#!/bin/bash

# xswl-youdidit Web Dashboard 演示脚本
# 此脚本启动Web仪表板并显示可访问的信息

echo ""
echo "========================================"
echo "xswl-youdidit Web Dashboard 演示"
echo "========================================"
echo ""

# 检测运行环境
if [ -n "$CODESPACES" ]; then
    echo "✓ 在 GitHub Codespaces 环境中检测到"
    echo ""
fi

# 构建项目（如果还没有）
if [ ! -f build/examples/example_web_demo ]; then
    echo "正在构建项目..."
    cd build && make example_web_demo > /dev/null 2>&1 && cd ..
fi

if [ ! -f build/examples/example_web_demo ]; then
    echo "✗ 构建失败"
    exit 1
fi

echo "✓ 项目已构建"
echo ""

# 提示用户
echo "即将启动 Web 仪表板演示程序"
echo ""
echo "📝 说明:"
echo "  • 默认在 http://localhost:8080 运行（可用 --port 修改）"
echo "  • 可用 --forever 持续运行，避免端口转发超时"
if [ -n "$CODESPACES" ]; then
    echo "  • Codespaces: 打开 Ports 面板 → 选中端口 → Make Public → Open in Browser"
fi
echo "  • 支持的 REST API 端点:"
echo "    - GET /          - 返回 HTML 仪表板"
echo "    - GET /api/metrics     - 返回 JSON 指标"
echo "    - GET /api/tasks       - 返回任务列表"
echo "    - GET /api/claimers    - 返回申领者列表"
echo "    - GET /api/logs        - 返回事件日志"
echo ""
echo "按 Enter 键启动演示程序（可追加参数，如 --forever --port 3000）..."
read -r

# 启动演示程序
./build/examples/example_web_demo "$@"

echo ""
echo "✓ 演示程序已完成"
echo ""
