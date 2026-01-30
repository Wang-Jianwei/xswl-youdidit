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

// In-file CLI/Config helpers (kept local to this example)
struct Config {
    size_t num_tasks = 1000;
    size_t num_claimers = 4;
    int sample_interval_ms = 1000;
    std::string duration_ranges_str;
    std::vector<std::pair<int,int>> duration_ranges;
    std::string html_path;
    size_t max_latency_samples = 100000;
    size_t max_task_details = 100000;
    size_t max_event_samples = 1000000;
    // optional comma-separated category list; if empty all tasks/claimers use "default"
    std::string categories_str;
    std::vector<std::string> categories;
};

static std::vector<std::pair<int,int>> parse_duration_ranges(const std::string &s) {
    std::vector<std::pair<int,int>> out;
    std::istringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, ',')) {
        if (tok.empty()) continue;
        // trim
        size_t a = 0; while (a < tok.size() && isspace((unsigned char)tok[a])) ++a;
        size_t b = tok.size(); while (b > a && isspace((unsigned char)tok[b-1])) --b;
        std::string part = tok.substr(a, b-a);
        size_t dash = part.find('-');
        try {
            if (dash == std::string::npos) {
                int v = std::stoi(part);
                out.emplace_back(v, v);
            } else {
                int lo = std::stoi(part.substr(0, dash));
                int hi = std::stoi(part.substr(dash+1));
                if (hi < lo) std::swap(lo, hi);
                out.emplace_back(lo, hi);
            }
        } catch (...) {
            // ignore malformed token
        }
    }
    return out;
}

static std::vector<std::string> parse_categories(const std::string &s) {
    std::vector<std::string> out;
    std::istringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, ',')) {
        if (tok.empty()) continue;
        // trim
        size_t a = 0; while (a < tok.size() && isspace((unsigned char)tok[a])) ++a;
        size_t b = tok.size(); while (b > a && isspace((unsigned char)tok[b-1])) --b;
        out.push_back(tok.substr(a, b-a));
    }
    return out;
}

static void print_usage(const char* prog) {
    std::cerr << "Error: duration ranges are required. Usage:\n";
    std::cerr << "  " << prog << " <tasks> <claimers> <duration_ranges> [html_path] [sample_interval_ms] [max_latency_samples] [max_task_details] [max_event_samples] [categories]\n";
    std::cerr << "Example: " << prog << " 200 4 \"0-1,1-1,2-8\" perf_report.html 20 100000 2000 500000 \"A,B,C,D\"\n";
}

static bool parse_args(int argc, char** argv, Config &cfg) {
    if (argc > 1) cfg.num_tasks = std::stoul(argv[1]);
    if (argc > 2) cfg.num_claimers = std::stoul(argv[2]);
    if (argc > 3) cfg.duration_ranges_str = argv[3];
    if (!cfg.duration_ranges_str.empty()) cfg.duration_ranges = parse_duration_ranges(cfg.duration_ranges_str);
    if (cfg.duration_ranges.empty()) {
        print_usage(argv[0]);
        return false;
    }
    if (argc > 4) cfg.html_path = argv[4];
    // NEW: sample_interval_ms is now the 5th parameter (after html_path)
    if (argc > 5) cfg.sample_interval_ms = std::stoi(argv[5]);
    if (argc > 6) cfg.max_latency_samples = std::stoul(argv[6]);
    if (argc > 7) cfg.max_task_details = std::stoul(argv[7]);
    if (argc > 8) cfg.max_event_samples = std::stoul(argv[8]);
    if (argc > 9) cfg.categories_str = argv[9];
    if (!cfg.categories_str.empty()) cfg.categories = parse_categories(cfg.categories_str);
    return true;
}

// Cross-platform sleep helper: on Windows spin and yield to reduce wake-up jitter; plain sleep elsewhere
static void sleep_for_ms(int ms) {
    if (ms <= 0) return;
#if defined(_WIN32)
    auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
    while (std::chrono::steady_clock::now() < end) {
        // yield to give up the CPU and avoid busy-loop burning a full core
        std::this_thread::yield();
    }
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
}

