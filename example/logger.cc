#include "../logs/bitlog.h"


void loggerTest(const std::string &logger_name) {
    bitlog::Logger::ptr lp = bitlog::getLogger(logger_name);
    assert(lp.get());
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
    
    std::string log_msg = "hello bitejiuyeke-";
    size_t fsize = 0;
    size_t count = 0;
    while(count < 1000000) {
        std::string msg = log_msg + std::to_string(count++);
        lp->error("%s", msg.c_str());
    }
}
void functional_test() {
    bitlog::GlobalLoggerBuilder::ptr lbp(new bitlog::GlobalLoggerBuilder);
    lbp->buildLoggerName("all_sink_logger");
    lbp->buildFormatter("[%d][%c][%f:%l][%p] %m%n");
    lbp->buildLoggerLevel(bitlog::LogLevel::value::DEBUG);
    lbp->buildSink<bitlog::StdoutSink>();
    lbp->buildSink<bitlog::FileSink>("./logs/sync.log");
    lbp->buildSink<bitlog::RollSink>("./logs/roll-", 10 * 1024 * 1024);
    lbp->buildLoggerType(bitlog::Logger::Type::LOGGER_ASYNC);
    lbp->build(); 
    loggerTest("all_sink_logger");
}
int main(int argc, char *argv[])
{
    functional_test();
    return 0;
}
