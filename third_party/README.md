# Third Party Dependencies

本目录包含 xswl-youdidit 项目的所有第三方依赖库。

## 依赖列表

### 1. tl::optional
- **版本**: Latest from master
- **许可证**: CC0 (Public Domain)
- **用途**: C++11 兼容的 optional 实现
- **仓库**: https://github.com/TartanLlama/optional
- **引入方式**: Header-only

### 2. tl::expected
- **版本**: Latest from master
- **许可证**: CC0 (Public Domain)
- **用途**: 错误处理类型（类似 Rust 的 Result）
- **仓库**: https://github.com/TartanLlama/expected
- **引入方式**: Header-only

### 3. xswl-signals
- **版本**: Latest
- **许可证**: MIT
- **用途**: 信号槽机制实现
- **仓库**: https://github.com/Wang-Jianwei/xswl-signals
- **引入方式**: Git submodule

### 4. nlohmann/json (可选)
- **版本**: Latest from develop
- **许可证**: MIT
- **用途**: JSON 序列化/反序列化
- **仓库**: https://github.com/nlohmann/json
- **引入方式**: Header-only

## 更新依赖

### 更新 xswl-signals (git submodule)
```bash
git submodule update --remote third_party/xswl_signals
```

### 更新其他 header-only 库
重新下载对应的头文件即可。

## 许可证说明

所有第三方库的许可证都与本项目兼容。详情请参阅各库的许可证文件。
