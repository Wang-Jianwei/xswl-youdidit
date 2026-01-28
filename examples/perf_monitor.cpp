#include <xswl/youdidit/youdidit.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>
#include <string>
#include <iomanip>
#include <climits>
#include <limits>
#include <mutex>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <unordered_map>

using namespace xswl::youdidit;

struct Metrics {
    std::atomic<uint64_t> total_executions{0};
    std::atomic<uint64_t> total_latency_ns{0};
    std::atomic<uint64_t> min_latency_ns{ULLONG_MAX};
    std::atomic<uint64_t> max_latency_ns{0};
    std::atomic<uint64_t> failures{0};
};

struct Sample {
    uint64_t t_ms{0}; // ms since start
    uint64_t t_sec{0};
    uint64_t total{0};
    uint64_t delta{0};
    double avg_ms{0.0};
    double min_ms{0.0};
    double max_ms{0.0};
};

struct ClaimerStats {
    uint64_t completed{0};
    uint64_t total_latency_ns{0};
    uint64_t min_latency_ns{ULLONG_MAX};
    uint64_t max_latency_ns{0};
    uint64_t failures{0};
};

struct TaskDetail {
    std::string task_id;
    std::string claimer_id;
    double latency_ms{0.0};
    uint64_t latency_ns{0};
    bool failed{false};
};

struct PerClaimerSample {
    uint64_t t_ms{0};
    uint64_t t_sec{0};
    uint64_t completed{0};
    uint64_t delta{0};
    double avg_ms{0.0};
};

// per-claimer time series (t_sec -> samples)
std::unordered_map<std::string, std::vector<PerClaimerSample>> claimer_series;
std::mutex claimer_series_mutex;
// last seen completed count per claimer to compute delta
std::unordered_map<std::string, uint64_t> last_claimer_completed;

