// 【模块导读 · 时间与文件路径工具】
// 这里放置日志代码共用的小工具。LogMsg 构造时调用 date::now；FileSink 和 RollSink 用 file::path 与 create_directory 准备输出目录。
// 从 "out/app.log" 看调用顺序，path 先提取 "out/"，create_directory 再检查并尝试建立该目录，最后由 Sink 自己打开文件。
// 这些方法使用 POSIX 的 stat 和 mkdir，参数是调用者提供的路径。相对路径以运行日志程序时的工作目录为基准。

/*
    通用功能类，与业务无关的功能实现
        1. 获取系统时间
        2. 获取文件大小
        3. 创建目录
        4. 获取文件所在目录
*/
// 日志消息通过这里读取时间，文件输出目标通过这里提取父目录并尝试创建目录。
#ifndef __M_UTIL_H__
#define __M_UTIL_H__

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <cassert>
#include <sys/stat.h>

namespace bitlog{
    namespace util{
        class date {
            public:
                // time(nullptr) 给出 Unix 时间戳，单位为秒。此处直接转成 size_t，后续由 TimeFormatItem 按当地时区显示。
                static size_t now() { return (size_t)time(nullptr); }
        };
        class file {
            public:
                // stat 成功表示路径可访问。它也能查普通文件，因此这里的 exists 无法单独判断一个已存在路径是否为目录。
                static bool exists(const std::string &name) {
                    struct stat st;
                    return stat(name.c_str(), &st) == 0;
                }
                // 从末尾找最后一个路径分隔符。没有分隔符或传入空字符串时，退回 "."，表示当前目录。
                static std::string path(const std::string &name) {
                    if (name.empty()) return ".";
                    size_t pos = name.find_last_of("/\\");
                    if (pos == std::string::npos) return ".";
                    // 截取到 pos + 1 是为了保留最后一个分隔符，例如 "a/b.log" 返回 "a/"，绝对路径 "/app.log" 则返回 "/"。
                    return name.substr(0, pos + 1);
                }
                // 目标为空或已经存在时直接返回。否则按分隔符依次处理父级，让内层 mkdir 执行前已有机会创建外层。
                static void create_directory(const std::string &path) { 
                    if (path.empty()) return ;
                    if (exists(path)) return ;
                    // idx 指向下一段扫描的开始，pos 指向下一个分隔符。每次取 path 开头到 pos 的前缀，代表该层完整路径。
                    size_t pos, idx = 0;
                    // 按路径分隔符推进。前缀为空、等于 . 或 .. 或已存在时跳过，剩余前缀依次交给 mkdir。
                    while(idx < path.size()) {
                        pos = path.find_first_of("/\\", idx);
                        // 后面找不到分隔符时，最后一段就是整条路径尚未处理的结尾。尝试一次 mkdir 后返回。
                        if (pos == std::string::npos) {
                            mkdir(path.c_str(), 0755);
                            return;
                        }
                        // 跳过由开头或相邻分隔符产生的空段。"." 和 ".." 以及已存在的路径前缀也直接推进到下一段。
                        if (pos == idx) {idx = pos + 1; continue;}
                        std::string subdir = path.substr(0, pos);
                        if (subdir == "." || subdir == "..") {idx = pos + 1; continue;}
                        if (exists(subdir)) {idx = pos + 1; continue;}
                        // 按 0755 请求创建目录，实际权限还受 umask 影响。当前函数没有检查 mkdir 返回值，最终是否能打开文件还要看 Sink 的流状态。
                        mkdir(subdir.c_str(), 0755);
                        idx = pos + 1;
                    }
                }
        };
    }
}
#endif