int main(int argc, char** argv) {
    // Parse CLI into a small config object (keeps main short and makes parsing logic reusable)
    Config cfg;
    if (!parse_args(argc, argv, cfg)) return 1;

    size_t num_tasks = cfg.num_tasks;
    size_t num_claimers = cfg.num_claimers;
    int print_interval_sec = 1;
    int sample_interval_ms = cfg.sample_interval_ms;
    std::string duration_ranges_str = cfg.duration_ranges_str;
    std::vector<std::pair<int,int>> duration_ranges = cfg.duration_ranges;
    std::vector<std::string> categories = cfg.categories;
    std::string html_path = cfg.html_path;

    // latency sampling (limited)
    std::vector<uint64_t> latency_samples;
    std::mutex latency_mutex;
    // overshoot sampling (ns = actual - requested)
    std::vector<int64_t> overshoot_samples;
    std::mutex overshoot_mutex;
    size_t max_latency_samples = cfg.max_latency_samples;
    size_t sample_every = 1;

    // high frequency event timestamps (ms since start)
    std::vector<uint64_t> global_event_times_ms;
    std::mutex global_event_mutex;
    // max_event_samples is initialized from Config below
    size_t global_event_index = 0; // cursor used by monitor

    std::unordered_map<std::string, std::vector<uint64_t>> claimer_event_times_ms;
    std::unordered_map<std::string, size_t> claimer_event_index;
    std::mutex claimer_event_mutex;

    // sampling overhead measurement
    std::atomic<uint64_t> sampling_overhead_ns{0}; // total ns spent in sampling operations
    std::atomic<uint64_t> sampling_ops{0}; // number of sampling measurements (approx. number of tasks measured)

    // per-interval min/max (ns) to compute min/max for each sampling interval (reset by monitor)
    std::atomic<uint64_t> interval_min_ns{ULLONG_MAX};
    std::atomic<uint64_t> interval_max_ns{0};

    // max event samples configured from parsed Config
    size_t max_event_samples = cfg.max_event_samples; // cap


    // html_path is provided by parsed Config; numeric caps are initialized below using Config

    auto start_time = std::chrono::steady_clock::now();
    std::vector<Sample> samples;
    std::mutex samples_mutex;

    std::unordered_map<std::string, ClaimerStats> claimer_stats;
    std::mutex claimer_mutex;

    std::vector<TaskDetail> task_details;
    std::mutex task_details_mutex;
    size_t max_task_details = cfg.max_task_details; // cap to avoid unbounded growth

    // compute sample_every to limit memory (will be 1 if num_tasks <= max_latency_samples)
    sample_every = std::max<size_t>(1, num_tasks / max_latency_samples);

    // initial sample at t=0 (use NaN for averages/min/max so charts treat as gaps)
    {
        Sample s;
        s.t_sec = 0;
        s.total = 0;
        s.delta = 0;
        s.avg_ms = std::numeric_limits<double>::quiet_NaN();
        s.min_ms = std::numeric_limits<double>::quiet_NaN();
        s.max_ms = std::numeric_limits<double>::quiet_NaN();
        std::lock_guard<std::mutex> lock(samples_mutex);
        samples.push_back(s);
    }

    std::cout << "Performance monitor example\n";
    std::cout << "  tasks=" << num_tasks << " claimers=" << num_claimers;
    std::cout << " durations=" << duration_ranges_str;
    if (!cfg.categories_str.empty()) std::cout << " categories=" << cfg.categories_str;
    if (!html_path.empty()) std::cout << " html=" << html_path;
    std::cout << " max_latency_samples=" << max_latency_samples << " max_task_details=" << max_task_details << "\n";

    TaskPlatform platform;
    auto metrics = std::make_shared<Metrics>();

    // Create claimers
    std::vector<std::shared_ptr<Claimer>> claimers;
    for (size_t i = 0; i < num_claimers; ++i) {
        auto c = std::make_shared<Claimer>("claimer-" + std::to_string(i+1), "claimer-" + std::to_string(i+1));
        std::string cat = categories.empty() ? std::string("default") : categories[i % categories.size()];
        c->add_category(cat);
        c->set_max_concurrent(1);
        platform.register_claimer(c);
        claimers.push_back(c);
    }

    // Handler: parse input as milliseconds and sleep
    auto handler_factory = [metrics, &latency_samples, &latency_mutex, &max_latency_samples, &sample_every,
                         &overshoot_samples, &overshoot_mutex,
                         &interval_min_ns, &interval_max_ns,
                         &claimer_mutex, &claimer_stats, &task_details_mutex, &task_details, &max_task_details,
                         &start_time, &global_event_times_ms, &global_event_mutex, &max_event_samples,
                         &claimer_event_times_ms, &claimer_event_mutex, &sampling_overhead_ns, &sampling_ops]
                        (Task &task, const std::string &input) -> TaskResult {
        int ms = 0;
        try {
            ms = std::stoi(input);
        } catch (...) {
            ms = 0;
        }
        auto start = std::chrono::steady_clock::now();
        if (ms > 0) sleep_for_ms(ms);
        auto end = std::chrono::steady_clock::now();
        uint64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        metrics->total_executions.fetch_add(1, std::memory_order_relaxed);
        metrics->total_latency_ns.fetch_add(ns, std::memory_order_relaxed);

        // update global min/max
        uint64_t old_min = metrics->min_latency_ns.load(std::memory_order_relaxed);
        while (ns < old_min && !metrics->min_latency_ns.compare_exchange_weak(old_min, ns, std::memory_order_relaxed)) {}
        uint64_t old_max = metrics->max_latency_ns.load(std::memory_order_relaxed);
        while (ns > old_max && !metrics->max_latency_ns.compare_exchange_weak(old_max, ns, std::memory_order_relaxed)) {}

        // update per-interval min/max (allow monitor to read+reset)
        // interval_min starts at ULLONG_MAX; interval_max starts at 0
        uint64_t im = interval_min_ns.load(std::memory_order_relaxed);
        while (ns < im && !interval_min_ns.compare_exchange_weak(im, ns, std::memory_order_relaxed)) {}
        uint64_t iM = interval_max_ns.load(std::memory_order_relaxed);
        while (ns > iM && !interval_max_ns.compare_exchange_weak(iM, ns, std::memory_order_relaxed)) {}
        // sampled latency collection (cap and downsampling)
        // start timing of sampling work (to measure sampling overhead)
        auto _samp_start = std::chrono::steady_clock::now();
        uint64_t exec_count = metrics->total_executions.load(std::memory_order_relaxed);
        if (sample_every == 1 || (exec_count % sample_every) == 0) {
            std::lock_guard<std::mutex> lock(latency_mutex);
            if (latency_samples.size() < max_latency_samples) latency_samples.push_back(ns);
            // record sleep overshoot (actual ns - requested ns)
            int64_t requested_ns = static_cast<int64_t>(ms) * 1000000LL;
            int64_t overshoot = static_cast<int64_t>(ns) - requested_ns;
            std::lock_guard<std::mutex> lock2(overshoot_mutex);
            if (overshoot_samples.size() < max_latency_samples) overshoot_samples.push_back(overshoot);
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
        // end timing of sampling work and record
        auto _samp_end = std::chrono::steady_clock::now();
        uint64_t _samp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(_samp_end - _samp_start).count();
        sampling_overhead_ns.fetch_add(_samp_ns, std::memory_order_relaxed);
        sampling_ops.fetch_add(1, std::memory_order_relaxed);

        TaskResult r("ok");
        return r;
    };

    // Publish tasks (each task re-uses the same handler)
    for (size_t i = 0; i < num_tasks; ++i) {
        auto builder = platform.task_builder();
        std::string task_cat = categories.empty() ? std::string("default") : categories[i % categories.size()];
        builder.title("perf-task")
               .category(task_cat)
               .priority(50)
               .auto_cleanup(false)
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
            auto &c = claimers[i];
            // duration ranges are required; pick a range uniformly then sample within it
            std::uniform_int_distribution<size_t> range_idx_dist(0, duration_ranges.size()-1);
            while (!done.load(std::memory_order_acquire)) {
                auto tasks = c->claim_tasks_to_capacity();
                if (tasks.empty()) {
                    // nothing to claim right now
                    sleep_for_ms(20);
                    continue;
                }
                for (auto &t : tasks) {
                    // pick a random range then sample uniformly within it
                    size_t ri = range_idx_dist(rng);
                    auto pr = duration_ranges[ri];
                    std::uniform_int_distribution<int> d(pr.first, std::max(pr.first, pr.second));
                    int ms = d(rng);
                    auto res = c->run_task(t, std::to_string(ms));
                    if (!res.ok()) {
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
        uint64_t last_total_executions = 0;
        uint64_t last_total_latency_ns = 0;
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
            // global average for console (unchanged)
            uint64_t executed = total;
            double avg_ms = executed ? (double)tot_lat_ns / executed / 1e6 : 0.0;
            uint64_t min_ns = metrics->min_latency_ns.load(std::memory_order_relaxed);
            uint64_t max_ns = metrics->max_latency_ns.load(std::memory_order_relaxed);

            // compute per-interval average using deltas from last sampled totals
            uint64_t delta_exec = (total > last_total_executions) ? (total - last_total_executions) : 0;
            uint64_t delta_lat_ns = (tot_lat_ns >= last_total_latency_ns) ? (tot_lat_ns - last_total_latency_ns) : 0;
            double interval_avg_ms = std::numeric_limits<double>::quiet_NaN();
            if (delta_exec > 0) {
                interval_avg_ms = (double)delta_lat_ns / (double)delta_exec / 1e6;
            }
            // update last totals for next interval
            last_total_executions = total;
            last_total_latency_ns = tot_lat_ns;

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
                // use per-interval avg (not global) to make avg/min/max comparable
                s.avg_ms = interval_avg_ms;
                // use per-interval min/max (read and reset interval atomics)
                uint64_t im = interval_min_ns.exchange(ULLONG_MAX, std::memory_order_acq_rel);
                uint64_t iM = interval_max_ns.exchange(0, std::memory_order_acq_rel);
                s.min_ms = (im==ULLONG_MAX?std::numeric_limits<double>::quiet_NaN():(im/1e6));
                s.max_ms = (iM==0?std::numeric_limits<double>::quiet_NaN():(iM/1e6));
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
                    ps.avg_ms = (cs.completed ? (double)cs.total_latency_ns / cs.completed / 1e6 : std::numeric_limits<double>::quiet_NaN());
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
        sleep_for_ms(100);
    }

    // signal threads to stop and join
    done.store(true, std::memory_order_release);
    for (auto &t : threads) if (t.joinable()) t.join();
    if (monitor.joinable()) monitor.join();

    // push a final sample to capture end state
    {
        auto now = std::chrono::steady_clock::now();
        uint64_t t_ms_final = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
        uint64_t t_sec = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        uint64_t total = metrics->total_executions.load();
        uint64_t last_total = 0;
        Sample s;
        s.t_ms = t_ms_final;
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

            // overshoot statistics (ns -> ms)
            double overshoot_avg_ms = 0.0;
            double overshoot_p90 = 0.0, overshoot_p99 = 0.0;
            double overshoot_max_ms = 0.0;
            {
                std::vector<int64_t> over_copy;
                {
                    std::lock_guard<std::mutex> lock(overshoot_mutex);
                    over_copy = overshoot_samples;
                }
                if (!over_copy.empty()) {
                    double sum = 0.0;
                    int64_t minv = over_copy[0], maxv = over_copy[0];
                    for (auto v : over_copy) { sum += (double)v; minv = std::min<int64_t>(minv, v); maxv = std::max<int64_t>(maxv, v); }
                    overshoot_avg_ms = (sum / (double)over_copy.size()) / 1e6;
                    overshoot_max_ms = maxv / 1e6;
                    std::sort(over_copy.begin(), over_copy.end());
                    auto at_over = [&](double q)->double{
                        size_t idx = std::min<size_t>(over_copy.size()-1, (size_t)std::floor(q * over_copy.size()));
                        return over_copy[idx] / 1e6;
                    };
                    overshoot_p90 = at_over(0.90);
                    overshoot_p99 = at_over(0.99);
                }
            }
            std::cout << "Overshoot stats: avg_ms=" << std::fixed << std::setprecision(6) << overshoot_avg_ms << " p90_ms=" << overshoot_p90 << " p99_ms=" << overshoot_p99 << " max_ms=" << overshoot_max_ms << "\n";


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
                << "<style>body{font-family:Arial,Helvetica,sans-serif;margin:12px;font-size:13px;line-height:1.25;color:#222} table{border-collapse:collapse;font-size:0.9em} td,th{border:1px solid #ddd;padding:4px} th{background:#f8f8f8} h1{font-size:1.25em;margin-bottom:6px} h2{font-size:1.05em;margin-top:10px;margin-bottom:6px} h3{font-size:0.95em;margin:4px 0} .muted{color:#666;font-size:0.85em} .claimer-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:8px;margin:6px 0} .claimer-card{display:block;padding:6px;border:1px solid #eee;border-radius:4px;background:#fff} .claimer-card h3{font-size:0.9em;margin:2px 0 6px} .claimer-card .muted{font-size:0.85em} .claimer-card canvas{width:100%;height:48px}</style>\n"
                << "</head><body>\n";
            ofs << "<style>.claimer-card canvas{height:48px !important;max-height:48px !important;display:block;box-sizing:border-box!important}</style>\n";
            ofs << "<h1>性能监控报告</h1>\n";
            ofs << "<p class='muted'>本报告汇总任务吞吐、延迟与各 Claimer 的时间序列，便于分析性能表现与定位瓶颈。</p>\n";
            ofs << "<h2>总览</h2>\n";
            ofs << "<p class='muted'>显示总体完成任务数、失败数、平均/最小/最大延迟与百分位（P50/P90/P99），用于快速评估系统整体性能。</p>\n";
            ofs << "<table>\n";
            ofs << "<tr><th>总任务数</th><th>已完成</th><th>失败数</th><th>平均延迟 (ms)</th><th>最小延迟 (ms)</th><th>最大延迟 (ms)</th><th>P50 (ms)</th><th>P90 (ms)</th><th>P99 (ms)</th><th>采样总开销 (ms)</th><th>平均采样 (µs)</th><th>采样次数</th><th>平均过冲 (ms)</th><th>过冲 P90 (ms)</th><th>过冲 P99 (ms)</th><th>过冲最大 (ms)</th><th>Claimer数</th></tr>\n";
            ofs << "<tr><td>" << stats.total_tasks << "</td><td>" << stats.completed_tasks << "</td><td>" << metrics->failures.load() << "</td><td>" << std::fixed << std::setprecision(3) << avg_ms << "</td><td>" << (min_ns==ULLONG_MAX?0.0:(min_ns/1e6)) << "</td><td>" << (max_ns/1e6) << "</td><td>" << std::fixed << std::setprecision(3) << p50 << "</td><td>" << std::fixed << std::setprecision(3) << p90 << "</td><td>" << std::fixed << std::setprecision(3) << p99 << "</td><td>" << std::fixed << std::setprecision(3) << (sampling_overhead_ns.load() / 1e6) << "</td><td>" << std::fixed << std::setprecision(6) << (sampling_ops.load() ? (sampling_overhead_ns.load() / (double)sampling_ops.load() / 1e6 * 1000.0) : 0.0) << "</td><td>" << sampling_ops.load() << "</td><td>" << std::fixed << std::setprecision(6) << overshoot_avg_ms << "</td><td>" << std::fixed << std::setprecision(6) << overshoot_p90 << "</td><td>" << std::fixed << std::setprecision(6) << overshoot_p99 << "</td><td>" << std::fixed << std::setprecision(6) << overshoot_max_ms << "</td><td>" << stats.total_claimers << "</td></tr>\n";
            ofs << "</table>\n";

            // 测试输入参数及平均延迟评估
            std::string _eval_msg = "平均延迟由所提供的时长区间决定；若观测值明显异常，请检查时长区间、采样频率或系统调度情况。";

            ofs << "<h2>测试输入参数</h2>\n";
            ofs << "<table>\n";
            ofs << "<tr><th>总任务数</th><th>Claimers</th><th>时长分布</th><th>Categories</th><th>采样间隔 (ms)</th><th>最大延迟样本数</th><th>最大任务详情数</th><th>最大事件样本数</th></tr>\n";
            ofs << "<tr><td>" << num_tasks << "</td><td>" << num_claimers << "</td><td>" << duration_ranges_str << "</td><td>" << (cfg.categories_str.empty()?std::string("-"):cfg.categories_str) << "</td><td>" << sample_interval_ms << "</td><td>" << max_latency_samples << "</td><td>" << max_task_details << "</td><td>" << max_event_samples << "</td></tr>\n";
            ofs << "</table>\n";

            ofs << "<h3>关于\"平均延迟\"的说明与评估</h3>\n";
            ofs << "<p class='muted'>\"平均延迟\"是对所有已完成任务的平均处理时间（单位：ms），计算方法为：总延迟 / 已完成任务数。该指标受时长分布、Claimers 数目与采样间隔影响。</p>\n";
            ofs << "<p class='muted'>任务时长分布说明：必须在命令行提供时长范围字符串（例如 \"0-1,1-1,2-8\"），示例会先从这些区间中均匀选取一个区间，然后在该区间内均匀采样一个整数毫秒值作为任务时长。测量使用 <code>steady_clock</code> 以纳秒精度计时并汇总为 ms 显示（例如观测到的最小延迟可能为 1.063 ms），报告中会列出采样测量的平均开销以便评估观测误差。注意：若指定了包含 0 的区间（例如 0-1），0 表示不调用 sleep（即时完成），测得延迟可能接近 0，这取决于调度与计时精度。</p>\n";
            // durations are integer milliseconds; compute two expectations:
            // 1) expected_range_uniform: select a range uniformly, then sample integer uniformly inside it (current algorithm)
            // 2) expected_value_uniform: select an integer uniformly from the union of all integer values across ranges
            double expected_range_uniform = 0.0;
            double expected_value_uniform = 0.0;
            if (!duration_ranges.empty()) {
                double sum_mid = 0.0;
                double total_count = 0.0;
                double total_sum_values = 0.0;
                for (const auto &pr : duration_ranges) {
                    int a = pr.first;
                    int b = pr.second;
                    if (b < a) continue;
                    double mid = (a + b) / 2.0; // exact expectation for uniform integer [a,b]
                    sum_mid += mid;
                    double count = static_cast<double>(b - a + 1);
                    total_count += count;
                    double range_sum = (static_cast<double>(a + b) * count) / 2.0; // sum of integers in [a,b]
                    total_sum_values += range_sum;
                }
                expected_range_uniform = sum_mid / duration_ranges.size();
                expected_value_uniform = total_count > 0.0 ? (total_sum_values / total_count) : 0.0;
            }
            double diff_range = avg_ms - expected_range_uniform;
            double diff_range_pct = (expected_range_uniform > 0.0) ? (diff_range / expected_range_uniform * 100.0) : 0.0;
            double diff_value = avg_ms - expected_value_uniform;
            double diff_value_pct = (expected_value_uniform > 0.0) ? (diff_value / expected_value_uniform * 100.0) : 0.0;

            ofs << "<p>任务时长以整数毫秒表示。当前示例的抽样方式是 <strong>先均匀选区间再区间内均匀采样</strong>，该方式的期望单任务时长: <strong>" << std::fixed << std::setprecision(3) << expected_range_uniform << " ms</strong>。实际测得平均延迟: <strong>" << std::fixed << std::setprecision(3) << avg_ms << " ms</strong>，差异: <strong>" << std::fixed << std::setprecision(3) << diff_range << " ms</strong>（约 " << std::fixed << std::setprecision(2) << diff_range_pct << "%）。</p>\n";
            // reference value toggle (hidden by default)
            ofs << "<div style='margin-top:6px;margin-bottom:4px;'><button id=\"btn-ref\" onclick=\"toggleReferenceValues()\">显示参考值</button></div>\n";
            ofs << "<div id=\"reference-values\" style=\"display:none\" class=\"muted\">作为参考，若按所有可能的整数值均匀抽样（按值均匀），期望为 <strong>" << std::fixed << std::setprecision(3) << expected_value_uniform << " ms</strong>，与实际的差异为 <strong>" << std::fixed << std::setprecision(3) << diff_value << " ms</strong>（约 " << std::fixed << std::setprecision(2) << diff_value_pct << "%）。</div>\n";
            // include sampling overhead note
            double _sampling_avg_ms = sampling_ops.load() ? (sampling_overhead_ns.load() / (double)sampling_ops.load() / 1e6) : 0.0;
            double _sampling_pct = (avg_ms > 0.0) ? (_sampling_avg_ms / avg_ms * 100.0) : 0.0;
            ofs << "<p class='muted'>测得平均每次采样开销: <strong>" << std::fixed << std::setprecision(6) << _sampling_avg_ms * 1000 << " µs</strong>（采样次数=" << sampling_ops.load() << "），占平均延迟约 <strong>" << std::fixed << std::setprecision(2) << _sampling_pct << "%</strong>。若占比较大（例如 >5%），采样本身会显著影响测量结果，应考虑降低采样频率或改用轻量级采样方案。</p>\n";
            ofs << "<p class='muted'>\"过冲（overshoot）\" 定义为实际耗时减去请求的 sleep 时长（单位：ms）。它反映操作系统唤醒延迟与调度抖动。测得平均过冲: <strong>" << std::fixed << std::setprecision(6) << overshoot_avg_ms << " ms</strong>，P90: <strong>" << std::fixed << std::setprecision(6) << overshoot_p90 << " ms</strong>，P99: <strong>" << std::fixed << std::setprecision(6) << overshoot_p99 << " ms</strong>，最大过冲: <strong>" << std::fixed << std::setprecision(6) << overshoot_max_ms << " ms</strong>。</p>\n";
            ofs << "<p class='muted'>" << _eval_msg << "</p>\n";

            ofs << "<h2>吞吐量随时间变化</h2>\n";
            ofs << "<p class='muted'>每个采样间隔内完成的任务数，用于观察吞吐波动与突发。</p>\n";
            ofs << "<canvas id=\"chart-throughput\" width=\"" << width << "\" height=\"" << height << "\"></canvas>\n";
            ofs << "<div style='margin-top:8px;'><button onclick=\"resetZoomAll()\">重置缩放</button></div>\n";

            // per-claimer canvases
            ofs << "<h2>按 Claimer 的时间序列</h2>\n";
            ofs << "<p class='muted'>每个 Claimer 在各采样点的完成数及平均延迟（以小图示呈现，便于快速比较）。</p>\n";
            ofs << "<div class='claimer-grid'>\n";
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
                    ofs << "<canvas id=\"chart-claimer-" << safe_id << "\" width=\"220\" height=\"48\"></canvas>\n";
                    const auto &cs = claimer_stats[cid];
                    double avg = cs.completed ? (double)cs.total_latency_ns / cs.completed / 1e6 : 0.0;
                    ofs << "<div class='muted'>已完成=" << cs.completed << " 平均延迟(ms)=" << std::fixed << std::setprecision(3) << avg << "</div>\n";
                    ofs << "</div>\n";
                }
            }
            ofs << "</div>\n";

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
                    if (std::isnan(s.avg_ms)) ofs << "null"; else ofs << s.avg_ms;
                    first = false;
                }
            }
            ofs << "];\n";

            ofs << "const globalMin = [";
            {
                std::lock_guard<std::mutex> lock(samples_mutex);
                bool first = true;
                for (const auto &s : samples) {
                    if (!first) ofs << ",";
                    if (std::isnan(s.min_ms)) ofs << "null"; else ofs << s.min_ms;
                    first = false;
                }
            }
            ofs << "];\n";

            ofs << "const globalMax = [";
            {
                std::lock_guard<std::mutex> lock(samples_mutex);
                bool first = true;
                for (const auto &s : samples) {
                    if (!first) ofs << ",";
                    if (std::isnan(s.max_ms)) ofs << "null"; else ofs << s.max_ms;
                    first = false;
                }
            }
            ofs << "];\n";

            ofs << "const charts = {};\n";
            ofs << "const ctx = document.getElementById('chart-throughput').getContext('2d');\n";
            ofs << "charts['throughput'] = new Chart(ctx, { type: 'line', data: { labels: labels, datasets: [{ label: '每间隔完成数', data: globalDelta, borderColor: 'steelblue', backgroundColor: 'rgba(70,130,180,0.2)', yAxisID: 'y' }, { label: '平均延迟 (ms)', data: globalAvg, borderColor: 'darkorange', backgroundColor: 'rgba(255,165,0,0.2)', yAxisID: 'y1' }] }, options: { responsive: true, scales: { x: { grid: { display: true } }, y: { type: 'linear', position: 'left' }, y1: { type: 'linear', position: 'right', grid: { drawOnChartArea: false } } }, plugins: { zoom: { pan: { enabled: true, mode: 'x' }, zoom: { wheel: { enabled: true }, pinch: { enabled: true }, mode: 'x' } }, tooltip: { callbacks: { title: function(items){ return items && items.length ? items[0].label : ''; } } } } } });\n";
ofs << "// samples chart (duplicates labels/globalDelta/globalAvg for an interactive samples view)\n";
            ofs << "(function(){\n";
            ofs << "  function initSamples(){ var el=document.getElementById('chart-samples'); if(!el) return false; var ctxS = el.getContext('2d'); charts['samples'] = new Chart(ctxS, { type: 'line', data: { labels: labels, datasets: [ { label: '平均延迟（样本点）', data: globalAvg, showLine: false, pointRadius: 3, pointBackgroundColor: 'darkorange', borderColor: 'transparent' }, { label: '最小延迟', data: globalMin, borderColor: 'rgba(200,200,200,0.25)', pointRadius: 0, fill: '+1', backgroundColor: 'rgba(200,200,200,0.12)' }, { label: '最大延迟', data: globalMax, borderColor: 'rgba(200,200,200,0.25)', pointRadius: 0, fill: false } ] }, options: { responsive: false, maintainAspectRatio: false, scales: { x: { grid: { display: true } }, y: { type: 'linear', position: 'left', title: { display: true, text: '延迟 (ms)' } } }, plugins: { legend: { display: false }, tooltip: { callbacks: { title: function(items){ return items && items.length ? items[0].label : ''; } } }, zoom: { pan: { enabled: true, mode: 'x' }, zoom: { wheel: { enabled: true }, pinch: { enabled: true }, mode: 'x' } } }, interaction: { mode: 'nearest', intersect: true } } }); return true; }\n";
            ofs << "  if(!initSamples()){ document.addEventListener('DOMContentLoaded', initSamples); }\n";
            ofs << "})();\n";
            ofs << "function toggleSamplesTable(){ var t=document.getElementById('samples-table'); if(!t) return; t.style.display = (t.style.display==='none') ? 'table' : 'none'; }\n";
            ofs << "function toggleReferenceValues(){ var d=document.getElementById('reference-values'); var b=document.getElementById('btn-ref'); if(!d || !b) return; if(d.style.display==='none'){ d.style.display='block'; b.textContent='隐藏参考值'; } else { d.style.display='none'; b.textContent='显示参考值'; } }\n";
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
                        if (std::isnan(ps.avg_ms)) ofs << "null"; else ofs << ps.avg_ms;
                        first = false;
                    }
                    ofs << "];\n";
                    ofs << "const ctx = document.getElementById('chart-claimer-" << safe_id << "').getContext('2d');\n";
                    ofs << "charts['claimer-" << safe_id << "'] = new Chart(ctx, { type: 'line', data: { labels: labels, datasets: [{ label: '每间隔完成数', data: deltaData, borderColor: 'steelblue', backgroundColor: 'rgba(70,130,180,0.15)', fill: true, yAxisID: 'y' }, { label: '平均延迟 (ms)', data: avgData, borderColor: 'darkorange', backgroundColor: 'rgba(255,165,0,0.15)', fill: false, yAxisID: 'y1' }] }, options: { responsive: true, maintainAspectRatio: false, plugins: { legend: { display: false }, tooltip: { callbacks: { title: function(items){ return items && items.length ? items[0].label : ''; } } }, zoom: { pan: { enabled: false }, zoom: { wheel: { enabled: false }, pinch: { enabled: false } } } }, elements: { point: { radius: 0 }, line: { tension: 0 } }, scales: { x: { display: false }, y: { display: false }, y1: { display: false } }, interaction: { mode: 'index', intersect: false } } });\n",
                    ofs << "})();\n";
                }
            }

            // ensure small claimer charts are non-responsive to avoid resize loops then update them
            ofs << "for (var k in charts){ if (k.startsWith && k.startsWith('claimer-') && charts[k]){ charts[k].options.responsive=false; try{ charts[k].update(); }catch(e){} } }\n";
            ofs << "function resetZoomAll(){ for (var k in charts) if (charts[k] && charts[k].resetZoom) charts[k].resetZoom(); }\n";
            ofs << "</script>\n";

            ofs << "<h2>样本列表</h2>\n";
            ofs << "<p class='muted'>采样点的时间戳（mm:ss.ms）、完成数与平均延迟，既可在图表中查看，也可导出为表格。</p>\n";
            ofs << "<div style='margin-bottom:6px;'><button onclick=\"toggleSamplesTable()\">切换表格/图表</button> <button onclick=\"resetZoomAll()\">重置缩放</button></div>\n";
            ofs << "<canvas id=\"chart-samples\" width=\"800\" height=\"140\" style=\"height:140px!important;max-height:140px!important;display:block;box-sizing:border-box!important;\"></canvas>\n";
            ofs << "<table id=\"samples-table\" style=\"display:none\">\n<tr><th>时间</th><th>总数</th><th>区间增量</th><th>平均延迟(ms)</th><th>最小延迟(ms)</th><th>最大延迟(ms)</th></tr>\n";
            {
                std::lock_guard<std::mutex> lock(samples_mutex);
                for (const auto &s : samples) {
                    ofs << "<tr><td>" << _fmt_ms(s.t_ms) << "</td><td>" << s.total << "</td><td>" << s.delta << "</td><td>";
                    if (std::isnan(s.avg_ms)) ofs << "-"; else ofs << std::fixed << std::setprecision(3) << s.avg_ms;
                    ofs << "</td><td>";
                    if (std::isnan(s.min_ms)) ofs << "-"; else ofs << s.min_ms;
                    ofs << "</td><td>";
                    if (std::isnan(s.max_ms)) ofs << "-"; else ofs << s.max_ms;
                    ofs << "</td></tr>\n";
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