int main(int argc, char** argv) {
    size_t num_tasks = 1000;
    size_t num_claimers = 4;
    int mean_ms = 10;
    int print_interval_sec = 1;
    int sample_interval_ms = 1000;
    if (argc > 8) sample_interval_ms = std::stoi(argv[8]);

    // latency sampling (limited)
    std::vector<uint64_t> latency_samples;
    std::mutex latency_mutex;
    size_t max_latency_samples = 100000;
    size_t sample_every = 1;

    // high frequency event timestamps (ms since start)
    std::vector<uint64_t> global_event_times_ms;
    std::mutex global_event_mutex;
    size_t max_event_samples = 1000000; // cap
    size_t global_event_index = 0; // cursor used by monitor

    std::unordered_map<std::string, std::vector<uint64_t>> claimer_event_times_ms;
    std::unordered_map<std::string, size_t> claimer_event_index;
    std::mutex claimer_event_mutex;

    if (argc > 1) num_tasks = std::stoul(argv[1]);
    if (argc > 2) num_claimers = std::stoul(argv[2]);
    if (argc > 3) mean_ms = std::stoi(argv[3]);

    // Optional HTML output path (argv[4])
    std::string html_path;
    if (argc > 4) html_path = argv[4];

    // optional max latency samples (argv[5])
    if (argc > 5) max_latency_samples = std::stoul(argv[5]);

    // optional max event samples (argv[7])
    if (argc > 7) max_event_samples = std::stoul(argv[7]);

    auto start_time = std::chrono::steady_clock::now();
    std::vector<Sample> samples;
    std::mutex samples_mutex;

    std::unordered_map<std::string, ClaimerStats> claimer_stats;
    std::mutex claimer_mutex;

    std::vector<TaskDetail> task_details;
    std::mutex task_details_mutex;
    size_t max_task_details = 100000; // cap to avoid unbounded growth

    // parse max task details from argv[6]
    if (argc > 6) max_task_details = std::stoul(argv[6]);

    // compute sample_every to limit memory (will be 1 if num_tasks <= max_latency_samples)
    sample_every = std::max<size_t>(1, num_tasks / max_latency_samples);

    // initial sample at t=0
    {
        Sample s;
        s.t_sec = 0;
        s.total = 0;
        s.delta = 0;
        s.avg_ms = 0.0;
        s.min_ms = 0.0;
        s.max_ms = 0.0;
        std::lock_guard<std::mutex> lock(samples_mutex);
        samples.push_back(s);
    }

    std::cout << "Performance monitor example\n";
    std::cout << "  tasks=" << num_tasks << " claimers=" << num_claimers
              << " mean_ms=" << mean_ms;
    if (!html_path.empty()) std::cout << " html=" << html_path;
    std::cout << " max_latency_samples=" << max_latency_samples << " max_task_details=" << max_task_details << "\n";

    TaskPlatform platform;
    auto metrics = std::make_shared<Metrics>();

    // Create claimers
    std::vector<std::shared_ptr<Claimer>> claimers;
    for (size_t i = 0; i < num_claimers; ++i) {
        auto c = std::make_shared<Claimer>("claimer-" + std::to_string(i+1), "claimer-" + std::to_string(i+1));
        c->add_category("default");
        c->set_max_concurrent(1);
        platform.register_claimer(c);
        claimers.push_back(c);
    }

    // Handler: parse input as milliseconds and sleep
    auto handler_factory = [metrics, &latency_samples, &latency_mutex, &max_latency_samples, &sample_every,
                         &claimer_mutex, &claimer_stats, &task_details_mutex, &task_details, &max_task_details,
                         &start_time, &global_event_times_ms, &global_event_mutex, &max_event_samples,
                         &claimer_event_times_ms, &claimer_event_mutex]
                        (Task &task, const std::string &input) -> tl::expected<TaskResult, std::string> {
        int ms = 0;
        try {
            ms = std::stoi(input);
        } catch (...) {
            ms = 0;
        }
        auto start = std::chrono::steady_clock::now();
        if (ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        auto end = std::chrono::steady_clock::now();
        uint64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        metrics->total_executions.fetch_add(1, std::memory_order_relaxed);
        metrics->total_latency_ns.fetch_add(ns, std::memory_order_relaxed);

        // update min
        uint64_t old_min = metrics->min_latency_ns.load(std::memory_order_relaxed);
        while (ns < old_min && !metrics->min_latency_ns.compare_exchange_weak(old_min, ns, std::memory_order_relaxed)) {}
        // update max
        uint64_t old_max = metrics->max_latency_ns.load(std::memory_order_relaxed);
        while (ns > old_max && !metrics->max_latency_ns.compare_exchange_weak(old_max, ns, std::memory_order_relaxed)) {}

        // sampled latency collection (cap and downsampling)
        uint64_t exec_count = metrics->total_executions.load(std::memory_order_relaxed);
        if (sample_every == 1 || (exec_count % sample_every) == 0) {
            std::lock_guard<std::mutex> lock(latency_mutex);
            if (latency_samples.size() < max_latency_samples) latency_samples.push_back(ns);
        }

        // per-claimer stats and per-task detail
        try {
            std::string cid = task.claimer_id();
            // update claimer map
            {
                std::lock_guard<std::mutex> lock(claimer_mutex);
                auto &cs = claimer_stats[cid];
                cs.completed += 1;
                cs.total_latency_ns += ns;
                if (ns < cs.min_latency_ns) cs.min_latency_ns = ns;
                if (ns > cs.max_latency_ns) cs.max_latency_ns = ns;
            }

            // push a task detail record (bounded)
            {
                std::lock_guard<std::mutex> lock(task_details_mutex);
                if (task_details.size() < max_task_details) {
                    TaskDetail td;
                    td.task_id = task.id();
                    td.claimer_id = cid;
                    td.latency_ns = ns;
                    td.latency_ms = ns / 1e6;
                    task_details.push_back(std::move(td));
                }
            }

            // append event timestamps (ms since start) for global and claimer
            auto now = std::chrono::steady_clock::now();
            uint64_t ms_since_start = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
            {
                std::lock_guard<std::mutex> lock(global_event_mutex);
                if (global_event_times_ms.size() < max_event_samples) global_event_times_ms.push_back(ms_since_start);
            }
            {
                std::lock_guard<std::mutex> lock(claimer_event_mutex);
                auto &vec = claimer_event_times_ms[cid];
                if (vec.size() < max_event_samples) vec.push_back(ms_since_start);
            }
        } catch (...) {
            // ignore any task metadata errors
        }

        TaskResult r(true, "ok");
        return r;
    };

    // Publish tasks (each task re-uses the same handler)
    for (size_t i = 0; i < num_tasks; ++i) {
        auto builder = platform.task_builder();
        builder.title("perf-task")
               .category("default")
               .priority(50)
               .handler(handler_factory);
        auto task = builder.build();
        platform.publish_task(task);
    }

    std::atomic<bool> done{false};

    // Claimer threads
    std::vector<std::thread> threads;
    for (size_t i = 0; i < claimers.size(); ++i) {
        threads.emplace_back([&, i]() {
            std::mt19937 rng(static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count() + i));
            std::uniform_int_distribution<int> dist(1, std::max(1, mean_ms * 2));
            auto &c = claimers[i];
            while (!done.load(std::memory_order_acquire)) {
                auto tasks = c->claim_tasks_to_capacity();
                if (tasks.empty()) {
                    // nothing to claim right now
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                    continue;
                }
                for (auto &t : tasks) {
                    int ms = dist(rng);
                    auto res = c->run_task(t, std::to_string(ms));
                    if (!res.has_value()) {
                        metrics->failures.fetch_add(1, std::memory_order_relaxed);
                        // record per-claimer failure
                        {
                            std::lock_guard<std::mutex> lock(claimer_mutex);
                            auto &cs = claimer_stats[c->id()];
                            cs.failures += 1;
                        }
                        // record per-task failure detail (no latency recorded)
                        {
                            std::lock_guard<std::mutex> lock(task_details_mutex);
                            if (task_details.size() < max_task_details) {
                                TaskDetail td;
                                td.task_id = t->id();
                                td.claimer_id = c->id();
                                td.latency_ms = 0.0;
                                td.latency_ns = 0;
                                td.failed = true;
                                task_details.push_back(std::move(td));
                            }
                        }
                    }
                }
            }
        });
    }

    // Monitor thread
    std::thread monitor([&]() {
        uint64_t last_total = 0;
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sample_interval_ms));
            // compute delta using high-frequency event timestamps (ms)
            auto now = std::chrono::steady_clock::now();
            uint64_t current_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();

            uint64_t delta = 0;
            {
                std::lock_guard<std::mutex> lock(global_event_mutex);
                while (global_event_index < global_event_times_ms.size() && global_event_times_ms[global_event_index] <= current_ms) {
                    ++delta;
                    ++global_event_index;
                }
            }

            uint64_t total = metrics->total_executions.load(std::memory_order_relaxed);

            uint64_t tot_lat_ns = metrics->total_latency_ns.load(std::memory_order_relaxed);
            uint64_t executed = total;
            double avg_ms = executed ? (double)tot_lat_ns / executed / 1e6 : 0.0;
            uint64_t min_ns = metrics->min_latency_ns.load(std::memory_order_relaxed);
            uint64_t max_ns = metrics->max_latency_ns.load(std::memory_order_relaxed);

            auto stats = platform.get_statistics();

            std::cout << "[stats] completed_delta=" << delta
                      << " total_completed=" << total
                      << " rate=" << delta / (double)print_interval_sec << "/s"
                      << " avg_ms=" << std::fixed << std::setprecision(3) << avg_ms
                      << " min_ms=" << (min_ns==ULLONG_MAX?0.0:(min_ns/1e6))
                      << " max_ms=" << (max_ns/1e6)
                      << " platform_completed=" << stats.completed_tasks
                      << " published=" << stats.published_tasks
                      << " claimed=" << stats.claimed_tasks
                      << " processing=" << stats.processing_tasks
                      << " failures=" << metrics->failures.load() << "\n";

            // push a sample for later HTML report
            {
                auto now = std::chrono::steady_clock::now();
                uint64_t t_sec = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
                Sample s;
                s.t_ms = current_ms;
                s.t_sec = t_sec;
                s.total = total;
                s.delta = delta;
                s.avg_ms = avg_ms;
                s.min_ms = (min_ns==ULLONG_MAX?0.0:(min_ns/1e6));
                s.max_ms = (max_ns/1e6);
                std::lock_guard<std::mutex> lock(samples_mutex);
                samples.push_back(s);
            }

            // record per-claimer samples
            {
                std::lock_guard<std::mutex> lock1(claimer_mutex);
                std::lock_guard<std::mutex> lock2(claimer_series_mutex);
                for (const auto &pair : claimer_stats) {
                    const std::string &cid = pair.first;
                    const ClaimerStats &cs = pair.second;
                    // compute delta for this claimer using its event times
                    uint64_t d = 0;
                    {
                        std::lock_guard<std::mutex> lock_ev(claimer_event_mutex);
                        auto &vec = claimer_event_times_ms[cid];
                        auto &idx = claimer_event_index[cid];
                        while (idx < vec.size() && vec[idx] <= current_ms) {
                            ++d;
                            ++idx;
                        }
                    }
                    PerClaimerSample ps;
                    uint64_t ps_t_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
                    ps.t_ms = ps_t_ms;
                    ps.t_sec = ps_t_ms/1000;
                    ps.completed = cs.completed;
                    ps.delta = d;
                    ps.avg_ms = (cs.completed ? (double)cs.total_latency_ns / cs.completed / 1e6 : 0.0);
                    claimer_series[cid].push_back(ps);
                }
            }

            if (total >= num_tasks) {
                break;
            }
        }
    });

    // wait for work to finish
    while (metrics->total_executions.load(std::memory_order_relaxed) < num_tasks) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // signal threads to stop and join
    done.store(true, std::memory_order_release);
    for (auto &t : threads) if (t.joinable()) t.join();
    if (monitor.joinable()) monitor.join();

    // push a final sample to capture end state
    {
        auto now = std::chrono::steady_clock::now();
        uint64_t t_sec = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        uint64_t total = metrics->total_executions.load();
        uint64_t last_total = 0;
        Sample s;
        s.t_sec = t_sec;
        s.total = total;
        {
            std::lock_guard<std::mutex> lock(samples_mutex);
            if (!samples.empty()) last_total = samples.back().total;
            s.delta = (total > last_total) ? (total - last_total) : 0;
        }
        s.avg_ms = (double)metrics->total_latency_ns.load() / (total ? total : 1) / 1e6;
        s.min_ms = (metrics->min_latency_ns.load()==ULLONG_MAX ? 0.0 : (metrics->min_latency_ns.load()/1e6));
        s.max_ms = metrics->max_latency_ns.load() / 1e6;
        std::lock_guard<std::mutex> lock(samples_mutex);
        samples.push_back(s);
    }

    // Final report
    uint64_t executed = metrics->total_executions.load();
    uint64_t tot_lat_ns = metrics->total_latency_ns.load();
    double avg_ms = executed ? (double)tot_lat_ns / executed / 1e6 : 0.0;
    uint64_t min_ns = metrics->min_latency_ns.load();
    uint64_t max_ns = metrics->max_latency_ns.load();

    std::cout << "\nFinal: executed=" << executed
              << " avg_ms=" << std::fixed << std::setprecision(3) << avg_ms
              << " min_ms=" << (min_ns==ULLONG_MAX?0.0:(min_ns/1e6))
              << " max_ms=" << (max_ns/1e6)
              << " failures=" << metrics->failures.load() << "\n";

    auto stats = platform.get_statistics();
    std::cout << "Platform stats: total=" << stats.total_tasks
              << " published=" << stats.published_tasks
              << " claimed=" << stats.claimed_tasks
              << " processing=" << stats.processing_tasks
              << " completed=" << stats.completed_tasks
              << " failed=" << stats.failed_tasks
              << " abandoned=" << stats.abandoned_tasks
              << " claimers=" << stats.total_claimers << "\n";

    if (!html_path.empty()) {
        std::ofstream ofs(html_path);
        if (ofs) {
            // compute max delta for scaling and percentiles
            uint64_t max_delta = 1;
            std::vector<uint64_t> lat_copy;
            {
                std::lock_guard<std::mutex> lock(samples_mutex);
                for (const auto &s : samples) max_delta = std::max<uint64_t>(max_delta, s.delta);
            }
            {
                std::lock_guard<std::mutex> lock(latency_mutex);
                lat_copy = latency_samples; // copy for percentile calc
            }

            double p50 = 0.0, p90 = 0.0, p99 = 0.0;
            if (!lat_copy.empty()) {
                std::sort(lat_copy.begin(), lat_copy.end());
                auto at = [&](double q)->double{
                    size_t idx = std::min<size_t>(lat_copy.size()-1, (size_t)std::floor(q * lat_copy.size()));
                    return lat_copy[idx] / 1e6;
                };
                p50 = at(0.50);
                p90 = at(0.90);
                p99 = at(0.99);
            }

            int width = 800, height = 160, margin = 30;
            std::ostringstream svg_points;
            std::vector<std::pair<double,double>> svg_coords;
            {
                std::lock_guard<std::mutex> lock(samples_mutex);
                if (!samples.empty()) {
                    size_t n = samples.size();
                    for (size_t i = 0; i < n; ++i) {
                        double x = margin + (double)i * (width - 2*margin) / (std::max<size_t>(1, n - 1));
                        double y = margin + (height - 2*margin) * (1.0 - (double)samples[i].delta / (double)max_delta);
                        svg_coords.emplace_back(x,y);
                        svg_points << x << "," << y << " ";
                    }
                }
            }

            ofs << "<!doctype html>\n<html><head><meta charset=\"utf-8\"><title>性能监控报告</title>\n"
                << "<style>body{font-family:Arial,Helvetica,sans-serif;margin:12px;font-size:13px;line-height:1.25;color:#222} table{border-collapse:collapse;font-size:0.9em} td,th{border:1px solid #ddd;padding:4px} th{background:#f8f8f8} h1{font-size:1.25em;margin-bottom:6px} h2{font-size:1.05em;margin-top:10px;margin-bottom:6px} h3{font-size:0.95em;margin:4px 0} .muted{color:#666;font-size:0.85em} .claimer-card{display:inline-block;margin:6px;border:1px solid #eee;padding:6px;border-radius:4px;vertical-align:top} canvas{max-width:100%;height:auto}</style>\n"
                << "</head><body>\n";
            ofs << "<h1>性能监控报告</h1>\n";
            ofs << "<p class='muted'>本报告汇总任务吞吐、延迟与各 Claimer 的时间序列，便于分析性能表现与定位瓶颈。</p>\n";
            ofs << "<h2>总览</h2>\n";
            ofs << "<p class='muted'>显示总体完成任务数、失败数、平均/最小/最大延迟与百分位（P50/P90/P99），用于快速评估系统整体性能。</p>\n";
            ofs << "<table>\n";
            ofs << "<tr><th>指标</th><th>数值</th></tr>\n";
            ofs << "<tr><td>总任务数（发布）</td><td>" << stats.total_tasks << "</td></tr>\n";
            ofs << "<tr><td>已完成</td><td>" << stats.completed_tasks << "</td></tr>\n";
            ofs << "<tr><td>失败数</td><td>" << metrics->failures.load() << "</td></tr>\n";
            ofs << "<tr><td>平均延迟 (ms)</td><td>" << std::fixed << std::setprecision(3) << avg_ms << "</td></tr>\n";
            ofs << "<tr><td>最小延迟 (ms)</td><td>" << (min_ns==ULLONG_MAX?0.0:(min_ns/1e6)) << "</td></tr>\n";
            ofs << "<tr><td>最大延迟 (ms)</td><td>" << (max_ns/1e6) << "</td></tr>\n";
            ofs << "<tr><td>P50 (ms)</td><td>" << std::fixed << std::setprecision(3) << p50 << "</td></tr>\n";
            ofs << "<tr><td>P90 (ms)</td><td>" << std::fixed << std::setprecision(3) << p90 << "</td></tr>\n";
            ofs << "<tr><td>P99 (ms)</td><td>" << std::fixed << std::setprecision(3) << p99 << "</td></tr>\n";
            ofs << "<tr><td>Claimer 数</td><td>" << stats.total_claimers << "</td></tr>\n";
            ofs << "</table>\n";

            // 测试输入参数及平均延迟评估
            double _expected_mean = (double)mean_ms;
            double _diff = avg_ms - _expected_mean;
            double _pct = (_expected_mean!=0.0) ? (_diff / _expected_mean * 100.0) : 0.0;
            std::string _eval_msg;
            if (_pct > 20.0) _eval_msg = "平均延迟明显高于期望（>20%），可能由排队、调度或系统开销引起，建议增加 Claimer 或检查任务执行路径。";
            else if (_pct < -20.0) _eval_msg = "平均延迟显著低于期望（<-20%），可能是采样偏差或任务短于期望，请核实任务延迟分布。";
            else _eval_msg = "平均延迟与期望值大致一致（±20%）。";

            ofs << "<h2>测试输入参数</h2>\n";
            ofs << "<table>\n";
            ofs << "<tr><th>参数</th><th>值</th></tr>\n";
            ofs << "<tr><td>总任务数 (tasks)</td><td>" << num_tasks << "</td></tr>\n";
            ofs << "<tr><td>Claimers</td><td>" << num_claimers << "</td></tr>\n";
            ofs << "<tr><td>期望单任务时长 mean_ms (ms)</td><td>" << mean_ms << "</td></tr>\n";
            ofs << "<tr><td>采样间隔 sample_interval_ms (ms)</td><td>" << sample_interval_ms << "</td></tr>\n";
            ofs << "<tr><td>最大延迟样本数 max_latency_samples</td><td>" << max_latency_samples << "</td></tr>\n";
            ofs << "<tr><td>最大任务详情数 max_task_details</td><td>" << max_task_details << "</td></tr>\n";
            ofs << "<tr><td>最大事件样本数 max_event_samples</td><td>" << max_event_samples << "</td></tr>\n";
            ofs << "</table>\n";

            ofs << "<h3>关于\"平均延迟\"的说明与评估</h3>\n";
            ofs << "<p class='muted'>\"平均延迟\"是对所有已完成任务的平均处理时间（单位：ms），计算方法为：总延迟 / 已完成任务数。该指标不仅反映任务本身执行耗时，也包含调度与排队开销，受输入参数（如 <code>mean_ms</code>、Claimers 数目与采样间隔）影响。</p>\n";
            ofs << "<p>期望单任务时长: <strong>" << mean_ms << " ms</strong>。实际测得平均延迟: <strong>" << std::fixed << std::setprecision(3) << avg_ms << " ms</strong>，差异: <strong>" << ( _diff >= 0 ? "+" : "") << std::fixed << std::setprecision(3) << _diff << " ms</strong>（约 " << std::fixed << std::setprecision(1) << _pct << "%）。</p>\n";
            ofs << "<p class='muted'>" << _eval_msg << "</p>\n";

            ofs << "<h2>吞吐量随时间变化</h2>\n";
            ofs << "<p class='muted'>每个采样间隔内完成的任务数，用于观察吞吐波动与突发。</p>\n";
            ofs << "<canvas id=\"chart-throughput\" width=\"" << width << "\" height=\"" << height << "\"></canvas>\n";
            ofs << "<div style='margin-top:8px;'><button onclick=\"resetZoomAll()\">重置缩放</button></div>\n";

            // per-claimer canvases
            ofs << "<h2>按 Claimer 的时间序列</h2>\n";
            ofs << "<p class='muted'>每个 Claimer 在各采样点的完成数及平均延迟（双 Y 轴），便于比较不同 Claimer 的表现。</p>\n";
            {
                std::lock_guard<std::mutex> lock1(claimer_mutex);
                std::lock_guard<std::mutex> lock2(claimer_series_mutex);
                for (const auto &pair : claimer_series) {
                    const auto &cid = pair.first;
                    std::string safe_id = cid;
                    // replace non-alnum with '-'
                    for (auto &ch : safe_id) if (!((ch>='A'&&ch<='Z')||(ch>='a'&&ch<='z')||(ch>='0'&&ch<='9'))) ch = '-';
                    ofs << "<div class='claimer-card'>";
                    ofs << "<h3 style='margin:4px 0'>" << cid << "</h3>\n";
                    ofs << "<canvas id=\"chart-claimer-" << safe_id << "\" width=\"360\" height=\"96\"></canvas>\n";
                    const auto &cs = claimer_stats[cid];
                    double avg = cs.completed ? (double)cs.total_latency_ns / cs.completed / 1e6 : 0.0;
                    ofs << "<div class='muted'>已完成=" << cs.completed << " 平均延迟(ms)=" << std::fixed << std::setprecision(3) << avg << "</div>\n";
                    ofs << "</div>\n";
                }
            }

            // Chart.js and data
            ofs << "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>\n";
            ofs << "<script src=\"https://cdn.jsdelivr.net/npm/chartjs-plugin-zoom@2.2.0/dist/chartjs-plugin-zoom.min.js\"></script>\n";
            ofs << "<script>\n";
            // register zoom plugin if available (compat with different global names)
            ofs << "if (typeof Chart !== 'undefined') {\n"
                << "  if (typeof window.ChartZoom !== 'undefined') Chart.register(window.ChartZoom);\n"
                << "  else if (typeof window.chartjsPluginZoom !== 'undefined') Chart.register(window.chartjsPluginZoom);\n"
                << "  else if (typeof window.zoomPlugin !== 'undefined') Chart.register(window.zoomPlugin);\n"
                << "}\n";

            // global labels and data
            auto _fmt_ms = [](uint64_t ms)->std::string{ uint64_t total = ms; uint64_t m = total / 60000; uint64_t s = (total % 60000) / 1000; uint64_t ms_part = total % 1000; char buf[32]; snprintf(buf, sizeof(buf), "%02llu:%02llu.%03llu", (unsigned long long)m, (unsigned long long)s, (unsigned long long)ms_part); return std::string(buf); };
            ofs << "const labels = [";
            {
                std::lock_guard<std::mutex> lock(samples_mutex);
                bool first = true;
                for (const auto &s : samples) {
                    if (!first) ofs << ",";
                    ofs << "\"" << _fmt_ms(s.t_ms) << "\"";
                    first = false;
                }
            }
            ofs << "];\n";

            ofs << "const globalDelta = [";
            {
                std::lock_guard<std::mutex> lock(samples_mutex);
                bool first = true;
                for (const auto &s : samples) {
                    if (!first) ofs << ",";
                    ofs << s.delta;
                    first = false;
                }
            }
            ofs << "];\n";

            ofs << "const globalAvg = [";
            {
                std::lock_guard<std::mutex> lock(samples_mutex);
                bool first = true;
                for (const auto &s : samples) {
                    if (!first) ofs << ",";
                    ofs << s.avg_ms;
                    first = false;
                }
            }
            ofs << "];\n";

            ofs << "const charts = {};\n";
            ofs << "const ctx = document.getElementById('chart-throughput').getContext('2d');\n";
            ofs << "charts['throughput'] = new Chart(ctx, { type: 'line', data: { labels: labels, datasets: [{ label: '每间隔完成数', data: globalDelta, borderColor: 'steelblue', backgroundColor: 'rgba(70,130,180,0.2)', yAxisID: 'y' }, { label: '平均延迟 (ms)', data: globalAvg, borderColor: 'darkorange', backgroundColor: 'rgba(255,165,0,0.2)', yAxisID: 'y1' }] }, options: { responsive: true, scales: { x: { grid: { display: true } }, y: { type: 'linear', position: 'left' }, y1: { type: 'linear', position: 'right', grid: { drawOnChartArea: false } } }, plugins: { zoom: { pan: { enabled: true, mode: 'x' }, zoom: { wheel: { enabled: true }, pinch: { enabled: true }, mode: 'x' } }, tooltip: { callbacks: { title: function(items){ return items && items.length ? items[0].label : ''; } } } } } });\n";

            // per-claimer datasets
            {
                std::lock_guard<std::mutex> lock(claimer_series_mutex);
                for (const auto &pair : claimer_series) {
                    const auto &cid = pair.first;
                    std::string safe_id = cid;
                    for (auto &ch : safe_id) if (!((ch>='A'&&ch<='Z')||(ch>='a'&&ch<='z')||(ch>='0'&&ch<='9'))) ch = '-';

                    ofs << "(function(){\n";
                    ofs << "const labels = [";
                    bool first = true;
                    for (const auto &ps : pair.second) {
                        if (!first) ofs << ",";
                        ofs << "\"" << _fmt_ms(ps.t_ms) << "\"";
                        first = false;
                    }
                    ofs << "];\n";
                    ofs << "const deltaData = [";
                    first = true;
                    for (const auto &ps : pair.second) {
                        if (!first) ofs << ",";
                        ofs << ps.delta;
                        first = false;
                    }
                    ofs << "];\n";
                    ofs << "const avgData = [";
                    first = true;
                    for (const auto &ps : pair.second) {
                        if (!first) ofs << ",";
                        ofs << ps.avg_ms;
                        first = false;
                    }
                    ofs << "];\n";
                    ofs << "const ctx = document.getElementById('chart-claimer-" << safe_id << "').getContext('2d');\n";
                    ofs << "charts['claimer-" << safe_id << "'] = new Chart(ctx, { type: 'line', data: { labels: labels, datasets: [{ label: '每间隔完成数', data: deltaData, borderColor: 'steelblue', backgroundColor: 'rgba(70,130,180,0.2)', yAxisID: 'y' }, { label: '平均延迟 (ms)', data: avgData, borderColor: 'darkorange', backgroundColor: 'rgba(255,165,0,0.2)', yAxisID: 'y1' }] }, options: { responsive: true, scales: { x: { grid: { display: true } }, y: { type: 'linear', position: 'left' }, y1: { type: 'linear', position: 'right', grid: { drawOnChartArea: false } } }, plugins: { zoom: { pan: { enabled: true, mode: 'x' }, zoom: { wheel: { enabled: true }, pinch: { enabled: true }, mode: 'x' } }, tooltip: { callbacks: { title: function(items){ return items && items.length ? items[0].label : ''; } } } }, interaction: { mode: 'index', intersect: false } } });\n"; 
                    ofs << "})();\n";
                }
            }

            ofs << "function resetZoomAll(){ for (var k in charts) if (charts[k] && charts[k].resetZoom) charts[k].resetZoom(); }\n";
            ofs << "</script>\n";

            ofs << "<h2>样本列表</h2>\n";
            ofs << "<p class='muted'>采样点的时间戳、完成数与平均延迟，供进一步分析与导出。</p>\n";
            ofs << "<table>\n<tr><th>时间(秒)</th><th>总数</th><th>区间增量</th><th>平均延迟(ms)</th><th>最小延迟(ms)</th><th>最大延迟(ms)</th></tr>\n";
            {
                std::lock_guard<std::mutex> lock(samples_mutex);
                for (const auto &s : samples) {
                    ofs << "<tr><td>" << s.t_sec << "</td><td>" << s.total << "</td><td>" << s.delta
                        << "</td><td>" << std::fixed << std::setprecision(3) << s.avg_ms
                        << "</td><td>" << s.min_ms << "</td><td>" << s.max_ms << "</td></tr>\n";
                }
            }
            ofs << "</table>\n";

            // per-claimer summary
            ofs << "<h2>按 Claimer 的汇总</h2>\n";
            ofs << "<p class='muted'>统计每个 Claimer 的总完成数、平均/最小/最大延迟与失败数，便于定位个体差异。</p>\n";
            ofs << "<table>\n<tr><th>Claimer（标识）</th><th>已完成</th><th>平均延迟(ms)</th><th>最小延迟(ms)</th><th>最大延迟(ms)</th></tr>\n";
            {
                std::lock_guard<std::mutex> lock(claimer_mutex);
                for (const auto &pair : claimer_stats) {
                    const auto &id = pair.first;
                    const auto &cs = pair.second;
                    double avg = cs.completed ? (double)cs.total_latency_ns / cs.completed / 1e6 : 0.0;
                    double minms = (cs.min_latency_ns==ULLONG_MAX?0.0:(cs.min_latency_ns/1e6));
                    double maxms = cs.max_latency_ns/1e6;
                    ofs << "<tr><td>" << id << "</td><td>" << cs.completed << "</td><td>" << std::fixed << std::setprecision(3) << avg
                        << "</td><td>" << minms << "</td><td>" << maxms << "</td></tr>\n";
                }
            }
            ofs << "</table>\n";

            // per-task details (top N slowest)
            size_t topN = 100;
            std::vector<TaskDetail> copy_details;
            {
                std::lock_guard<std::mutex> lock(task_details_mutex);
                copy_details = task_details; // copy
            }
            std::sort(copy_details.begin(), copy_details.end(), [](const TaskDetail &a, const TaskDetail &b){ return a.latency_ns > b.latency_ns; });
            ofs << "<h2>按任务详细信息（Top " << std::min<size_t>(topN, copy_details.size()) << "）</h2>\n";
            ofs << "<p class='muted'>列出延迟最长的任务（Top N），包含所属 Claimer 与延迟，帮助定位慢任务。</p>\n";
            ofs << "<table>\n<tr><th>任务 ID</th><th>Claimer（标识）</th><th>延迟 (ms)</th></tr>\n";
            for (size_t i = 0; i < std::min<size_t>(topN, copy_details.size()); ++i) {
                const auto &d = copy_details[i];
                ofs << "<tr><td>" << d.task_id << "</td><td>" << d.claimer_id << "</td><td>" << std::fixed << std::setprecision(3) << d.latency_ms << "</td></tr>\n";
            }
            ofs << "</table>\n";

            ofs << "</body></html>\n";
            ofs.close();
            std::cout << "Wrote HTML summary to " << html_path << "\n";
        } else {
            std::cerr << "Failed to open HTML output file: " << html_path << "\n";
        }
    }

    return 0;
}
