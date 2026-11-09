// 日志库的调用入口。调用方可以按名称取得已注册的日志器，也可以直接使用默认的 root 日志器。
#ifndef __M_BIT_H__
#define __M_BIT_H__
#include "logger.hpp"

namespace bitlog {
    // 按名称查找返回共享指针。名称未注册时返回空指针，调用方应先确认查找成功。
    Logger::ptr getLogger(const std::string &name) {
        return loggerManager::getInstance().getLogger(name);
    }
    // root 由日志器管理器在首次使用时创建，适合不需要自行配置日志器的调用点。
    Logger::ptr rootLogger() {
        return loggerManager::getInstance().rootLogger();
    }

    // 这些宏在调用点展开，把源文件和行号交给 Logger。GNU 的逗号吞并扩展允许省略格式串之后的实参。
    #define debug(fmt, ...) debug(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define info(fmt, ...) info(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define warn(fmt, ...) warn(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define error(fmt, ...) error(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define fatal(fmt, ...) fatal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
    
    // LOG_* 由调用方指定日志器，最终仍通过上方同名宏补齐文件和行号。传入的日志器指针须有效。
    #define LOG_DEBUG(logger, fmt, ...) (logger)->debug(fmt, ##__VA_ARGS__)
    #define LOG_INFO(logger, fmt, ...) (logger)->info(fmt, ##__VA_ARGS__)
    #define LOG_WARN(logger, fmt, ...) (logger)->warn(fmt, ##__VA_ARGS__)
    #define LOG_ERROR(logger, fmt, ...) (logger)->error(fmt, ##__VA_ARGS__)
    #define LOG_FATAL(logger, fmt, ...) (logger)->fatal(fmt, ##__VA_ARGS__)

    // LOGD 至 LOGF 固定使用 root 日志器，级别从 DEBUG 到 FATAL，参数采用 printf 风格。
    #define LOGD(fmt, ...) LOG_DEBUG(bitlog::rootLogger(), fmt, ##__VA_ARGS__)
    #define LOGI(fmt, ...) LOG_INFO(bitlog::rootLogger(), fmt, ##__VA_ARGS__)
    #define LOGW(fmt, ...) LOG_WARN(bitlog::rootLogger(), fmt, ##__VA_ARGS__)
    #define LOGE(fmt, ...) LOG_ERROR(bitlog::rootLogger(), fmt, ##__VA_ARGS__)
    #define LOGF(fmt, ...) LOG_FATAL(bitlog::rootLogger(), fmt, ##__VA_ARGS__)
}

#endif