// 日志输出目标的接口和实现。上游传入已格式化的字节区间，由目标决定写入终端、普通文件还是滚动文件。
#ifndef __M_SINK_H__
#define __M_SINK_H__
#include "util.hpp"
#include "message.hpp"
#include "formatter.hpp"
#include <memory>
#include <mutex>

namespace bitlog{
// 统一接收指针和字节长度，可处理单条消息或一批连续消息。虚析构函数支持通过基类指针释放派生对象。
class LogSink {
    public:
        using ptr = std::shared_ptr<LogSink>;
        LogSink() {}
        virtual ~LogSink() {}
        virtual void log(const char *data, size_t len) = 0;
};
// 标准输出目标把指定长度的字节写入 std::cout。换行由格式器决定，此处也没有显式刷新。
class StdoutSink : public LogSink {
    public:
        using ptr = std::shared_ptr<StdoutSink>;
        StdoutSink() = default;
        void log(const char *data, size_t len) {
            std::cout.write(data, len);
        }
};
// 文件输出目标在构造时打开路径。追加模式保留原有内容，文件流随对象生命周期关闭。
class FileSink : public LogSink {
    public:
        using ptr = std::shared_ptr<FileSink>;
        FileSink(const std::string &filename):_filename(filename) {
            // 先尝试创建父目录，再按二进制追加模式打开。打开状态由断言检查。
            util::file::create_directory(util::file::path(filename));
            _ofs.open(_filename, std::ios::binary | std::ios::app);
            assert(_ofs.is_open());
        }
        const std::string &file() {return _filename; }
        // 按给定长度写入。流进入错误状态时打印诊断，接口没有向上返回错误码。
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
// 滚动文件目标每次写入前根据当前批次累计量决定是否切换文件。
class RollSink : public LogSink {
    public:
        using ptr = std::shared_ptr<RollSink>;
        RollSink(const std::string &basename, size_t max_fsize):
            _basename(basename), _max_fsize(max_fsize), _cur_fsize(0){
            util::file::create_directory(util::file::path(basename));
        }
        // 整批数据写入同一个文件。达到阈值后的切换发生在下一次调用，因此一次大批次可能让文件超过阈值。
        void log(const char *data, size_t len) {
            initLogFile();
            _ofs.write(data, len);
            if (_ofs.good() == false) {
                std::cout << "日志输出文件失败！\n";
            }
            // 统计提交给当前流的字节数，即使 write 失败也增加计数。计数不包含追加打开时文件里原有的内容。
            _cur_fsize += len;
        }
    private:
        // 第一次调用和累计大小达到阈值时走同一打开流程，切换后从零累计。
        void initLogFile() {
            if (_ofs.is_open() == false || _cur_fsize >= _max_fsize) {
                _ofs.close();
                std::string name = createFilename();
                _ofs.open(name, std::ios::binary | std::ios::app);
                assert(_ofs.is_open());
                _cur_fsize = 0;
                return;
            }
            return;
        } 
        // 使用基本路径与当地时间拼出文件名，精度到秒，各时间分量直接拼接。相同名称会按追加模式打开同一个文件。
        std::string createFilename() {
            time_t t = time(NULL);
            struct tm lt;
            localtime_r(&t, &lt);
            std::stringstream ss;
            ss << _basename;
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

// 模板工厂由调用方选定具体类型，转发构造实参，返回统一的 LogSink 共享指针。
class SinkFactory {
    public:
        template<typename SinkType, typename ...Args>
        static LogSink::ptr create(Args &&...args) {
            return std::make_shared<SinkType>(std::forward<Args>(args)...);
        }
};

}
#endif