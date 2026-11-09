
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
                // 返回以秒为单位的 Unix 时间戳。当前直接转换为 size_t，没有单独处理 time 的失败返回值。
                static size_t now() { return (size_t)time(nullptr); }
        };
        class file {
            public:
                // 通过 stat 判断路径是否可访问。成功只表示路径存在，文件类型不在这里区分。
                static bool exists(const std::string &name) {
                    struct stat st;
                    return stat(name.c_str(), &st) == 0;
                }
                // 按最后一个正斜杠或反斜杠截出目录前缀并保留分隔符。空路径或仅有文件名时使用当前目录。
                static std::string path(const std::string &name) {
                    if (name.empty()) return ".";
                    size_t pos = name.find_last_of("/\\");
                    if (pos == std::string::npos) return ".";
                    return name.substr(0, pos + 1);
                }
                // 逐段尝试创建缺失的父目录。POSIX mkdir 的权限参数为 0755，实际权限还受进程的 umask 影响。
                static void create_directory(const std::string &path) { 
                    if (path.empty()) return ;
                    if (exists(path)) return ;
                    size_t pos, idx = 0;
                    // 按路径分隔符推进。前缀为空、等于 . 或 .. 或已存在时跳过，剩余前缀依次交给 mkdir。
                    while(idx < path.size()) {
                        pos = path.find_first_of("/\\", idx);
                        // 最后一段没有尾部分隔符时，直接尝试创建完整路径。当前辅助函数不检查 mkdir 的返回值。
                        if (pos == std::string::npos) {
                            mkdir(path.c_str(), 0755);
                            return;
                        }
                        if (pos == idx) {idx = pos + 1; continue;}
                        std::string subdir = path.substr(0, pos);
                        if (subdir == "." || subdir == "..") {idx = pos + 1; continue;}
                        if (exists(subdir)) {idx = pos + 1; continue;}
                        mkdir(subdir.c_str(), 0755);
                        idx = pos + 1;
                    }
                }
        };
    }
}
#endif