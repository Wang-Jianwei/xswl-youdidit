#include <xswl/youdidit/youdidit.hpp>
#include <xswl/youdidit/web/web_dashboard.hpp>
#include <xswl/youdidit/web/web_server.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <atomic>

using namespace xswl::youdidit;

int main(int argc, char** argv) {
    std::cout << "======================================\n"
              << "xswl-youdidit Web Dashboard Demo\n"
              << "======================================\n\n";
    
    // 创建任务平台
    TaskPlatform platform;
    
    // 创建申领者
    auto claimer1 = std::make_shared<Claimer>("Worker-1", "Worker 1");
    auto claimer2 = std::make_shared<Claimer>("Worker-2", "Worker 2");
    
    claimer1->add_category("demo");
    claimer2->add_category("demo");
    
    platform.register_claimer(claimer1);
    platform.register_claimer(claimer2);
    
    std::cout << "✓ 已注册 2 个申领者\n\n";
    
    // 解析运行时长参数
    int duration_seconds = -1; // 默认持续运行
    int port = 9999;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--duration" && i + 1 < argc) {
            try {
                duration_seconds = std::stoi(argv[++i]);
            } catch (...) {
                std::cerr << "无效的 --duration 参数，使用默认持续运行\n";
                duration_seconds = -1;
            }
        } else if (arg == "--port" && i + 1 < argc) {
            try {
                port = std::stoi(argv[++i]);
            } catch (...) {
                std::cerr << "无效的 --port 参数，使用默认 9999\n";
                port = 9999;
            }
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "用法: example_web_demo [--duration 秒] [--port 端口]\n";
            std::cout << "  --duration N   指定运行秒数（默认持续运行）\n";
            std::cout << "  --port P       指定监听端口（默认 9999）\n";
            return 0;
        }
    }

    // 创建仪表板
    WebDashboard dashboard(&platform);
    
    // 创建Web服务器
    WebServer web_server(&dashboard, port);
    // 对于 Codespaces/容器转发，使用 127.0.0.1；本地开发用 0.0.0.0
#ifdef __CODESPACES__
    web_server.set_host("127.0.0.1");
#else
    web_server.set_host("0.0.0.0");
#endif
    
    std::cout << "启动Web服务器...\n";
    web_server.start();
    
    if (!web_server.is_running()) {
        std::cerr << "✗ 无法启动Web服务器\n";
        return 1;
    }
    
    std::cout << "✓ Web服务器已启动\n";
    std::cout << "\n📊 访问地址: http://localhost:" << port << "\n\n";
    
    // 控制线程运行
    std::atomic<bool> keep_running{true};
    std::atomic<int> task_counter{1};
    
    // 启动申领者工作线程
    auto start_claimer_worker = [&](std::shared_ptr<Claimer> claimer) {
        return std::thread([&keep_running, claimer]() {
            while (keep_running) {
                // 尝试申领任务到容量上限
                auto tasks = claimer->claim_tasks_to_capacity();
                
                if (!tasks.empty()) {
                    // 状态现在会根据 claimed_task_count 自动计算为 Busy/Idle
                    
                    // 执行已申领的任务
                    for (auto& task : tasks) {
                        // 稍微延迟一下让Claimed状态可以被观察到
                        std::this_thread::sleep_for(std::chrono::milliseconds(300));
                        
                        // 使用 run_task：一次调用完成执行和记账
                        // 内部会自动：执行任务 -> complete_task/abandon_task
                        claimer->run_task(task, "");
                    }
                }
                // 状态会自动变为 Idle（当 claimed_task_count == 0 时）
                
                // 短暂休息后继续
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
            }
        });
    };
    
    std::cout << "启动申领者工作线程...\n";
    std::thread worker1_thread = start_claimer_worker(claimer1);
    std::thread worker2_thread = start_claimer_worker(claimer2);
    std::cout << "✓ 申领者工作线程已启动\n\n";
    
    // 任务类型定义
    struct TaskType {
        std::string name;
        std::string category;
        int min_duration_ms;
        int max_duration_ms;
        double success_rate;
        int priority;
    };
    
    std::vector<TaskType> task_types = {
        {"数据处理", "data", 2000, 10000, 0.95, 3},
        {"文件导入", "import", 500, 2000, 0.90, 2},
        {"报表生成", "report", 1000, 3000, 0.85, 1},
        {"数据清理", "cleanup", 300, 800, 0.98, 2},
        {"数据验证", "validation", 100, 500, 0.92, 3},
        {"邮件发送", "email", 400, 1200, 0.88, 1},
        {"备份任务", "backup", 2000, 5000, 0.95, 1},
        {"快速查询", "query", 50, 200, 0.99, 3}
    };
    
    // 启动任务生成器线程
    std::thread task_generator([&]() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> interval_dist(1, 3); // 1-3秒间隔
        std::uniform_int_distribution<> type_dist(0, task_types.size() - 1);
        std::uniform_real_distribution<> success_dist(0.0, 1.0);
        
        while (keep_running) {
            // 随机等待一段时间
            int wait_seconds = interval_dist(gen);
            for (int i = 0; i < wait_seconds * 10 && keep_running; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            if (!keep_running) break;
            
            // 随机选择任务类型
            auto& task_type = task_types[type_dist(gen)];
            std::uniform_int_distribution<> duration_dist(task_type.min_duration_ms, task_type.max_duration_ms);
            
            int task_id = task_counter++;
            int duration_ms = duration_dist(gen);
            double success_threshold = task_type.success_rate;
            
            auto builder = platform.task_builder();
            builder.title(task_type.name + " #" + std::to_string(task_id))
                   .category(task_type.category)
                   .priority(task_type.priority)
                   .handler([duration_ms, success_threshold, success_dist](Task&, const std::string&) mutable -> TaskResult {
                       // 模拟任务处理
                       std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
                       
                       // 根据成功率决定是否成功
                       std::random_device rd_local;
                       bool success = success_dist(rd_local) < success_threshold;
                       
                       if (success) {
                           TaskResult result("成功完成");
                           return result;
                       } else {
                           return Error("处理失败", ErrorCode::TASK_EXECUTION_FAILED);
                       }
                   });
            
            auto task = builder.build();
            platform.publish_task(task);
            
            std::cout << "📝 发布新任务: " << task_type.name << " #" << task_id 
                      << " (类别: " << task_type.category 
                      << ", 优先级: " << task_type.priority << ")\n";
        }
    });
    
    // 确保申领者都添加所有类别
    std::cout << "配置申领者类别...\n";
    for (auto& task_type : task_types) {
        claimer1->add_category(task_type.category);
        claimer2->add_category(task_type.category);
    }
    std::cout << "✓ 申领者已配置为处理所有类别的任务\n\n";

    // 运行指定时长或直到中断
    if (duration_seconds < 0) {
        std::cout << "运行直到被中断（Ctrl+C）...\n";
        while (keep_running) std::this_thread::sleep_for(std::chrono::seconds(1));
    } else {
        for (int i = 0; i < duration_seconds && keep_running; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    // 停止并清理
    keep_running = false;
    task_generator.join();
    worker1_thread.join();
    worker2_thread.join();

    web_server.stop();
    dashboard.stop_server();

    std::cout << "已停止，退出程序。\n";
    return 0;
}
