// 【模块导读 · 日志吞吐量怎么算】
// 这里负责一轮压测的计时。bench/logger.cc 负责配置同步或异步日志器，再把日志器名、线程数、每条正文长度和目标条数传进来。
// 每个线程用同一份正文连续调用日志器，单独记录自己的耗时。主线程等全部线程结束，再用最慢线程的时间估算总吞吐。
// 同步组计入调用线程写 Sink 的时间；异步组统计提交循环耗时，后台最终写完可能更晚。用数据比较时先区分这两个口径。
#ifndef __M_BENCH_H__
#define __M_BENCH_H__
#include "../logs/bitlog.h"
#include <chrono>

namespace bitlog {
// thread_num 必须大于 0，因为下面要用它做除法。msglen 是正文的字节数，msg_count 是希望提交的总条数。
void bench(const std::string &loger_name, size_t thread_num, size_t msglen, size_t msg_count)
{
    // 基准函数以名称查找已经构建并注册的日志器。没有查到就返回，后续线程会共享这一个有效的实例。
    Logger::ptr lp = getLogger(loger_name);
    if (lp.get() == nullptr) return;
    // 创建由 msglen 个字符 '1' 组成的正文。线程只读取它，保证不同模式比较时正文完全一致。
    std::string msg(msglen, '1');
    // 平均分配为整除。例如目标 10 条、线程数 3，则每线程 3 条，实际发出 9 条。当前汇总仍使用输入的 10 条。
    size_t msg_count_per_thread = msg_count / thread_num;
    // 耗时数组按线程数建立，每个线程独占自己的槽位。threads 保存所有线程对象，主线程稍后逐个 join。
    std::vector<double> cost_time(thread_num);
    std::vector<std::thread> threads;
    std::cout << "输入线程数量: " << thread_num << std::endl;
    std::cout << "输出日志数量: " << msg_count << std::endl;
    std::cout << "输出日志大小: " << msglen * msg_count / 1024  << "KB" << std::endl;
    // lambda 按值捕获 i，确保每个线程记录自己的编号；日志器、正文和结果容器等通过引用访问，主线程在离开函数前会 join。
    for (int i = 0; i < thread_num; i++) {
        threads.emplace_back([&, i](){
            // 开始和结束时间包住日志调用循环，单线程吞吐用实际循环次数除以该线程用时。
            auto start = std::chrono::high_resolution_clock::now();
            for(size_t j = 0; j < msg_count_per_thread; j++) {
                // fatal 在这里选中最高普通级别，用于通过测试的默认阈值；它只是记录消息，不会结束测试进程。
                lp->fatal("%s", msg.c_str());
            }
            auto end = std::chrono::high_resolution_clock::now();
            // 把计时差换算成以秒为单位的 double，允许保留小数；随后保存耗时并打印该线程的估算速率。
            auto cost = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
            cost_time[i] = cost.count();
            auto avg = msg_count_per_thread / cost_time[i];
            std::cout << "线程" << i << "耗时: " << cost.count() << "s" << " 平均：" << (size_t)avg << "/s\n";
        });
    }
    // join 保证线程都已结束，主线程这时可以安全读完耗时数组。异步写入线程属于日志器，此处 join 的是生产线程。
    for(auto &thr : threads) {
        thr.join();
    }
    // 取最慢生产线程的耗时作为并行测试用时估计。没有额外的起跑屏障，所以这不是精确的整段墙钟时间。
    double max_cost = 0;
    for (auto cost : cost_time) max_cost = max_cost < cost ? cost : max_cost;
    std::cout << "总消耗时间: " << max_cost << std::endl;
    // 条数除以秒数得到条每秒，正文总字节数除以秒再除以 1024 两次得到 MiB 每秒。显示文本沿用源码里的 MB。
    std::cout << "平均每秒输出: " << (size_t)(msg_count / max_cost) << "条日志" << std::endl;
    std::cout << "平均每秒输出: " << (size_t)(msglen * msg_count / max_cost / 1024 / 1024) << "MB" << std::endl;
}
}

#endif
