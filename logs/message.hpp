// 【模块导读 · 一条日志的数据】
// LogMsg 是 Logger 交给 Formatter 的记录。它把 printf 风格的正文和当次调用的元数据放在一起。
// 例如业务调用 LOGI("处理了 %d 条", 3)，正文先由 Logger 生成；时间、线程、位置、级别和日志器名保存在其他字段。
// Formatter 决定哪些字段进入最终文本。异步路径送入后台的是已经格式化的字节，LogMsg 本身在调用线程内用完。
#ifndef __M_MSG_H__
#define __M_MSG_H__
#include "util.hpp"
#include "level.hpp"
#include <thread>
#include <memory>

namespace bitlog{
// 阅读时把字段分成三组：_payload 是正文，_ctime 和 _tid 是运行上下文，_line、_file、_name、_level 用于定位和分类。
struct LogMsg {
    using ptr = std::shared_ptr<LogMsg>;
    // _line 与 _file 通常由 bitlog.h 的调用宏提供。文件名可能包含编译时传入的路径。
    size_t _line;//行号
    // _ctime 保存秒级时间戳，%d 输出时才转换为当地时间。_tid 保存调用线程的 ID，%t 按流输出该 ID。
    size_t _ctime;//时间
    std::thread::id _tid;//线程ID
    // _name 是日志器的配置名，与日志写入的目标文件名相互独立。_level 由调用的 debug/info 等入口决定。
    std::string _name;//日志器名称
    std::string _file;//文件名
    std::string _payload;//日志消息
    LogLevel::value _level;//日志等级
    // 构造时复制日志器名称和文件名，移动接收已经格式化的正文，并立即取当前时间与线程 ID。
    // 这些字符串和元数据属于这条记录，可以由格式器独立读取，不必回头访问调用方的参数。
    LogMsg(std::string &name, std::string file, size_t line, std::string &&payload, 
        LogLevel::value level): _name(name), _file(file), _payload(std::move(payload)), _level(level),
        _line(line), _ctime(util::date::now()), _tid(std::this_thread::get_id()) {
            //std::cout << "构造msg\n";
        }
    //~LogMsg() { /*std::cout << "析构msg\n";*/}
};
}

#endif