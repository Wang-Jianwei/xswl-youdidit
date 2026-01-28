# GitHub Codespaces 用户指南

本指南帮助您在 GitHub Codespaces 环境中运行并访问 xswl-youdidit Web 仪表板。

## 快速开始

### 1️⃣ 启动 Web 仪表板

在终端中运行：

```bash
cd /workspaces/xswl-youdidit
./run_web_demo.sh
```

演示程序会：
- ✓ 启动 Web 服务器（监听 0.0.0.0:8080）
- ✓ 注册示例申领者（Worker）
- ✓ 发布示例任务
- ✓ 运行 30 秒
- ✓ 自动关闭

### 2️⃣ 访问仪表板

根据您的运行环境选择访问方式：

#### 🖥️ **本地开发环境**
直接在浏览器中打开：
```
http://localhost:8080
```

#### ☁️ **GitHub Codespaces**

Codespaces 会自动为您的服务器创建一个可公开访问的 URL。有两种方法可以打开它：

**方式一：自动端口转发**
1. 按 `Ctrl+Shift+P` 打开命令面板
2. 搜索 "Ports: Focus on Ports View"
3. 查找端口 8080
4. 右键点击 → "Open in Browser" 或点击"浏览器"图标

**方式二：手动打开 Codespaces URL**
- Codespaces 会在输出中显示一个 URL，格式如：
  ```
  https://your-username-xxxxxxx-8080.app.github.dev
  ```
- 直接在浏览器中打开此 URL

### 3️⃣ 查看仪表板

Web 仪表板将显示：

- 📊 **平台指标**
  - 总任务数
  - 已完成任务数
  - 申领者数
  - 平均处理时间

- 📝 **任务列表**
  - 任务 ID 和标题
  - 优先级
  - 申领状态

- 👷 **申领者列表**
  - 申领者名称
  - 已处理任务数
  - 当前状态

- 📋 **事件日志**
  - 最近事件记录
  - 事件时间戳

## 🔧 REST API 端点

如果您想直接访问数据，可以使用以下 REST API：

```bash
# 获取 HTML 仪表板
curl http://localhost:8080

# 获取 JSON 格式的指标
curl http://localhost:8080/api/metrics

# 获取任务列表
curl http://localhost:8080/api/tasks

# 获取申领者列表
curl http://localhost:8080/api/claimers

# 获取事件日志
curl http://localhost:8080/api/logs
```

## ⚙️ 配置说明

### 主机绑定

Web 服务器默认绑定到 `0.0.0.0:8080`，这允许：
- ✅ 本地访问 (`127.0.0.1:8080`)
- ✅ Codespaces 外部 URL 访问
- ✅ 网络上其他机器访问

### 为什么使用 0.0.0.0？

在 GitHub Codespaces 中：
- Codespaces 的端口转发功能需要服务器监听 `0.0.0.0`
- 仅在 `127.0.0.1` 上监听的服务器无法通过外部 URL 访问
- `0.0.0.0` 包含本地访问，因此向后兼容

## 🐛 故障排除

### 问题：浏览器显示"当前无法处理此请求"

**原因**：服务器未运行或端口被占用

**解决方案**：
```bash
# 检查端口是否被占用
lsof -i :8080

# 如果有进程占用，可以杀死它
kill -9 <PID>

# 重新启动演示
./run_web_demo.sh
```

### 问题：Codespaces 端口转发不工作

**原因**：可能是网络连接问题

**解决方案**：
1. 检查 Codespaces 中的 "Ports" 视图
2. 确保端口 8080 显示为 "Public" 而不是 "Private"
3. 如果是 Private，右键点击端口并选择 "Make Public"

### 问题：仪表板不显示任何数据

**原因**：演示程序可能已完成（默认运行 30 秒）

**解决方案**：
重新运行演示程序：
```bash
./run_web_demo.sh
```

## 📚 更多信息

- 查看 [API.md](API.md) 了解完整的 API 文档
- 查看 [README.md](README.md) 了解项目概述
- 查看 [docs/WEB_API.md](docs/WEB_API.md) 了解 Web API 详情

## 🎉 就这样！

现在您可以在 GitHub Codespaces 中充分利用 xswl-youdidit 的 Web 仪表板功能了！

如有问题，请参考故障排除部分或查阅项目文档。
