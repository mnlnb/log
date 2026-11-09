// 【模块导读 · 配两组日志器做对照压测】
// main 调用 bench_test，分别配置异步和同步的文件日志器。每种模式都跑单线程与五线程，实际计时逻辑在 bench.h。
// 两组用相同的条数和正文大小，只输出正文，便于比较提交侧成本。同步写 "./logs/sync.log"，异步写 "./logs/async.log"。
// 文件以追加模式打开，重复运行会累积数据；用这个程序读代码时可先顺着一次配置走完，再看汇总公式。
#include "../logs/bitlog.h"
#include "bench.h"

// 同步组创建一个 SyncLogger，通过 build 将它登记，bench 函数才能用名字取到它。
void sync_bench_thread_log(size_t thread_count, size_t msg_count, size_t msglen)
{
    // 函数内的静态计数跨调用递增，构造不同注册名，避免两轮同步测试共用同一个键。两轮的目标文件路径相同。
    static int num = 1;
    std::string logger_name = "sync_bench_logger" + std::to_string(num++);
    LOGI("************************************************");
    LOGI("同步日志测试: %d threads, %d messages", thread_count, msg_count);

    // 建造者依次设置名称、模式、目标和同步类型。用 "%m" 只输出正文，时间、文件和换行都由这个模板省略。
    bitlog::GlobalLoggerBuilder::ptr lbp(new bitlog::GlobalLoggerBuilder);
    lbp->buildLoggerName(logger_name);
    lbp->buildFormatter("%m");
    lbp->buildSink<bitlog::FileSink>("./logs/sync.log");
    // 选择 LOGGER_SYNC 后，生产线程会在每次调用中进入 SyncLogger::logIt 写文件，写入耗时计入基准。
    lbp->buildLoggerType(bitlog::Logger::Type::LOGGER_SYNC);
    lbp->build(); 
    // 把名称和负载交给 bench，让测试函数查同一实例并启动生产线程。
    bitlog::bench(logger_name, thread_count, msglen, msg_count);
    LOGI("************************************************");
}
// 异步组的调用结构相同，区别是注册名、目标文件和类型。用相同负载有助于对照两种路径。
void async_bench_thread_log(size_t thread_count, size_t msg_count, size_t msglen)
{
    static int num = 1;
    std::string logger_name = "async_bench_logger" + std::to_string(num++);
    LOGI("************************************************");
    LOGI("异步日志测试: %d threads, %d messages", thread_count, msg_count);

    // 该组同样用 "%m" 和 FileSink，所以文件中的数据构成与同步组一致。两组使用不同文件路径。
    bitlog::GlobalLoggerBuilder::ptr lbp(new bitlog::GlobalLoggerBuilder);
    lbp->buildLoggerName(logger_name);
    lbp->buildFormatter("%m");
    lbp->buildSink<bitlog::FileSink>("./logs/async.log");
    // 选择 LOGGER_ASYNC 后，生产线程提交到 Looper，工作线程批量写文件。基准返回时后台可能还在处理。
    lbp->buildLoggerType(bitlog::Logger::Type::LOGGER_ASYNC);
    lbp->build(); 
    bitlog::bench(logger_name, thread_count, msglen, msg_count);
    LOGI("************************************************");
}
// 两种模式各跑两轮，每轮目标一百万条，每条正文 100 字节，线程数分别为 1 和 5。
void bench_test() {
    // 先测异步再测同步。各轮生产线程会在 bench 内 join，已注册异步日志器由管理器持有，其后台批次可能稍后完成。
    /*异步日志输出*/
    async_bench_thread_log(1, 1000000, 100);
    async_bench_thread_log(5, 1000000, 100);
    // 同步写日志
    sync_bench_thread_log(1, 1000000, 100);
    sync_bench_thread_log(5, 1000000, 100);
}

// 入口直接执行预设参数。想理解计时从这里进入 bench_test，然后在两种建造流程的 bench 调用处切换到 bench.h。
int main(int argc, char *argv[])
{
    bench_test();

    return 0;
}
