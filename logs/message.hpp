// 日志消息的数据模型。Logger 先构造这份记录，再由 Formatter 按配置提取并输出字段。
#ifndef __M_MSG_H__
#define __M_MSG_H__
#include "util.hpp"
#include "level.hpp"
#include <thread>
#include <memory>

namespace bitlog{
// 字段覆盖调用位置、产生时间、线程、日志器名称、正文和级别，格式化时无需再向调用者查询。
struct LogMsg {
    using ptr = std::shared_ptr<LogMsg>;
    size_t _line;//行号
    size_t _ctime;//时间
    std::thread::id _tid;//线程ID
    std::string _name;//日志器名称
    std::string _file;//文件名
    std::string _payload;//日志消息
    LogLevel::value _level;//日志等级
    // 构造时取当前时间和线程 ID，记录的是调用线程的上下文。字符串字段持有自己的数据，正文通过移动接收。
    LogMsg(std::string &name, std::string file, size_t line, std::string &&payload, 
        LogLevel::value level): _name(name), _file(file), _payload(std::move(payload)), _level(level),
        _line(line), _ctime(util::date::now()), _tid(std::this_thread::get_id()) {
            //std::cout << "构造msg\n";
        }
    //~LogMsg() { /*std::cout << "析构msg\n";*/}
};
}

#endif