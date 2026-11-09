// 构造同步和异步两种文件日志器，用同一组参数比较不同生产线程数下的调用侧吞吐。
#include "../logs/bitlog.h"
#include "bench.h"

// 同步组为每次测试生成不同的注册名，所有同步测试追加到同一个相对路径文件。
void sync_bench_thread_log(size_t thread_count, size_t msg_count, size_t msglen)
{
    static int num = 1;
    std::string logger_name = "sync_bench_logger" + std::to_string(num++);
    LOGI("************************************************");
    LOGI("同步日志测试: %d threads, %d messages", thread_count, msg_count);

    // 格式只输出 %m，基准文件里不添加元数据或换行。同步模式由调用线程直接写入文件目标。
    bitlog::GlobalLoggerBuilder::ptr lbp(new bitlog::GlobalLoggerBuilder);
    lbp->buildLoggerName(logger_name);
    lbp->buildFormatter("%m");
    lbp->buildSink<bitlog::FileSink>("./logs/sync.log");
    lbp->buildLoggerType(bitlog::Logger::Type::LOGGER_SYNC);
    lbp->build(); 
    bitlog::bench(logger_name, thread_count, msglen, msg_count);
    LOGI("************************************************");
}
// 异步组也按测试轮次生成注册名，使用独立的追加文件。日志调用负责提交正文，文件写入由工作线程完成。
void async_bench_thread_log(size_t thread_count, size_t msg_count, size_t msglen)
{
    static int num = 1;
    std::string logger_name = "async_bench_logger" + std::to_string(num++);
    LOGI("************************************************");
    LOGI("异步日志测试: %d threads, %d messages", thread_count, msg_count);

    bitlog::GlobalLoggerBuilder::ptr lbp(new bitlog::GlobalLoggerBuilder);
    lbp->buildLoggerName(logger_name);
    lbp->buildFormatter("%m");
    lbp->buildSink<bitlog::FileSink>("./logs/async.log");
    lbp->buildLoggerType(bitlog::Logger::Type::LOGGER_ASYNC);
    lbp->build(); 
    bitlog::bench(logger_name, thread_count, msglen, msg_count);
    LOGI("************************************************");
}
// 两组都用 1 个和 5 个线程分别提交 100 万条、每条正文 100 字节的消息，便于对照模式与线程数。
void bench_test() {
    /*异步日志输出*/
    async_bench_thread_log(1, 1000000, 100);
    async_bench_thread_log(5, 1000000, 100);
    // 同步写日志
    sync_bench_thread_log(1, 1000000, 100);
    sync_bench_thread_log(5, 1000000, 100);
}

// 程序入口直接运行预设基准，命令行参数未用于调整负载。
int main(int argc, char *argv[])
{
    bench_test();

    return 0;
}
