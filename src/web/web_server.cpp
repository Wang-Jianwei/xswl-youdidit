#include <xswl/youdidit/web/web_server.hpp>
#include <httplib.h>
#include <sstream>
#include <chrono>
#include <thread>

namespace xswl {
namespace youdidit {

WebServer::WebServer(WebDashboard *dashboard, int port)
    : dashboard_(dashboard), host_("0.0.0.0"), port_(port), running_(false), http_server_(nullptr) {}

WebServer::~WebServer() noexcept {
    stop();
}

void WebServer::start() {
    if (running_) return;
    
    try {
        // Cast void* to httplib::Server*
        httplib::Server *server = new httplib::Server();
        http_server_ = static_cast<void*>(server);
        
        // Setup routes BEFORE starting server
        _setup_routes();

        // Start listening in background thread
        // Official cpp-httplib: listen() is blocking, so run in thread
        running_ = true;
        server_thread_ = std::thread([this, server]() {
            server->listen(host_.c_str(), port_);
        });
        
        // Give the server a moment to bind
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } catch (...) {
        running_ = false;
        if (http_server_) {
            httplib::Server *server = static_cast<httplib::Server*>(http_server_);
            delete server;
            http_server_ = nullptr;
        }
    }
}

void WebServer::stop() {
    if (http_server_) {
        httplib::Server *server = static_cast<httplib::Server*>(http_server_);
        server->stop();
    }

    if (server_thread_.joinable()) {
        server_thread_.join();
    }

    if (http_server_) {
        httplib::Server *server = static_cast<httplib::Server*>(http_server_);
        delete server;
        http_server_ = nullptr;
    }

    running_ = false;
}

bool WebServer::is_running() const noexcept {
    return running_;
}

void WebServer::set_port(int port) {
    port_ = port;
}

int WebServer::get_port() const noexcept {
    return port_;
}

void WebServer::set_host(const std::string &host) {
    host_ = host;
}

const std::string &WebServer::host() const noexcept {
    return host_;
}

void WebServer::_setup_routes() {
    if (!http_server_ || !dashboard_) return;
    
    httplib::Server *server = static_cast<httplib::Server*>(http_server_);
    
    // 首页 - Web 仪表板
    server->Get("/", [this](const httplib::Request& req, httplib::Response& res) {
        res.set_content(_get_dashboard_html(), "text/html; charset=utf-8");
    });
    
    // REST API - 获取指标
    server->Get("/api/metrics", [this](const httplib::Request& req, httplib::Response& res) {
        auto metrics = dashboard_->get_metrics();
        
        std::ostringstream oss;
        oss << "{"
            << "\"total_tasks\":" << metrics.total_tasks << ","
            << "\"published_tasks\":" << metrics.published_tasks << ","
            << "\"claimed_tasks\":" << metrics.claimed_tasks << ","
            << "\"processing_tasks\":" << metrics.processing_tasks << ","
            << "\"completed_tasks\":" << metrics.completed_tasks << ","
            << "\"failed_tasks\":" << metrics.failed_tasks << ","
            << "\"abandoned_tasks\":" << metrics.abandoned_tasks << ","
            << "\"total_claimers\":" << metrics.total_claimers
            << "}";
        
        res.set_content(oss.str(), "application/json");
    });
    
    // REST API - 获取任务摘要
    server->Get("/api/tasks", [this](const httplib::Request& req, httplib::Response& res) {
        auto tasks = dashboard_->get_tasks_summary();
        
        std::ostringstream oss;
        oss << "{\"tasks\":[";
        for (size_t i = 0; i < tasks.size(); ++i) {
            if (i > 0) oss << ",";
            
            auto published_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                tasks[i].published_at.time_since_epoch()).count();
            
            oss << "{";
            oss << "\"id\":\"" << tasks[i].id << "\",";
            oss << "\"title\":\"" << tasks[i].title << "\",";
            oss << "\"category\":\"" << tasks[i].category << "\",";
            oss << "\"priority\":" << tasks[i].priority << ",";
            oss << "\"status\":\"" << to_string(tasks[i].status) << "\",";
            oss << "\"published_at\":" << published_ms << ",";
            oss << "\"claimer_id\":\"" << tasks[i].claimer_id << "\"";
            oss << "}";
        }
        oss << "],\"count\":" << tasks.size() << "}";
        
        res.set_content(oss.str(), "application/json");
    });
    
