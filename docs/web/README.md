# Web / 监控 文档合集

本目录包含与 Web 监控系统实现与设计相关的文档，并说明如何在构建时启用 Web 子工程。

主要文件：

- `WEB_MONITORING.md` - 监控系统设计与部署模式
- `WEB_API.md` - Web API（C++ 接口与 HTTP REST API）
- `WEB_SERVER_IMPLEMENTATION.md` - Web 服务器实现总结

构建与运行

- 默认：Web 子工程随主仓库一起配置，但在 Windows 工具链上默认关闭 `web/examples` 与 `web/tests`（见 `BUILD_WEB_EXAMPLES` / `BUILD_WEB_TESTS`）。
- 在命令行启用 Web 子工程示例与测试：

```bash
# 启用 Web 与其示例/测试
cmake .. -DBUILD_WEB=ON -DBUILD_WEB_EXAMPLES=ON -DBUILD_WEB_TESTS=ON
cmake --build . -j$(nproc)
```

> ⚠️ Windows 注意事项：若在 Windows 上启用 Web 测试/示例，请确保所用 SDK/(MinGW/MSVC) 提供必要的 Winsock 与网络 API，或手动链接 `ws2_32`、`iphlpapi`。如遇平台兼容问题，可先在 Windows 上仅构建 `youdidit` 核心库并在支持的环境中运行 Web 示例。