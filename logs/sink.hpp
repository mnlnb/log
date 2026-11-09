// 【模块导读 · 日志文本最终写到哪里】
// Sink 是输出末端，输入为一段已排版的字节和长度。同步日志器通常交来一行，异步日志器可能一次交来多行。
// StdoutSink 写终端，FileSink 追加到一个固定文件，RollSink 按累计写入量在下一批到来时尝试切换到带时间戳的文件。
// 一只日志器可以同时配置多个目标。例如示例同时向终端、固定日志和滚动日志各送一份文本，顺序由 buildSink 的调用顺序决定。
// 读本文件时先看 LogSink::log 的参数，再比较三个目标处理相同字节的方式，最后看 SinkFactory 如何为 Builder 创建目标。
#ifndef __M_SINK_H__
#define __M_SINK_H__
#include "util.hpp"
#include "message.hpp"
#include "formatter.hpp"
#include <memory>
#include <mutex>

namespace bitlog{
// 接口用 const char* 加长度表示有效区间，因此可以写包含空字节的输入，也可以写没有结尾空字符的 Buffer。
class LogSink {
    public:
        using ptr = std::shared_ptr<LogSink>;
        LogSink() {}
        // 虚析构使通过 LogSink 基类指针销毁时仍调用实际目标类型的析构，文件流成员也会随之清理。
        virtual ~LogSink() {}
        virtual void log(const char *data, size_t len) = 0;
};
// 标准输出目标没有独立文件或容量状态，它直接向进程的 std::cout 写指定数量的字节。
class StdoutSink : public LogSink {
    public:
        using ptr = std::shared_ptr<StdoutSink>;
        StdoutSink() = default;
        void log(const char *data, size_t len) {
            // write 的长度来自上游。该调用不会添加换行或显式刷新，常见的一行一条由模板中的 %n 实现。
            std::cout.write(data, len);
        }
};
// 固定文件目标在构造时选定路径，之后始终向该路径对应的流追加。示例中的 "./logs/sync.log" 相对于程序工作目录。
class FileSink : public LogSink {
    public:
        using ptr = std::shared_ptr<FileSink>;
        // 先用 util::file::path 提取父目录，再尝试逐段创建目录。比如 "./logs/sync.log" 得到 "./logs/"。
        FileSink(const std::string &filename):_filename(filename) {
            // 先尝试创建父目录，再按二进制追加模式打开。打开状态由断言检查。
            util::file::create_directory(util::file::path(filename));
            // binary 保持输入的字节形式，app 让每次写入追加到已有内容之后。打开失败由下一行断言检查。
            _ofs.open(_filename, std::ios::binary | std::ios::app);
            assert(_ofs.is_open());
        }
        // file 返回目标路径的引用。只要 FileSink 仍存活，就可以用它确认配置时传入的文件名。
        const std::string &file() {return _filename; }
        // log 把本次收到的字节交给文件流。检查 good() 是检查流状态，失败时当前接口只打印诊断。
        void log(const char *data, size_t len) {
            _ofs.write((const char*)data, len);
            if (_ofs.good() == false) {
                std::cout << "日志输出文件失败！\n";
            }
        }
    private:
        std::string _filename;
        std::ofstream _ofs;
};
// 滚动目标关心当前流累计接收了多少字节。每次调用先考虑切换，再把整批数据写进当前流。
class RollSink : public LogSink {
    public:
        using ptr = std::shared_ptr<RollSink>;
        // basename 是拼接文件名用的前缀，例如 "./logs/roll-"，max_fsize 的单位是字节。构造时先准备目录，直到首批日志到来才打开文件。
        RollSink(const std::string &basename, size_t max_fsize):
            _basename(basename), _max_fsize(max_fsize), _cur_fsize(0){
            util::file::create_directory(util::file::path(basename));
        }
        // 先调用 initLogFile 检查流是否需要打开或切换，再进行本批写入。批次可以跨过大小阈值，下次写入才检查是否滚动。
        void log(const char *data, size_t len) {
            initLogFile();
            _ofs.write(data, len);
            // 写失败时报告诊断，当前仍用 len 更新计数。_cur_fsize 表示本实例向当前流提交的字节总数。
            if (_ofs.good() == false) {
                std::cout << "日志输出文件失败！\n";
            }
            // 计数从每次打开时的 0 开始，追加模式下文件已存在的内容不会计入。阈值因此与实际文件大小可能不同。
            _cur_fsize += len;
        }
    private:
        // 当前没有流，或累计量已达到 _max_fsize，就关闭旧流、生成名称、追加打开，再把计数归零。
        void initLogFile() {
            if (_ofs.is_open() == false || _cur_fsize >= _max_fsize) {
                _ofs.close();
                // 时间戳在切换时生成。同一秒内得到同名时，追加模式会继续写入该文件。
                std::string name = createFilename();
                _ofs.open(name, std::ios::binary | std::ios::app);
                assert(_ofs.is_open());
                _cur_fsize = 0;
                return;
            }
            return;
        } 
        // createFilename 取当地时间，把年、月、日、时、分、秒依次接在前缀后，末尾追加 ".log"。
        std::string createFilename() {
            time_t t = time(NULL);
            struct tm lt;
            localtime_r(&t, &lt);
            std::stringstream ss;
            ss << _basename;
            // tm_year 自 1900 起计，tm_mon 从 0 起计，所以这里分别加 1900 和 1。各时间字段直接拼接，没有补零或分隔符。
            ss << lt.tm_year + 1900;
            ss << lt.tm_mon + 1;
            ss << lt.tm_mday;
            ss << lt.tm_hour;
            ss << lt.tm_min;
            ss << lt.tm_sec;
            ss << ".log";
            return ss.str();
        }
    private:
        std::string _basename;
        std::ofstream _ofs;
        size_t _max_fsize;
        size_t _cur_fsize;
};

// Builder 调用这个模板工厂集中创建目标。调用方选择具体类型，例如 FileSink，统一接收 LogSink 共享指针。
class SinkFactory {
    public:
        template<typename SinkType, typename ...Args>
        // forward 把各实参以原来的值类别交给 make_shared。不同目标所需的路径和阈值参数都走同一个接口。
        static LogSink::ptr create(Args &&...args) {
            return std::make_shared<SinkType>(std::forward<Args>(args)...);
        }
};

}
#endif