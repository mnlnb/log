// 【模块导读 · 日志库入口】
// 业务代码通常只包含这个头文件。它把按名称查找日志器、使用默认日志器和记录源码位置这三件事集中起来。
// 想看默认日志，顺着 LOGI → rootLogger → Logger::info；想看具名日志，顺着 getLogger → LOG_INFO → Logger::info。
// 例如 LOG_INFO(lp, "%s", "启动") 最终会在 lp 上调用 info，并在格式串前补入调用处的文件名和行号。
// 这里仅转发调用。是否过滤、怎样生成一行文字、写到哪里，分别看 logger.hpp、formatter.hpp 和 sink.hpp。
#ifndef __M_BIT_H__
#define __M_BIT_H__
#include "logger.hpp"

namespace bitlog {
    // 向全局 loggerManager 查询。名称来自 GlobalLoggerBuilder 注册，查无此名返回空共享指针，调用者须确认后再使用。
    Logger::ptr getLogger(const std::string &name) {
        return loggerManager::getInstance().getLogger(name);
    }
    // root 是管理器首次初始化时建好的同步日志器。未自定义时使用默认级别、默认行格式和标准输出。
    Logger::ptr rootLogger() {
        return loggerManager::getInstance().rootLogger();
    }

    // 第一组是函数式宏，匹配 debug(...) 等记号，展开时补上 __FILE__ 和 __LINE__。
    // 宏替换期间不会反复展开自身，因此展开结果中的 debug(...) 最终仍是同名成员函数调用。
    // ##__VA_ARGS__ 使用 GNU 扩展，在 LOGI("启动") 这种只有格式串的调用中吞掉多余逗号。
    #define debug(fmt, ...) debug(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define info(fmt, ...) info(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define warn(fmt, ...) warn(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define error(fmt, ...) error(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define fatal(fmt, ...) fatal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
    
    // 第二组接收已选好的日志器。例如 LOG_WARN(lp, "次数=%d", n) 先展开成 (lp)->warn(...)，再由上组宏补调用位置。
    #define LOG_DEBUG(logger, fmt, ...) (logger)->debug(fmt, ##__VA_ARGS__)
    #define LOG_INFO(logger, fmt, ...) (logger)->info(fmt, ##__VA_ARGS__)
    #define LOG_WARN(logger, fmt, ...) (logger)->warn(fmt, ##__VA_ARGS__)
    #define LOG_ERROR(logger, fmt, ...) (logger)->error(fmt, ##__VA_ARGS__)
    #define LOG_FATAL(logger, fmt, ...) (logger)->fatal(fmt, ##__VA_ARGS__)

    // 第三组是默认日志器的简写。最后一个字母对应 DEBUG、INFO、WARN、ERROR、FATAL 这五档级别。
    #define LOGD(fmt, ...) LOG_DEBUG(bitlog::rootLogger(), fmt, ##__VA_ARGS__)
    #define LOGI(fmt, ...) LOG_INFO(bitlog::rootLogger(), fmt, ##__VA_ARGS__)
    #define LOGW(fmt, ...) LOG_WARN(bitlog::rootLogger(), fmt, ##__VA_ARGS__)
    #define LOGE(fmt, ...) LOG_ERROR(bitlog::rootLogger(), fmt, ##__VA_ARGS__)
    #define LOGF(fmt, ...) LOG_FATAL(bitlog::rootLogger(), fmt, ##__VA_ARGS__)
}

#endif