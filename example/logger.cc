// 【模块导读 · 第一次读项目从这里开始】
// 这是日志库的完整用法示例。先看文件末尾 main，再看 functional_test 如何搭建日志器，最后到 loggerTest 看怎样发一条日志。
// 流程是 main → functional_test → GlobalLoggerBuilder::build → loggerTest。被登记的日志器叫 all_sink_logger，三个目标收到相同文本。
// 如果只想追踪一条消息，选 LOG_INFO(lp, "%s", "LOG_INFO")。从 bitlog.h 看宏展开，随后跟进 Logger::info、Formatter 和 AsyncLooper。
// 后半段会额外写一百万条日志，标准输出和文件量都较大。阅读时可以先跳过循环，掌握一条日志怎样到达三个目标。
#include "../logs/bitlog.h"


// loggerTest 通过名称查询之前注册的日志器。assert 验证查询返回有效指针，后续所有具名调用使用这个 lp。
void loggerTest(const std::string &logger_name) {
    bitlog::Logger::ptr lp = bitlog::getLogger(logger_name);
    assert(lp.get());
    // 首尾分隔线使用 LOGF，写到默认 root。中间的 lp 调用和带 lp 的 LOG_* 调用写到 all_sink_logger。
    LOGF("------------example--------------------");
    // 这一组使用形如 lp->info 的写法。bitlog.h 定义的函数式宏仍会在预处理时给这些调用加上文件和行号。
    lp->debug("%s", "logger->debug");
    lp->info("%s", "logger->info");
    lp->warn("%s", "logger->warn");
    lp->error("%s", "logger->error");
    lp->fatal("%s", "logger->fatal");
    // 这一组显式写 LOG_INFO(lp, ...)，阅读时对照 bitlog.h 的两层展开，可看到它和上一组最终都调用 Logger 对应级别的方法。
    LOG_DEBUG(lp, "%s", "LOG_DEBUG");
    LOG_INFO(lp, "%s", "LOG_INFO");
    LOG_WARN(lp, "%s", "LOG_WARN");
    LOG_ERROR(lp, "%s", "LOG_ERROR");
    LOG_FATAL(lp, "%s", "LOG_FATAL");
    LOGF("---------------------------------------");
    
    // 下面构造一段前缀，给每条测试日志附上自增编号。这样滚动或追加后仍能通过编号辨认先后。
    std::string log_msg = "hello bitejiuyeke-";
    size_t fsize = 0;
    size_t count = 0;
    // 每轮只提交一条 ERROR 记录，直到 count 达到一百万。ERROR 高于这里配置的 DEBUG 阈值，因此都通过过滤。
    while(count < 1000000) {
        std::string msg = log_msg + std::to_string(count++);
        lp->error("%s", msg.c_str());
    }
}
// functional_test 负责配置和创建。下面的 buildLoggerName 还决定 loggerTest 调用 getLogger 时应传入的键。
void functional_test() {
    bitlog::GlobalLoggerBuilder::ptr lbp(new bitlog::GlobalLoggerBuilder);
    lbp->buildLoggerName("all_sink_logger");
    // 模板顺序是时间、名称、源码文件及行号、级别、正文、换行。正文中的 %s 先由 Logger 展开，模板里的 %m 再取展开后的正文。
    lbp->buildFormatter("[%d][%c][%f:%l][%p] %m%n");
    // DEBUG 是普通日志里最低的级别，示例会让 DEBUG 到 FATAL 的五种调用都走输出流程。
    lbp->buildLoggerLevel(bitlog::LogLevel::value::DEBUG);
    // 三个 buildSink 依次添加终端、固定文件和滚动文件。滚动阈值为 10 MiB，两个文件路径相对于程序运行目录。
    lbp->buildSink<bitlog::StdoutSink>();
    lbp->buildSink<bitlog::FileSink>("./logs/sync.log");
    lbp->buildSink<bitlog::RollSink>("./logs/roll-", 10 * 1024 * 1024);
    // 异步模式下当前线程负责格式化和入缓冲，工作线程负责逐批写入这三个目标。
    lbp->buildLoggerType(bitlog::Logger::Type::LOGGER_ASYNC);
    // build 创建并登记实例。虽然这里没有保存返回值，注册表仍持有共享指针，随后可以按名称查找。
    lbp->build(); 
    loggerTest("all_sink_logger");
}
// main 只调用一次完整示例。退出时全局管理器释放注册的日志器，异步日志器析构时等待其工作线程。
int main(int argc, char *argv[])
{
    functional_test();
    return 0;
}