    // REST API - 获取申领者摘要
    server->Get("/api/claimers", [this](const httplib::Request& req, httplib::Response& res) {
        auto claimers = dashboard_->get_claimers_summary();
        
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < claimers.size(); ++i) {
            if (i > 0) oss << ",";
            oss << "{"
                << "\"id\":\"" << claimers[i].id << "\","
                << "\"name\":\"" << claimers[i].name << "\","
                << "\"status\":\"" << to_string(claimers[i].status) << "\","
                << "\"active_task_count\":" << claimers[i].active_task_count << ","
                << "\"total_completed\":" << claimers[i].total_completed << ","
                << "\"total_failed\":" << claimers[i].total_failed
                << "}";
        }
        oss << "]";
        
        res.set_content(oss.str(), "application/json");
    });
    
    // REST API - 获取事件日志
    // REST API - 获取事件日志
    server->Get("/api/logs", [this](const httplib::Request& req, httplib::Response& res) {
        auto logs = dashboard_->get_event_logs();
        
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < logs.size(); ++i) {
            if (i > 0) oss << ",";
            oss << "\"" << logs[i] << "\"";
        }
        oss << "]";
        
        res.set_content(oss.str(), "application/json");
    });
}

std::string WebServer::_get_dashboard_html() {
    return R"HTML(<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>xswl-youdidit 任务控制台</title>
    <style>
        :root {
            --bg-color: #0f172a;
            --panel-bg: #1e293b;
            --text-primary: #e2e8f0;
            --text-secondary: #94a3b8;
            --border-color: #334155;
            --font-mono: 'JetBrains Mono', 'Fira Code', Consolas, monospace;
        }
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            font-size: 13px; /* 更紧凑的字体 */
            background-color: var(--bg-color);
            color: var(--text-primary);
            height: 100vh;
            display: flex;
            flex-direction: column;
            overflow: hidden;
        }
        
        /* 滚动条 */
        ::-webkit-scrollbar { width: 6px; height: 6px; }
        ::-webkit-scrollbar-corner { background: transparent; }
        ::-webkit-scrollbar-thumb { background: #475569; border-radius: 3px; }
        ::-webkit-scrollbar-thumb:hover { background: #64748b; }
        
        /* 布局结构 */
        .wrapper {
            display: flex;
            flex-direction: column;
            height: 100%;
        }
        
        .top-bar {
            height: 48px;
            background: var(--panel-bg);
            border-bottom: 1px solid var(--border-color);
            display: flex;
            align-items: center;
            justify-content: space-between;
            padding: 0 16px;
            flex-shrink: 0;
            z-index: 20;
        }
        
        .brand {
            font-weight: 600;
            font-size: 13px;
            color: #fff;
            display: flex;
            align-items: center;
            gap: 8px;
            letter-spacing: 0.5px;
        }

        /* 顶部紧凑指标栏 */
        .metrics-strip {
            display: flex;
            gap: 16px;
            align-items: center;
            margin-right: 16px;
        }
        .mini-metric {
            display: flex;
            align-items: baseline;
            gap: 4px;
            font-size: 12px;
            color: var(--text-secondary);
        }
        .mini-metric strong {
            color: var(--text-primary);
            font-family: var(--font-mono);
            font-weight: 600;
        }

        /* 主体内容 */
        .main-content {
            flex: 1;
            display: grid;
            grid-template-columns: 1fr 280px; /* 任务列表为主，右侧放申领者 */
            overflow: hidden;
        }

        /* 面板通用样式 */
        .panel {
            display: flex;
            flex-direction: column;
            border-right: 1px solid var(--border-color);
            min-width: 0; /* 防止 grid 溢出 */
            overflow: hidden; /* 修复：限制高度以显示滚动条 */
        }
        .panel:last-child { border-right: none; background: #162032; }

        .panel-header {
            height: 36px;
            padding: 0 12px;
            border-bottom: 1px solid var(--border-color);
            display: flex;
            align-items: center;
            justify-content: space-between;
            background: rgba(15, 23, 42, 0.3);
            font-size: 11px;
            font-weight: 600;
            color: var(--text-secondary);
        }

        .scroll-area {
            flex: 1;
            overflow-y: auto;
        }

        /* 表格样式 */
        table {
            width: 100%;
            border-collapse: collapse;
            font-size: 11px;
        }
        thead th {
            text-align: center;
            padding: 8px 12px;
            font-weight: 500;
            color: var(--text-secondary);
            border-bottom: 1px solid var(--border-color);
            background: var(--bg-color);
            position: sticky;
            top: 0;
            z-index: 10;
        }
        tbody td {
            text-align: center;
            padding: 6px 12px;
            border-bottom: 1px solid rgba(51, 65, 85, 0.4);
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis;
            max-width: 200px;
        }
        tbody tr:hover { background: rgba(255, 255, 255, 0.03); }

        /* 申领者列表样式 */
        .claimer-item {
            padding: 8px 12px;
            border-bottom: 1px solid rgba(51, 65, 85, 0.4);
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .c-name {
            font-size: 11px;
            font-weight: 500;
            color: #e2e8f0;
        }
        .c-info {
            font-size: 11px;
            color: #64748b;
            margin-top: 2px;
        }

        /* 状态胶囊 */
        .pill {
            display: inline-flex;
            align-items: center;
            padding: 1px 6px;
            border-radius: 4px;
            font-size: 11px;
            font-weight: 500;
            line-height: 1.4;
        }
        .st-pub { color: #60a5fa; background: rgba(96, 165, 250, 0.1); }
        .st-clm { color: #facc15; background: rgba(250, 204, 21, 0.1); }
        .st-pro { color: #3b82f6; background: rgba(59, 130, 246, 0.1); }
        .st-ok  { color: #4ade80; background: rgba(74, 222, 128, 0.1); }
        .st-err { color: #f87171; background: rgba(248, 113, 113, 0.15); }
        
        .id-mono { font-family: var(--font-mono); color: #94a3b8; font-size: 11px; }

        .prio-dot { display: inline-block; width: 6px; height: 6px; border-radius: 50%; margin-right: 4px; }
        .p-high { background: #ef4444; }
        .p-med  { background: #f59e0b; }
        .p-low  { background: #10b981; }

    </style>
</head>
<body>
    <div class="wrapper">
        <div class="top-bar">
            <div class="brand">
                <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="4" style="color:#60a5fa"><path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"/><polyline points="22 4 12 14.01 9 11.01"/></svg>
                XSWL TASK CONTROL
            </div>
            
            <div style="display:flex; align-items:center">
                <div class="metrics-strip">
                    <div class="mini-metric">处理中 <strong style="color:#3b82f6" id="m-pro">0</strong></div>
                    <div class="mini-metric">积压 <strong style="color:#eab308" id="m-back">0</strong></div>
                    <div class="mini-metric">已完成 <strong style="color:#4ade80" id="m-ok">0</strong></div>
                    <div class="mini-metric">失败 <strong style="color:#f87171" id="m-fail">0</strong></div>
                    <div class="mini-metric">总计 <strong id="m-tot">0</strong></div>
                </div>
                <div style="font-size:11px; color:#64748b; font-family:var(--font-mono)" id="sys-time">00:00:00</div>
            </div>
        </div>

        <div class="main-content">
            <!-- 任务列表 -->
            <div class="panel">
                <div class="panel-header">
                    <span>实时任务流</span>
                    <span style="font-family:var(--font-mono); font-size:11px; background:rgba(255,255,255,0.05); padding:2px 6px; border-radius:3px" id="task-count">0</span>
                </div>
                <div class="scroll-area">
                    <table style="table-layout: fixed;">
                        <thead>
                            <tr>
                                <th width="80">时间</th>
                                <th width="40">Pr</th>
                                <th width="100">任务ID</th>
                                <th width="180">名称</th>
                                <th width="100">分类</th>
                                <th width="120">申领者 (Claimer)</th>
                                <th width="80">状态</th>
                            </tr>
                        </thead>
                        <tbody id="task-list">
                            <!-- JS fill -->
                        </tbody>
                    </table>
                </div>
            </div>

            <!-- 申领者列表 (右侧边栏) -->
            <div class="panel">
                <div class="panel-header">工作节点</div>
                <div class="scroll-area" id="claimer-list">
                    <!-- JS fill -->
                </div>
            </div>
        </div>
    </div>

<script>
const $ = id => document.getElementById(id);
const esc = s => (s||'').toString().replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));

// 辅助渲染
const renderSt = s => {
    const map = {'Published':'st-pub','Claimed':'st-clm','Processing':'st-pro','Completed':'st-ok','Failed':'st-err','Abandoned':'st-err'};
    return `<span class="pill ${map[s]||'st-pub'}">${s}</span>`;
};
const renderPr = p => {
    let cls = 'p-low'; if(p>=3) cls='p-high'; else if(p>=2) cls='p-med';
    return `<span class="prio-dot ${cls}"></span>`;
};
const fmtT = ts => new Date(ts).toLocaleTimeString('zh-CN',{hour12:false});

async function tick() {
    $('sys-time').innerText = new Date().toLocaleTimeString();

    // 1. Metrics & Claimers
    try {
        const [mRes, cRes] = await Promise.all([fetch('/api/metrics'), fetch('/api/claimers')]);
        const m = await mRes.json();
        const c = await cRes.json();
        
        // Metrics
        $('m-pro').innerText = m.processing_tasks;
        $('m-back').innerText = (m.published_tasks||0) + (m.claimed_tasks||0);
        $('m-ok').innerText = m.completed_tasks;
        $('m-fail').innerText = (m.failed_tasks||0) + (m.abandoned_tasks||0);
        $('m-tot').innerText = m.total_tasks;

        // Claimers
        $('claimer-list').innerHTML = c.map(x => {
            const active = x.active_task_count > 0;
            const stColor = active ? '#4ade80' : '#475569';
            return `
            <div class="claimer-item">
                <div>
                    <div class="c-name">${esc(x.name||x.id)}</div>
                    <div class="c-info">完成: ${x.total_completed} | 失败: ${x.total_failed}</div>
                </div>
                <div style="text-align:right">
                    <div style="font-size:10px; color:${stColor}; font-weight:600">${active ? 'BUSY' : 'IDLE'}</div>
                    <div style="font-size:10px; color:#64748b">Load: ${x.active_task_count}</div>
                </div>
            </div>`;
        }).join('');
    } catch(e) { console.error('Data fail', e); }

    // 2. Tasks
    try {
        const res = await fetch('/api/tasks');
        const data = await res.json();
        const tasks = (data.tasks||[]).sort((a,b)=> (b.published_at||0)-(a.published_at||0));
        $('task-count').innerText = tasks.length;
        
        $('task-list').innerHTML = tasks.slice(0, 100).map(t => {
            const claimer = t.claimer_id ? `<span style="font-size:11px; color:#94a3b8;">${esc(t.claimer_id)}</span>` 
                                         : '<span style="font-size:11px; color:#334155">-</span>';
            return `
            <tr>
                <td style="font-size:11px; color:#64748b;">${fmtT(t.published_at)}</td>
                <td style="text-align:center">${renderPr(t.priority)}</td>
                <td class="id-mono" title="${esc(t.id)}">${esc(t.id)}</td>
                <td style="font-size:11px; max-width:100px" title="${esc(t.title)}">${esc(t.title)}</td>
                <td><span style="font-size:11px; padding:1px 4px; background:#1e293b; border-radius:3px">${esc(t.category)}</span></td>
                <td>${claimer}</td>
                <td>${renderSt(t.status)}</td>
            </tr>`;
        }).join('');
    } catch(e) {}
}

tick();
setInterval(tick, 1500); // Faster updates
</script>
</body>
</html>)HTML";
}

} // namespace youdidit
} // namespace xswl
