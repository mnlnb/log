// 演示具名日志器的构建、级别调用以及一条消息输出到多个目标的方式。
#include "../logs/bitlog.h"


// 通过注册名取得日志器。断言要求该名称已被 functional_test 登记。
void loggerTest(const std::string &logger_name) {
    bitlog::Logger::ptr lp = bitlog::getLogger(logger_name);
    assert(lp.get());
    // 分隔线由 root 日志器输出。中间依次演示具名日志器的成员入口及显式指定日志器的 LOG_* 宏。
    LOGF("------------example--------------------");
    lp->debug("%s", "logger->debug");
    lp->info("%s", "logger->info");
    lp->warn("%s", "logger->warn");
    lp->error("%s", "logger->error");
    lp->fatal("%s", "logger->fatal");
    LOG_DEBUG(lp, "%s", "LOG_DEBUG");
    LOG_INFO(lp, "%s", "LOG_INFO");
    LOG_WARN(lp, "%s", "LOG_WARN");
    LOG_ERROR(lp, "%s", "LOG_ERROR");
    LOG_FATAL(lp, "%s", "LOG_FATAL");
    LOGF("---------------------------------------");
    
    // 随后产生一百万条带递增编号的 ERROR 消息，用于观察文件追加和按大小滚动的效果。
    std::string log_msg = "hello bitejiuyeke-";
    size_t fsize = 0;
    size_t count = 0;
    while(count < 1000000) {
        std::string msg = log_msg + std::to_string(count++);
        lp->error("%s", msg.c_str());
    }
}
// 把名称、格式、阈值和输出目标集中交给全局建造者。build 创建日志器并登记该名称。
void functional_test() {
    bitlog::GlobalLoggerBuilder::ptr lbp(new bitlog::GlobalLoggerBuilder);
    lbp->buildLoggerName("all_sink_logger");
    // 示例行含时间、日志器、调用位置、级别和正文。%n 让每条消息占一行。
    lbp->buildFormatter("[%d][%c][%f:%l][%p] %m%n");
    lbp->buildLoggerLevel(bitlog::LogLevel::value::DEBUG);
    // 同一异步日志器同时配置标准输出、普通追加文件和 10 MiB 滚动阈值。相对文件路径以程序工作目录为基准。
    lbp->buildSink<bitlog::StdoutSink>();
    lbp->buildSink<bitlog::FileSink>("./logs/sync.log");
    lbp->buildSink<bitlog::RollSink>("./logs/roll-", 10 * 1024 * 1024);
    lbp->buildLoggerType(bitlog::Logger::Type::LOGGER_ASYNC);
    lbp->build(); 
    loggerTest("all_sink_logger");
}
// 入口运行完整示例，日志器注册表持有实例，退出时随静态管理器析构。
int main(int argc, char *argv[])
{
    functional_test();
    return 0;
}
