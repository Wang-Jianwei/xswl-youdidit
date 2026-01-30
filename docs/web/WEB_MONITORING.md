# Web 监控系统设计文档

本文档详细描述 xswl-youdidit 的 Web 监控系统架构设计、部署模式和技术实现。

## 目录

- [设计理念](#设计理念)
- [架构概览](#架构概览)
- [部署模式](#部署模式)
- [核心组件](#核心组件)
- [跨进程通信设计](#跨进程通信设计)
- [与外部系统集成](#与外部系统集成)

---

## 设计理念

### 解耦原则

Web 监控系统遵循**单一职责原则**，与核心任务调度系统完全解耦：

| 组件 | 职责 | 依赖关系 |
|------|------|----------|
| `TaskPlatform` | 任务调度与管理 | 核心组件，无外部依赖 |
| `WebDashboard` | 监控、可视化、分析 | 可选组件，依赖 TaskPlatform |
| `MetricsExporter` | 指标导出 | 可选组件，支持跨进程监控 |

### 设计优势

```
┌─────────────────────────────────────────────────────────────────┐
│                        解耦设计优势                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────┐     ┌─────────────────────────────────┐   │
│  │  TaskPlatform   │     │         可选监控组件             │   │
│  │  ┌───────────┐  │     │  ┌─────────────────────────┐    │   │
│  │  │ 任务调度   │  │     │  │ WebDashboard            │    │   │
│  │  ├───────────┤  │     │  ├─────────────────────────┤    │   │
│  │  │ 申领者管理 │  │────▶│  │ MetricsExporter         │    │   │
│  │  ├───────────┤  │     │  ├─────────────────────────┤    │   │
│  │  │ 状态同步   │  │     │  │ Prometheus Integration  │    │   │
│  │  └───────────┘  │     │  └─────────────────────────┘    │   │
│  └─────────────────┘     └─────────────────────────────────┘   │
│                                                                 │
│  ✓ 故障隔离：监控崩溃不影响核心业务                               │
│  ✓ 按需启用：不需要监控时零开销                                   │
│  ✓ 独立升级：可单独更新监控组件                                   │
│  ✓ 灵活部署：支持同进程/独立进程/远程部署                          │
└─────────────────────────────────────────────────────────────────┘
```

---

## 架构概览

### 系统架构图

```
┌─────────────────────────────────────────────────────────────────────┐
│                      Web 监控系统架构                                │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│   ┌─────────────────────────────────────────────────────────────┐   │
│   │                      用户层 (User Layer)                     │   │
│   │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │   │
│   │  │  Web 浏览器  │  │  Grafana    │  │  自定义客户端 │         │   │
│   │  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘         │   │
│   └─────────┼────────────────┼────────────────┼─────────────────┘   │
│             │ HTTP/WS        │ Prometheus     │ HTTP/JSON           │
│             ▼                ▼                ▼                     │
│   ┌─────────────────────────────────────────────────────────────┐   │
│   │                     接口层 (API Layer)                       │   │
│   │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │   │
│   │  │  WebServer  │  │  Prometheus │  │  REST API   │         │   │
│   │  │  (内置 UI)   │  │  Exporter   │  │  Endpoints  │         │   │
│   │  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘         │   │
│   └─────────┼────────────────┼────────────────┼─────────────────┘   │
│             │                │                │                     │
│             ▼                ▼                ▼                     │
│   ┌─────────────────────────────────────────────────────────────┐   │
│   │                    服务层 (Service Layer)                    │   │
│   │  ┌──────────────────────────────────────────────────────┐   │   │
│   │  │                   WebDashboard                        │   │   │
│   │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐      │   │   │
│   │  │  │ TimeReplay │  │ EventLog   │  │ Analytics  │      │   │   │
│   │  │  └────────────┘  └────────────┘  └────────────┘      │   │   │
│   │  └──────────────────────────┬───────────────────────────┘   │   │
│   └─────────────────────────────┼───────────────────────────────┘   │
│                                 │                                   │
│                                 ▼                                   │
│   ┌─────────────────────────────────────────────────────────────┐   │
│   │                    数据层 (Data Layer)                       │   │
│   │  ┌──────────────────────────────────────────────────────┐   │   │
│   │  │                   TaskPlatform                        │   │   │
│   │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐      │   │   │
│   │  │  │   Tasks    │  │  Claimers  │  │ Statistics │      │   │   │
│   │  │  └────────────┘  └────────────┘  └────────────┘      │   │   │
│   │  └──────────────────────────────────────────────────────┘   │   │
│   └─────────────────────────────────────────────────────────────┘   │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### 数据流向

```
TaskPlatform ──┬──▶ WebDashboard ──▶ WebServer ──▶ Web UI
               │
               ├──▶ MetricsExporter ──▶ Prometheus ──▶ Grafana
               │
               └──▶ EventLog ──▶ 日志文件 / 外部日志系统
```

---

## 部署模式

### 模式 1：同进程部署（默认）

最简单的部署方式，适用于开发环境和小规模生产环境。

```cpp
// 同一进程中创建和使用
auto platform = std::make_shared<TaskPlatform>();
auto dashboard = std::make_shared<WebDashboard>(platform.get());
dashboard->start_server(8080);
```

**架构图：**

```
┌─────────────────────────────────────────────────────────┐
│                     单一进程                             │
│  ┌─────────────────┐       ┌─────────────────┐         │
│  │  TaskPlatform   │──────▶│  WebDashboard   │         │
│  │  (任务调度)      │ 指针  │  (Web :8080)    │         │
│  └─────────────────┘       └─────────────────┘         │
└─────────────────────────────────────────────────────────┘
```

**特点：**
- ✅ 配置简单，开箱即用
- ✅ 零网络开销
- ✅ 数据实时同步
- ⚠️ 监控崩溃可能影响主进程

---

### 模式 2：独立进程部署

监控服务与工作服务分离，适用于生产环境。

**工作进程：**
```cpp
// 工作进程：只运行 TaskPlatform + MetricsExporter
auto platform = std::make_shared<TaskPlatform>();
auto exporter = std::make_shared<MetricsExporter>(platform.get());
exporter->start_server(9090);  // 暴露指标端点
```

**监控进程：**
```cpp
// 监控进程：连接远程平台
auto dashboard = std::make_shared<WebDashboard>("http://localhost:9090");
dashboard->start_server(8080);
```

**架构图：**

```
┌──────────────────────┐      ┌──────────────────────┐
│    工作进程           │      │    监控进程           │
│  ┌─────────────────┐ │ HTTP │ ┌─────────────────┐  │
│  │  TaskPlatform   │ │      │ │  WebDashboard   │  │
│  │       │         │ │      │ │       │         │  │
│  │       ▼         │ │      │ │       ▼         │  │
│  │ ┌─────────────┐ │ │──────▶ │ ┌─────────────┐ │  │
│  │ │  Exporter   │ │ │ JSON  │ │ RemoteSource│ │  │
│  │ │  :9090      │ │ │      │ │ └─────────────┘ │  │
│  │ └─────────────┘ │ │      │ │       │         │  │
│  └─────────────────┘ │      │ │       ▼         │  │
└──────────────────────┘      │ │ ┌─────────────┐ │  │
                              │ │ │  Web :8080  │ │  │
                              │ │ └─────────────┘ │  │
                              │ └─────────────────┘  │
                              └──────────────────────┘
```

**特点：**
- ✅ 故障隔离：监控崩溃不影响核心业务
- ✅ 资源隔离：可独立限制 CPU/内存
- ✅ 独立重启：可单独重启监控服务
- ⚠️ 需要额外的网络通信

---

### 模式 3：集中监控部署

多个工作节点由统一的监控中心管理，适用于分布式部署。

**架构图：**

```
┌─────────────────────────────────────────────────────────────────┐
│                        集中监控架构                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   机器 A              机器 B              机器 C (运维中心)      │
│   ┌──────────────┐   ┌──────────────┐   ┌──────────────────┐   │
│   │ Platform-1   │   │ Platform-2   │   │  WebDashboard    │   │
│   │ ┌──────────┐ │   │ ┌──────────┐ │   │  (聚合多平台)     │   │
│   │ │Exporter  │─┼───┤ │Exporter  │─┼──▶│  ┌────────────┐  │   │
│   │ │:9090     │ │   │ │:9090     │ │   │  │  统一仪表板  │  │   │
│   │ └──────────┘ │   │ └──────────┘ │   │  │  :8080      │  │   │
│   └──────────────┘   └──────────────┘   │  └────────────┘  │   │
│                                          └──────────────────┘   │
│   ┌──────────────┐   ┌──────────────┐                          │
│   │ Platform-3   │   │ Platform-4   │                          │
│   │ ┌──────────┐ │   │ ┌──────────┐ │                          │
│   │ │Exporter  │─┼───┤ │Exporter  │─┼──────────────────────────┘
│   │ │:9090     │ │   │ │:9090     │ │
│   │ └──────────┘ │   │ └──────────┘ │
│   └──────────────┘   └──────────────┘
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

**监控中心配置：**
```cpp
// 聚合多个平台的监控数据
auto dashboard = std::make_shared<WebDashboard>(std::vector<std::string>{
    "http://machine-a:9090",
    "http://machine-b:9090",
    "http://machine-c:9090",
    "http://machine-d:9090"
});
dashboard->start_server(8080);
```

**特点：**
- ✅ 统一运维：一个仪表板管理所有平台
- ✅ 跨机房监控：支持远程数据采集
- ✅ 横向扩展：轻松添加新的工作节点
- ⚠️ 网络延迟：数据可能有轻微延迟

---

## 核心组件

### EventLog（事件日志）

负责记录系统中所有重要事件，支持查询和分析。

```cpp
class EventLog {
public:
    // 事件类型
    enum class EventType {
        TaskPublished,       // 任务发布
        TaskClaimed,         // 任务申领
        TaskStarted,         // 任务开始
        TaskProgressUpdated, // 进度更新
        TaskCompleted,       // 任务完成
        TaskFailed,          // 任务失败
        TaskAbandoned,       // 任务放弃
        PriorityChanged,     // 优先级变更
        ClaimerRegistered,   // 申领者注册
        ClaimerStatusChanged // 申领者状态变更
    };
    
    // 事件记录结构
    struct EventRecord {
        EventType type;
        Timestamp timestamp;
        std::string event_id;
        std::string source_id;
        std::map<std::string, std::string> metadata;
        std::string description;
    };
    
    // 事件记录
    void log_event(const EventRecord& record);  // 线程安全
    
    // 事件查询
    std::vector<EventRecord> get_events(const Timestamp &start, const Timestamp &end) const;
    std::vector<EventRecord> get_events_by_source(const std::string &source_id) const;
    std::vector<EventRecord> get_events_by_type(EventType type) const;
    
    // 导出
    std::string export_as_json(const Timestamp &start, const Timestamp &end) const;
    std::string export_as_csv(const Timestamp &start, const Timestamp &end) const;
    
    // 统计
    size_t get_event_count() const;
    size_t get_event_count_by_type(EventType type) const;
    
    // 清理
    void cleanup_events_before(const Timestamp &before);
    
private:
    mutable std::shared_mutex events_mutex_;
    std::deque<EventRecord> events_;
    std::unordered_multimap<std::string, size_t> source_index_;
};
```

### TimeReplay（时间回放）

支持时间旅行调试，查看任意时刻的系统状态。

```cpp
class TimeReplay {
public:
    // 获取指定时刻的系统快照
    std::string get_snapshot_at(const Timestamp &timestamp) const;
    
    // 获取时间段内的事件序列
    std::vector<EventLog::EventRecord> get_events_between(
        const Timestamp &start_time,
        const Timestamp &end_time
    ) const;
    
    // 检查指定时刻的状态
    std::string get_task_state_at(const TaskId &task_id, const Timestamp &timestamp) const;
    std::string get_claimer_state_at(const std::string &claimer_id, const Timestamp &timestamp) const;
    
    // 生成状态演化轨迹
    std::string generate_state_trace(
        const Timestamp &start_time,
        const Timestamp &end_time,
        int snapshot_interval_ms = 100
    ) const;
};
```

### WebServer（Web 服务器）

内置的轻量级 HTTP 服务器，提供 Web UI 和 API。

```cpp
class WebServer {
public:
    WebServer(WebDashboard* dashboard, int port);
    ~WebServer();
    
    void start();
    void stop();
    bool is_running() const;
    
    void set_port(int port);
    int get_port() const;
    
private:
    WebDashboard* dashboard_;
    int port_;
    std::unique_ptr<class HttpServer> http_server_;
};
```

---

## 跨进程通信设计

### 通信协议

跨进程监控使用 HTTP/JSON 协议，简单可靠。

```
┌─────────────────────────────────────────────────────────────────┐
│                      通信协议设计                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   TaskPlatform + MetricsExporter                               │
│   ┌─────────────────────────────────────────────────────────┐   │
│   │                   HTTP Server :9090                      │   │
│   │                                                          │   │
│   │  GET /metrics      → PlatformStatistics (JSON)          │   │
│   │  GET /tasks        → TaskSummary[] (JSON)               │   │
│   │  GET /tasks/{id}   → Task detail (JSON)                 │   │
│   │  GET /claimers     → ClaimerSummary[] (JSON)            │   │
│   │  GET /claimers/{id}→ Claimer detail (JSON)              │   │
│   │  GET /events       → EventRecord[] (JSON)               │   │
│   │                                                          │   │
│   │  WebSocket /ws     → 实时事件推送                         │   │
│   │                                                          │   │
│   └─────────────────────────────────────────────────────────┘   │
│                              │                                  │
│                              │ HTTP/WebSocket                   │
│                              ▼                                  │
│   WebDashboard (RemoteDataSource)                              │
│   ┌─────────────────────────────────────────────────────────┐   │
│   │  定期轮询 /metrics, /tasks, /claimers                    │   │
│   │  WebSocket 接收实时事件                                   │   │
│   │  聚合多个数据源                                           │   │
│   └─────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### MetricsExporter 类

负责将 TaskPlatform 的数据以 HTTP API 形式暴露。

```cpp
class MetricsExporter {
public:
    explicit MetricsExporter(TaskPlatform* platform);
    ~MetricsExporter();
    
    // 启动/停止 HTTP 服务
    void start_server(int port = 9090);
    void stop_server();
    bool is_running() const noexcept;
    
    // 配置
    MetricsExporter &set_update_interval(int milliseconds);
    MetricsExporter &enable_websocket(bool enable = true);
    MetricsExporter &set_cors_origin(const std::string &origin);
    
    // 手动获取数据（用于自定义集成）
    std::string get_metrics_json() const;
    std::string get_tasks_json() const;
    std::string get_claimers_json() const;
    std::string get_events_json(int limit = 100) const;
    
private:
    class Impl;
    std::unique_ptr<Impl> d;
};
```

### DataSource 抽象

WebDashboard 内部使用 DataSource 接口抽象数据来源。

```cpp
class WebDashboard {
public:
    // 同进程模式
    explicit WebDashboard(TaskPlatform* platform);
    
    // 远程模式
    explicit WebDashboard(const std::string &metrics_endpoint);
    
    // 多平台聚合模式
    explicit WebDashboard(const std::vector<std::string> &endpoints);
    
private:
    // 数据源抽象接口
    class DataSource {
    public:
        virtual ~DataSource() = default;
        virtual PlatformStatistics get_statistics() = 0;
        virtual std::vector<TaskSummary> get_tasks() = 0;
        virtual std::vector<ClaimerSummary> get_claimers() = 0;
        virtual std::vector<EventRecord> get_events(int limit) = 0;
    };
    
    // 本地数据源（直接调用 TaskPlatform）
    class LocalDataSource : public DataSource {
        TaskPlatform* platform_;
    public:
        explicit LocalDataSource(TaskPlatform* platform);
        // 实现接口...
    };
    
    // 远程数据源（HTTP 请求）
    class RemoteDataSource : public DataSource {
        std::string endpoint_;
        std::unique_ptr<HttpClient> client_;
    public:
        explicit RemoteDataSource(const std::string &endpoint);
        // 实现接口...
    };
    
    // 聚合数据源（多个远程源）
    class AggregatedDataSource : public DataSource {
        std::vector<std::unique_ptr<RemoteDataSource>> sources_;
    public:
        explicit AggregatedDataSource(const std::vector<std::string> &endpoints);
        // 实现接口...
    };
    
    std::unique_ptr<DataSource> data_source_;
};
```

---

## 与外部系统集成

### Prometheus 集成

支持 Prometheus 指标格式，可接入 Prometheus + Grafana 生态。

```cpp
class TaskPlatform {
public:
    // 输出 Prometheus 格式的指标
    std::string export_prometheus_metrics() const;
};
```

**输出格式：**

```prometheus
# HELP youdidit_tasks_total Total number of tasks by status
# TYPE youdidit_tasks_total gauge
youdidit_tasks_total{status="published"} 15
youdidit_tasks_total{status="claimed"} 30
youdidit_tasks_total{status="processing"} 25
youdidit_tasks_total{status="completed"} 85
youdidit_tasks_total{status="failed"} 10
youdidit_tasks_total{status="abandoned"} 5

# HELP youdidit_claimers_total Total number of claimers by status
# TYPE youdidit_claimers_total gauge
youdidit_claimers_total{status="idle"} 12
youdidit_claimers_total{status="busy"} 8
youdidit_claimers_total{status="offline"} 0

# HELP youdidit_task_duration_seconds Average task duration in seconds
# TYPE youdidit_task_duration_seconds gauge
youdidit_task_duration_seconds 3600

# HELP youdidit_tasks_per_hour Platform throughput
# TYPE youdidit_tasks_per_hour gauge
youdidit_tasks_per_hour 50

# HELP youdidit_task_completion_rate Task completion rate (0-1)
# TYPE youdidit_task_completion_rate gauge
youdidit_task_completion_rate 0.944

# HELP youdidit_platform_uptime_seconds Platform uptime in seconds
# TYPE youdidit_platform_uptime_seconds counter
youdidit_platform_uptime_seconds 9000
```

**Prometheus 配置示例：**

```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'youdidit'
    static_configs:
      - targets: ['localhost:9090']
    metrics_path: '/metrics'
    scrape_interval: 15s
```

### Grafana 仪表板

可导入预配置的 Grafana 仪表板模板。

```json
{
  "dashboard": {
    "title": "xswl-youdidit Monitoring",
    "panels": [
      {
        "title": "Tasks by Status",
        "type": "piechart",
        "targets": [
          { "expr": "youdidit_tasks_total" }
        ]
      },
      {
        "title": "Task Throughput",
        "type": "graph",
        "targets": [
          { "expr": "youdidit_tasks_per_hour" }
        ]
      },
      {
        "title": "Completion Rate",
        "type": "gauge",
        "targets": [
          { "expr": "youdidit_task_completion_rate * 100" }
        ]
      }
    ]
  }
}
```

---

## 版本信息

- **文档版本**: 1.0.0
- **最后更新**: 2026-01-27
- **适用版本**: xswl-youdidit 1.0.0+
