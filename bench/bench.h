// 多线程日志吞吐基准。对已注册的日志器发出固定长度正文，按调用侧耗时估算吞吐量。
#ifndef __M_BENCH_H__
#define __M_BENCH_H__
#include "../logs/bitlog.h"
#include <chrono>

namespace bitlog {
// thread_num 应为正数。每个线程发出 msg_count 除以线程数后取整得到的条数，余数不会分配。
void bench(const std::string &loger_name, size_t thread_num, size_t msglen, size_t msg_count)
{
    // 线程共享同一个日志器和只读正文。未找到指定日志器时直接结束本次测试。
    Logger::ptr lp = getLogger(loger_name);
    if (lp.get() == nullptr) return;
    std::string msg(msglen, '1');
    size_t msg_count_per_thread = msg_count / thread_num;
    std::vector<double> cost_time(thread_num);
    std::vector<std::thread> threads;
    std::cout << "输入线程数量: " << thread_num << std::endl;
    std::cout << "输出日志数量: " << msg_count << std::endl;
    std::cout << "输出日志大小: " << msglen * msg_count / 1024  << "KB" << std::endl;
    // 按值捕获当前线程编号。每个线程只写 cost_time 的对应元素，主线程在 join 之后读取。
    for (int i = 0; i < thread_num; i++) {
        threads.emplace_back([&, i](){
            // 计时只包围日志调用循环。同步日志包含写入目标的耗时，异步日志的后台刷写可能在计时结束后继续。
            auto start = std::chrono::high_resolution_clock::now();
            for(size_t j = 0; j < msg_count_per_thread; j++) {
                lp->fatal("%s", msg.c_str());
            }
            auto end = std::chrono::high_resolution_clock::now();
            auto cost = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
            cost_time[i] = cost.count();
            auto avg = msg_count_per_thread / cost_time[i];
            std::cout << "线程" << i << "耗时: " << cost.count() << "s" << " 平均：" << (size_t)avg << "/s\n";
        });
    }
    // 等待所有生产线程返回。这里没有等待异步日志器单独完成所有后台输出。
    for(auto &thr : threads) {
        thr.join();
    }
    // 取各线程耗时的最大值作为并行用时估计。速率按传入的总条数和正文长度换算，整除有余数时与实际发出的条数略有差异。
    double max_cost = 0;
    for (auto cost : cost_time) max_cost = max_cost < cost ? cost : max_cost;
    std::cout << "总消耗时间: " << max_cost << std::endl;
    std::cout << "平均每秒输出: " << (size_t)(msg_count / max_cost) << "条日志" << std::endl;
    std::cout << "平均每秒输出: " << (size_t)(msglen * msg_count / max_cost / 1024 / 1024) << "MB" << std::endl;
}
}

#endif
