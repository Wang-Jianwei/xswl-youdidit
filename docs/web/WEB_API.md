# Web API 文档

本文档详细描述 xswl-youdidit Web 监控系统的所有 API 接口，包括 C++ 类接口和 HTTP REST API。

## 目录

- [C++ API](#c-api)
  - [WebDashboard 类](#webdashboard-类)
  - [MetricsExporter 类](#metricsexporter-类)
  - [EventLog 类](#eventlog-类)
  - [TimeReplay 类](#timereplay-类)
  - [WebServer 类](#webserver-类)
- [HTTP REST API](#http-rest-api)
  - [指标接口](#指标接口)
  - [任务接口](#任务接口)
  - [申领者接口](#申领者接口)
  - [事件接口](#事件接口)
  - [时间回放接口](#时间回放接口)
  - [数据导出接口](#数据导出接口)
- [WebSocket API](#websocket-api)
- [数据结构定义](#数据结构定义)

---

## C++ API

### WebDashboard 类

Web 仪表板核心类，提供监控数据访问和 Web 服务功能。

```cpp
class WebDashboard {
public:
    // ========== 构造函数 ==========
    
    // 同进程模式：直接传入 TaskPlatform 指针
    explicit WebDashboard(TaskPlatform* platform);
    
    // 远程模式：连接远程 MetricsExporter
    explicit WebDashboard(const std::string &metrics_endpoint);
    
    // 多平台聚合模式：连接多个远程平台
    explicit WebDashboard(const std::vector<std::string> &endpoints);
    
    ~WebDashboard();
    
    // 禁止拷贝
    WebDashboard(const WebDashboard &) = delete;
    WebDashboard &operator=(const WebDashboard &) = delete;
    
    // ========== 服务器控制 ==========
    
    void start_server(int port = 8080);
    void stop_server();
    bool is_running() const noexcept;
    
    // ========== 配置方法（Fluent API）==========
    
    WebDashboard &set_update_interval(int milliseconds);
    WebDashboard &set_log_file_path(const std::string &path);
    WebDashboard &set_max_event_history(size_t max_events);
    WebDashboard &enable_https(const std::string &cert_path, const std::string &key_path);
    
    // ========== 实时监控数据 ==========
    
    // 复用 TaskPlatform::PlatformStatistics
    using DashboardMetrics = TaskPlatform::PlatformStatistics;
    
    DashboardMetrics get_metrics() const;
    std::string get_dashboard_data() const;  // JSON 格式
    
    // ========== 任务信息 ==========
    
    struct TaskSummary {
        TaskId id;
        std::string title;
        std::string category;
        int priority;
        TaskStatus status;
        int progress;
        std::string publisher_id;
        tl::optional<std::string> claimer_id;
        Timestamp created_at;
        tl::optional<Timestamp> completed_at;
    };
    
    std::vector<TaskSummary> get_tasks_summary() const;
    std::string get_task_detail(const TaskId &task_id) const;
    
    // ========== 申领者信息 ==========
    
    struct ClaimerSummary {
        std::string id;
        std::string name;
        std::string role;
        ClaimerState status;
        int active_task_count;
        int max_concurrent_tasks;
        int total_completed;
        int total_failed;
        double success_rate;
        int reputation_points;
        Timestamp last_active_at;
    };
    
    std::vector<ClaimerSummary> get_claimers_summary() const;
    std::string get_claimer_detail(const std::string &claimer_id) const;
    
    // ========== 时间回放 ==========
    
    std::shared_ptr<TimeReplay> get_time_replay() const;
    
    // ========== 事件日志 ==========
    
    std::vector<std::string> get_event_logs(int limit = 100, int offset = 0) const;
    std::vector<std::string> get_event_logs_by_type(const std::string &event_type, int limit = 100) const;
    
    // ========== 性能分析 ==========
    
    struct PerformanceAnalysis {
        std::map<std::string, double> task_duration_by_category;
        std::map<std::string, double> claimer_efficiency;
        double tasks_per_minute;
        double peak_concurrent_tasks;
        double avg_queue_wait_time_seconds;
        double claimer_utilization_rate;
        double system_load_average;
    };
    
    PerformanceAnalysis analyze_performance(
        const Timestamp &start_time,
        const Timestamp &end_time
    ) const;
    
    // ========== 数据导出 ==========
    
    std::string export_as_json(const Timestamp &start_time, const Timestamp &end_time) const;
    std::string export_as_csv(const Timestamp &start_time, const Timestamp &end_time) const;
    std::string generate_report(
        const Timestamp &start_time,
        const Timestamp &end_time,
        const std::string &report_type = "summary"
    ) const;
    
private:
    class Impl;
    std::unique_ptr<Impl> d;
};
```

#### 使用示例

```cpp
// 同进程模式
auto platform = std::make_shared<TaskPlatform>();
auto dashboard = std::make_shared<WebDashboard>(platform.get());
dashboard->set_update_interval(1000)
         ->set_max_event_history(10000)
         ->start_server(8080);

// 获取指标
auto metrics = dashboard->get_metrics();
std::cout << "总任务数: " << metrics.total_tasks << std::endl;
std::cout << "完成率: " << metrics.task_completion_rate * 100 << "%" << std::endl;

// 时间回放
auto replay = dashboard->get_time_replay();
auto snapshot = replay->get_snapshot_at(Timestamp::now() - std::chrono::minutes(30));

// 性能分析
auto analysis = dashboard->analyze_performance(
    Timestamp::now() - std::chrono::hours(1),
    Timestamp::now()
);
```

---

### MetricsExporter 类

用于跨进程监控，将 TaskPlatform 数据以 HTTP API 形式暴露。

```cpp
class MetricsExporter {
public:
    explicit MetricsExporter(TaskPlatform* platform);
    ~MetricsExporter();
    
    // 服务器控制
    void start_server(int port = 9090);
    void stop_server();
    bool is_running() const noexcept;
    
    // 配置
    MetricsExporter &set_update_interval(int milliseconds);
    MetricsExporter &enable_websocket(bool enable = true);
    MetricsExporter &set_cors_origin(const std::string &origin);
    
    // 手动获取数据
    std::string get_metrics_json() const;
    std::string get_tasks_json() const;
    std::string get_claimers_json() const;
    std::string get_events_json(int limit = 100) const;
    
    // Prometheus 格式导出
    std::string export_prometheus_metrics() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> d;
};
```

#### 使用示例

```cpp
// 工作进程
auto platform = std::make_shared<TaskPlatform>();
auto exporter = std::make_shared<MetricsExporter>(platform.get());
exporter->enable_websocket(true)
        ->set_cors_origin("*")
        ->start_server(9090);

// 监控进程（远程连接）
auto dashboard = std::make_shared<WebDashboard>("http://worker-host:9090");
dashboard->start_server(8080);
```

---

### EventLog 类

事件日志记录与查询。

```cpp
class EventLog {
public:
    // 事件类型枚举
    enum class EventType {
        TaskPublished,
        TaskClaimed,
        TaskStarted,
        TaskProgressUpdated,
        TaskCompleted,
        TaskFailed,
        TaskAbandoned,
        PriorityChanged,
        ClaimerRegistered,
        ClaimerStateChanged
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
    std::vector<EventRecord> get_events(
        const Timestamp &start_time,
        const Timestamp &end_time
    ) const;  // 线程安全
    
    std::vector<EventRecord> get_events_by_source(
        const std::string &source_id
    ) const;  // 线程安全
    
    std::vector<EventRecord> get_events_by_type(
        EventType event_type
    ) const;  // 线程安全
    
    // 导出
    std::string export_as_json(const Timestamp &start, const Timestamp &end) const;
    std::string export_as_csv(const Timestamp &start, const Timestamp &end) const;
    
    // 统计
    size_t get_event_count() const;
    size_t get_event_count_by_type(EventType event_type) const;
    
    // 清理
    void cleanup_events_before(const Timestamp &before_time);
    
private:
    mutable std::shared_mutex events_mutex_;
    std::deque<EventRecord> events_;
    std::unordered_multimap<std::string, size_t> source_index_;
};
```

---

### TimeReplay 类

时间回放功能，支持查看任意时刻的系统状态。

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
    std::string get_task_state_at(
        const TaskId &task_id,
        const Timestamp &timestamp
    ) const;
    
    std::string get_claimer_state_at(
        const std::string &claimer_id,
        const Timestamp &timestamp
    ) const;
    
    // 生成状态演化轨迹
    std::string generate_state_trace(
        const Timestamp &start_time,
        const Timestamp &end_time,
        int snapshot_interval_ms = 100
    ) const;
};
```

---

### WebServer 类

内置 HTTP 服务器。

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

## HTTP REST API

以下是 MetricsExporter 和 WebServer 暴露的 HTTP 端点。

### 指标接口

#### GET /api/metrics

获取平台统计指标。

**响应示例：**
```json
{
  "total_tasks": 170,
  "published_tasks": 15,
  "claimed_tasks": 30,
  "processing_tasks": 25,
  "completed_tasks": 85,
  "failed_tasks": 10,
  "abandoned_tasks": 5,
  "total_claimers": 20,
  "idle_claimers": 12,
  "busy_claimers": 8,
  "offline_claimers": 0,
  "avg_task_duration_seconds": 3600.5,
  "tasks_per_hour": 50.2,
  "task_completion_rate": 0.944,
  "claimer_success_rate": 0.895,
  "platform_start_time": "2026-01-27T08:00:00Z",
  "uptime_seconds": 9000,
  "last_update": "2026-01-27T10:30:00Z"
}
```

#### GET /api/dashboard

获取仪表板完整数据（包含任务和申领者概览）。

**响应示例：**
```json
{
  "metrics": { /* PlatformStatistics */ },
  "recent_tasks": [ /* TaskSummary[] */ ],
  "recent_events": [ /* EventRecord[] */ ],
  "claimers_overview": [ /* ClaimerSummary[] */ ]
}
```

#### GET /metrics (Prometheus 格式)

获取 Prometheus 格式的指标。

**响应示例：**
```prometheus
# HELP youdidit_tasks_total Total number of tasks
# TYPE youdidit_tasks_total gauge
youdidit_tasks_total{status="published"} 15
youdidit_tasks_total{status="completed"} 85
youdidit_tasks_total{status="failed"} 10
```

---

### 任务接口

#### GET /api/tasks

获取所有任务列表。

**查询参数：**
| 参数 | 类型 | 说明 |
|------|------|------|
| `status` | string | 按状态筛选 |
| `category` | string | 按分类筛选 |
| `min_priority` | int | 最小优先级 |
| `max_priority` | int | 最大优先级 |
| `limit` | int | 返回数量限制 |
| `offset` | int | 偏移量 |

**响应示例：**
```json
{
  "total": 170,
  "tasks": [
    {
      "id": "task-001",
      "title": "数据处理任务",
      "category": "data_processing",
      "priority": 5,
      "status": "Processing",
      "progress": 75,
      "publisher_id": "publisher-001",
      "claimer_id": "worker-001",
      "created_at": "2026-01-27T09:00:00Z",
      "completed_at": null
    }
  ]
}
```

#### GET /api/tasks/{id}

获取单个任务详情。

**响应示例：**
```json
{
  "id": "task-001",
  "title": "数据处理任务",
  "description": "处理用户数据并生成报告",
  "category": "data_processing",
  "priority": 5,
  "status": "Processing",
  "progress": 75,
  "publisher_id": "publisher-001",
  "claimer_id": "worker-001",
  "required_role": "DataProcessor",
  "tags": ["urgent", "data"],
  "metadata": {
    "input_file": "data.csv",
    "output_format": "json"
  },
  "created_at": "2026-01-27T09:00:00Z",
  "published_at": "2026-01-27T09:01:00Z",
  "claimed_at": "2026-01-27T09:05:00Z",
  "started_at": "2026-01-27T09:06:00Z",
  "completed_at": null,
  "deadline": "2026-01-27T18:00:00Z",
  "reward_points": 100
}
```

---

### 申领者接口

#### GET /api/claimers

获取所有申领者列表。

**查询参数：**
| 参数 | 类型 | 说明 |
|------|------|------|
| `status` | string | 按状态筛选 |
| `role` | string | 按角色筛选 |
| `limit` | int | 返回数量限制 |
| `offset` | int | 偏移量 |

**响应示例：**
```json
{
  "total": 20,
  "claimers": [
    {
      "id": "worker-001",
      "name": "Alice",
      "role": "DataProcessor",
      "status": "Busy",
      "active_task_count": 3,
      "max_concurrent_tasks": 5,
      "total_completed": 150,
      "total_failed": 8,
      "success_rate": 0.949,
      "reputation_points": 4500,
      "last_active_at": "2026-01-27T10:30:00Z"
    }
  ]
}
```

#### GET /api/claimers/{id}

获取单个申领者详情。

**响应示例：**
```json
{
  "id": "worker-001",
  "name": "Alice",
  "role": "DataProcessor",
  "status": "Busy",
  "skills": ["data_analysis", "machine_learning", "python"],
  "categories": ["analytics", "data_processing"],
  "active_task_count": 3,
  "max_concurrent_tasks": 5,
  "claimed_tasks": ["task-001", "task-002", "task-003"],
  "statistics": {
    "total_completed": 150,
    "total_failed": 8,
    "total_abandoned": 2,
    "success_rate": 0.949,
    "reputation_points": 4500,
    "total_rewards": 15000,
    "average_task_duration_seconds": 3200
  },
  "registered_at": "2026-01-15T08:00:00Z",
  "last_active_at": "2026-01-27T10:30:00Z"
}
```

---

### 事件接口

#### GET /api/events

获取事件日志。

**查询参数：**
| 参数 | 类型 | 说明 |
|------|------|------|
| `type` | string | 事件类型筛选 |
| `source_id` | string | 事件源 ID 筛选 |
| `start_time` | ISO8601 | 开始时间 |
| `end_time` | ISO8601 | 结束时间 |
| `limit` | int | 返回数量限制（默认100） |
| `offset` | int | 偏移量 |

**响应示例：**
```json
{
  "total": 1000,
  "events": [
    {
      "event_id": "evt-001",
      "type": "TaskCompleted",
      "timestamp": "2026-01-27T10:35:22Z",
      "source_id": "task-089",
      "metadata": {
        "claimer_id": "worker-005",
        "duration_seconds": "3600"
      },
      "description": "Task 'task-089' completed by 'worker-005'"
    }
  ]
}
```

**事件类型枚举：**
| 类型 | 说明 |
|------|------|
| `TaskPublished` | 任务发布 |
| `TaskClaimed` | 任务申领 |
| `TaskStarted` | 任务开始 |
| `TaskProgressUpdated` | 进度更新 |
| `TaskCompleted` | 任务完成 |
| `TaskFailed` | 任务失败 |
| `TaskAbandoned` | 任务放弃 |
| `PriorityChanged` | 优先级变更 |
| `ClaimerRegistered` | 申领者注册 |
| `ClaimerStateChanged` | 申领者状态变更 |

---

### 时间回放接口

#### GET /api/replay/snapshot

获取指定时刻的系统快照。

**查询参数：**
| 参数 | 类型 | 说明 |
|------|------|------|
| `timestamp` | ISO8601 | 目标时间点 |

**响应示例：**
```json
{
  "timestamp": "2026-01-27T10:00:00Z",
  "snapshot": {
    "tasks": [ /* 该时刻的任务状态 */ ],
    "claimers": [ /* 该时刻的申领者状态 */ ],
    "metrics": { /* 该时刻的统计数据 */ }
  }
}
```

#### GET /api/replay/trace

获取状态演化轨迹。

**查询参数：**
| 参数 | 类型 | 说明 |
|------|------|------|
| `start_time` | ISO8601 | 开始时间 |
| `end_time` | ISO8601 | 结束时间 |
| `interval_ms` | int | 快照间隔（毫秒） |

**响应示例：**
```json
{
  "start_time": "2026-01-27T10:00:00Z",
  "end_time": "2026-01-27T10:30:00Z",
  "interval_ms": 60000,
  "snapshots": [
    { "timestamp": "2026-01-27T10:00:00Z", "metrics": {...} },
    { "timestamp": "2026-01-27T10:01:00Z", "metrics": {...} },
    ...
  ]
}
```

#### GET /api/replay/task/{id}

获取指定任务在指定时刻的状态。

**查询参数：**
| 参数 | 类型 | 说明 |
|------|------|------|
| `timestamp` | ISO8601 | 目标时间点 |

#### GET /api/replay/claimer/{id}

获取指定申领者在指定时刻的状态。

**查询参数：**
| 参数 | 类型 | 说明 |
|------|------|------|
| `timestamp` | ISO8601 | 目标时间点 |

---

### 数据导出接口

#### POST /api/export/json

导出 JSON 格式数据。

**请求体：**
```json
{
  "start_time": "2026-01-27T09:00:00Z",
  "end_time": "2026-01-27T10:00:00Z",
  "include_tasks": true,
  "include_claimers": true,
  "include_events": true
}
```

#### POST /api/export/csv

导出 CSV 格式数据。

**请求体：**
```json
{
  "start_time": "2026-01-27T09:00:00Z",
  "end_time": "2026-01-27T10:00:00Z",
  "data_type": "tasks"  // tasks, claimers, events
}
```

#### POST /api/report/generate

生成报告。

**请求体：**
```json
{
  "start_time": "2026-01-27T09:00:00Z",
  "end_time": "2026-01-27T10:00:00Z",
  "report_type": "detailed"  // summary, detailed, performance
}
```

---

## WebSocket API

### 连接端点

```
ws://host:port/ws/stream
```

### 消息格式

**事件推送：**
```json
{
  "type": "event",
  "data": {
    "event_id": "evt-001",
    "type": "TaskCompleted",
    "timestamp": "2026-01-27T10:35:22Z",
    "source_id": "task-089",
    "description": "Task completed"
  }
}
```

**指标更新：**
```json
{
  "type": "metrics_update",
  "data": {
    "total_tasks": 171,
    "completed_tasks": 86,
    "task_completion_rate": 0.945
  }
}
```

**进度更新：**
```json
{
  "type": "progress_update",
  "data": {
    "task_id": "task-001",
    "progress": 80
  }
}
```

---

## 数据结构定义

### PlatformStatistics / DashboardMetrics

```cpp
struct PlatformStatistics {
    // 任务统计
    size_t total_tasks;
    size_t published_tasks;
    size_t claimed_tasks;
    size_t processing_tasks;
    size_t completed_tasks;
    size_t failed_tasks;
    size_t abandoned_tasks;
    
    // 申领者统计
    size_t total_claimers;
    size_t idle_claimers;
    size_t busy_claimers;
    size_t offline_claimers;
    
    // 性能指标
    double avg_task_duration_seconds;
    double tasks_per_hour;
    double task_completion_rate;      // 0.0-1.0
    double claimer_success_rate;      // 0.0-1.0
    
    // 时间信息
    Timestamp platform_start_time;
    std::chrono::seconds uptime;
    Timestamp last_update;
};
```

### TaskSummary

```cpp
struct TaskSummary {
    TaskId id;
    std::string title;
    std::string category;
    int priority;
    TaskStatus status;
    int progress;
    std::string publisher_id;
    tl::optional<std::string> claimer_id;
    Timestamp created_at;
    tl::optional<Timestamp> completed_at;
};
```

### ClaimerSummary

```cpp
struct ClaimerSummary {
    std::string id;
    std::string name;
    std::string role;
    ClaimerState status;
    int active_task_count;
    int max_concurrent_tasks;
    int total_completed;
    int total_failed;
    double success_rate;
    int reputation_points;
    Timestamp last_active_at;
};
```

### EventRecord

```cpp
struct EventRecord {
    EventType type;
    Timestamp timestamp;
    std::string event_id;
    std::string source_id;
    std::map<std::string, std::string> metadata;
    std::string description;
};
```

### PerformanceAnalysis

```cpp
struct PerformanceAnalysis {
    std::map<std::string, double> task_duration_by_category;
    std::map<std::string, double> claimer_efficiency;
    double tasks_per_minute;
    double peak_concurrent_tasks;
    double avg_queue_wait_time_seconds;
    double claimer_utilization_rate;
    double system_load_average;
};
```

---

## 错误响应

所有 API 在出错时返回统一格式：

```json
{
  "error": {
    "code": 1001,
    "message": "Task not found",
    "details": "Task with id 'task-999' does not exist"
  }
}
```

**HTTP 状态码：**
| 状态码 | 说明 |
|--------|------|
| 200 | 成功 |
| 400 | 请求参数错误 |
| 404 | 资源不存在 |
| 500 | 服务器内部错误 |

---

## 版本信息

- **API 版本**: 1.0.0
- **最后更新**: 2026-01-27
