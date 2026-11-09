// 日志器的主体。调用路径依次经过级别过滤、正文格式化、消息元数据拼装和整行格式化，最后由同步或异步日志器交给输出目标。
#ifndef __M_LOG_H__
#define __M_LOG_H__
#include "util.hpp"
#include "level.hpp"
#include "message.hpp"
#include "formatter.hpp"
#include "sink.hpp"
#include "looper.hpp"
#include <vector>
#include <list>
#include <atomic>
#include <unordered_map>
#include <cstdarg>
#include <type_traits>


namespace bitlog{
class SyncLogger;
class AsyncLogger;
// 基类负责两种日志器共有的状态和处理流程，派生类通过 logIt 决定格式化结果如何送出。
class Logger {
    public:
        enum class Type {
            LOGGER_SYNC = 0,
            LOGGER_ASYNC
        };
        using ptr = std::shared_ptr<Logger>; 
        // 构造时保存名称和阈值，并复制各输出目标的共享指针。每个输出目标的生命周期随持有它的共享指针延长。
        Logger(const std::string &name, 
            Formatter::ptr formatter,
            std::vector<LogSink::ptr> &sinks, 
            LogLevel::value level = LogLevel::value::DEBUG): 
            _name(name),  _level(level), _formatter(formatter),
            _sinks(sinks.begin(), sinks.end()){
        }
        std::string loggerName() { return _name; }
        LogLevel::value loggerLevel() { return _level; }
        // 以下五个入口结构相同。先比较级别，符合阈值时才初始化 va_list，交给 log 处理后再调用 va_end。
        void debug(const char *file, size_t line, const char *fmt, ...) {
            if (shouldLog(LogLevel::value::DEBUG) == false) {
                return ;
            }
            va_list al;
            va_start(al, fmt);
            log(LogLevel::value::DEBUG, file, line, fmt, al);
            va_end(al);
        }
        void info(const char *file, size_t line, const char *fmt, ...) {
            if (shouldLog(LogLevel::value::INFO) == false) return ;
            va_list al;
            va_start(al, fmt);
            log(LogLevel::value::INFO, file, line, fmt, al);
            va_end(al);
        }
        void warn(const char *file, size_t line, const char *fmt, ...) {
            if (shouldLog(LogLevel::value::WARN) == false) return ;
            va_list al;
            va_start(al, fmt);
            log(LogLevel::value::WARN, file, line, fmt, al);
            va_end(al);
        }
        void error(const char *file, size_t line, const char *fmt, ...) {
            if (shouldLog(LogLevel::value::ERROR) == false) return ;
            va_list al;
            va_start(al, fmt);
            log(LogLevel::value::ERROR, file, line, fmt, al);
            va_end(al);
        }
        void fatal(const char *file, size_t line, const char *fmt, ...) {
            if (shouldLog(LogLevel::value::FATAL) == false) return ;
            va_list al;
            va_start(al, fmt);
            log(LogLevel::value::FATAL, file, line, fmt, al);
            va_end(al);
        }
    public:
        // 建造者逐步收集名称、级别、运行方式、格式器和输出目标。默认阈值为 DEBUG，默认运行方式为同步。
        class Builder {
            public:
                using ptr = std::shared_ptr<Builder>;
                Builder():_level(LogLevel::value::DEBUG), 
                    _logger_type(Logger::Type::LOGGER_SYNC) {}
                void buildLoggerName(const std::string &name) { _logger_name = name; }
                void buildLoggerLevel(LogLevel::value level) { _level = level; }
                void buildLoggerType(Logger::Type type) { _logger_type = type; }
                // 格式器可由模式字符串创建，也可由调用方直接传入一个共享实例。
                void buildFormatter(const std::string pattern) { _formatter = std::make_shared<Formatter>(pattern); }
                void buildFormatter(const Formatter::ptr &formatter) { _formatter = formatter; }
                // 模板参数决定输出目标的具体类型。完美转发保留构造参数的值类别，允许多次添加不同输出目标。
                template<typename SinkType, typename ...Args>
                void buildSink(Args &&...args) { 
                    auto sink = SinkFactory::create<SinkType>(std::forward<Args>(args)...);
                    _sinks.push_back(sink); 
                }
                virtual Logger::ptr build() = 0;
            protected:
                Logger::Type _logger_type;
                std::string _logger_name;
                LogLevel::value _level;
                Formatter::ptr _formatter;
                std::vector<LogSink::ptr> _sinks;
        };
    protected:
        // 用原子级别字段作阈值比较，消息级别等于阈值时也会通过。OFF 高于这五种入口传入的级别。
        bool shouldLog(LogLevel::value level) { return level >= _level; }
        // vasprintf 解释正文格式并分配字符缓冲。成功时按返回长度复制到字符串并释放缓冲，失败时使用固定诊断文本。
        void log(LogLevel::value level, const char *file, size_t line, const char *fmt, va_list al) {
            char *buf;
            std::string msg;
            int len = vasprintf(&buf, fmt, al);
            if (len < 0) {
                msg = "格式化日志消息失败！！";
            }else {
                msg.assign(buf, len);
                free(buf);
            }
            //LogMsg(name, file, line, payload, level)
            // LogMsg 捕获调用位置、当前时间和线程。Formatter 生成最终文本，此时仍处于调用线程。
            LogMsg lm(_name, file, line, std::move(msg), level);
            std::stringstream ss;
            _formatter->format(ss, lm);
            logIt(std::move(ss.str()));
        }
        // 派生实现收到的是完整日志行。异步实现将其转成缓冲区里的字节，输出目标无需再处理 LogMsg。
        virtual void logIt(const std::string &msg) = 0;
    protected:
        std::mutex _mutex;
        std::string _name;
        Formatter::ptr _formatter;
        std::atomic<LogLevel::value> _level;
        std::vector<LogSink::ptr> _sinks;
};

// 同步日志器在调用线程遍历输出目标，输出耗时计入本次日志调用。
class SyncLogger : public Logger {
    public:
        using ptr = std::shared_ptr<SyncLogger>;
        SyncLogger(const std::string &name, 
            Formatter::ptr formatter,
            std::vector<LogSink::ptr> &sinks, 
            LogLevel::value level = LogLevel::value::DEBUG): 
            Logger(name, formatter, sinks, level){ 
            std::cout << LogLevel::toString(level) << " 同步日志器: " << name << "创建成功...\n";
        }
    private:
        virtual void logIt(const std::string &msg) {
            // 在持有本日志器互斥锁期间写入全部输出目标，防止同一日志器的多个线程交错输出。
            std::unique_lock<std::mutex> lock(_mutex);
            if (_sinks.empty()) { return ; }
            for (auto &it : _sinks) {
                it->log(msg.c_str(), msg.size());
            }
        }
};

// 异步日志器在调用线程完成格式化，后台线程负责输出。回调绑定当前 AsyncLogger 实例和一个批次参数。
class AsyncLogger : public Logger {
    public:
        using ptr = std::shared_ptr<AsyncLogger>;
        AsyncLogger(const std::string &name, 
            Formatter::ptr formatter, 
            std::vector<LogSink::ptr> &sinks, 
            LogLevel::value level = LogLevel::value::DEBUG): 
            Logger(name, formatter, sinks, level),
            _looper(std::make_shared<AsyncLooper>(std::bind(&AsyncLogger::backendLogIt, this, std::placeholders::_1))) {
            std::cout << LogLevel::toString(level) << "异步日志器: " << name << "创建成功...\n";
        }
    protected:
        // 生产者将文本复制进 looper 的写缓冲。写缓冲暂时无法容纳整条消息时，调用线程可能等待。
        virtual void logIt(const std::string &msg) {
            _looper->push(msg);
        }
        // 回调把同一批连续字节写入每个输出目标。它使用缓冲区提供的长度，不依赖空字符，也无需逐条拆分。
        void backendLogIt(Buffer &msg) {
            if (_sinks.empty()) { return; }
            for (auto &it : _sinks) {
                it->log(msg.begin(), msg.readAbleSize());
            }
        }
    protected:
        // _looper 是派生类持有的成员。它析构时停止并等待后台线程，此时基类保存的输出目标仍在。
        AsyncLooper::ptr _looper;
};

// 本地建造者只创建日志器，调用方直接持有返回值。日志器不会因此进入全局名称表。
class LocalLoggerBuilder: public Logger::Builder {
    public:
        virtual Logger::ptr build() {
            // 名称必填。未配置格式器或输出目标时，分别补上默认格式器和标准输出目标。
            if (_logger_name.empty()) {
                std::cout << "日志器名称不能为空！！";
                abort();
            }
            if (_formatter.get() == nullptr) {
                std::cout << "当前日志器：" << _logger_name << " 未检测到日志格式，默认设置为[ %d{%H:%M:%S}%T%t%T[%p]%T[%c]%T%f:%l%T%m%n ]!\n";
                _formatter = std::make_shared<Formatter>();
            }
            if (_sinks.empty()) {
                std::cout << "当前日志器：" << _logger_name << " 未检测到落地方向，默认设置为标准输出!\n";
                _sinks.push_back(std::make_shared<StdoutSink>());
            }
            // 根据配置实例化同步或异步派生类，再以 Logger 共享指针返回。
            Logger::ptr lp;
            if (_logger_type == Logger::Type::LOGGER_ASYNC) {
                lp = std::make_shared<AsyncLogger>(_logger_name, _formatter, _sinks, _level);
            }else {
                lp = std::make_shared<SyncLogger>(_logger_name, _formatter, _sinks, _level);
            }
            return lp;
        }
};


// 全局管理器保存 root 和名称到日志器的映射，互斥锁保护对表及 root 指针的访问。
class loggerManager{
    private:
        std::mutex _mutex;
        Logger::ptr _root_logger;
        std::unordered_map<std::string, Logger::ptr> _loggers;
    private:
        // 首次创建管理器时通过本地建造者构造同步 root，然后将同一指针登记到名称表。
        loggerManager(){ 
            std::unique_ptr<LocalLoggerBuilder> slb(new LocalLoggerBuilder());
            slb->buildLoggerName("root");
            slb->buildLoggerType(Logger::Type::LOGGER_SYNC);
            _root_logger = slb->build();
            _loggers.insert(std::make_pair("root", _root_logger));
        }
        loggerManager(const loggerManager&) = delete;
        loggerManager &operator=(const loggerManager&) = delete;
    public:
        // C++11 保证函数局部静态对象的首次初始化只发生一次。后续调用返回同一个管理器引用。
        static loggerManager& getInstance() {
            static loggerManager lm;
            return lm;
        }
        // 名称存在性检查只覆盖本次加锁范围。它与之后的 addLogger 是两个独立操作。
        bool hasLogger(const std::string &name)  {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _loggers.find(name);
            if (it == _loggers.end()) {
                return false;
            }
            return true;
        }
        // 这里使用 unordered_map::insert。名称已经存在时保留原条目，不会覆盖。
        void addLogger(const std::string &name, const Logger::ptr logger) {
            std::unique_lock<std::mutex> lock(_mutex);
            _loggers.insert(std::make_pair(name, logger));
        }
        // 查找时在锁内复制共享指针，因此返回值在解锁后仍持有日志器。缺失名称返回空指针。
        Logger::ptr getLogger(const std::string &name) {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _loggers.find(name);
            if (it == _loggers.end()) {
                return Logger::ptr();
            }
            return it->second;
        }
        Logger::ptr rootLogger() {
            std::unique_lock<std::mutex> lock(_mutex);
            return _root_logger;
        }
};

// 全局建造者创建实例后按名称注册，后续可以通过 getLogger 获取。
class GlobalLoggerBuilder: public Logger::Builder {
    public:
        virtual Logger::ptr build() {
            if (_logger_name.empty()) {
                std::cout << "日志器名称不能为空！！";
                abort();
            }
            // 重名预检依赖断言。禁用断言时由名称表的 insert 行为决定是否登记。
            assert(loggerManager::getInstance().hasLogger(_logger_name) == false);
            if (_formatter.get() == nullptr) {
                std::cout << "当前日志器：" << _logger_name << " 未检测到日志格式，默认设置为[ %d{%H:%M:%S}%T%t%T[%p]%T[%c]%T%f:%l%T%m%n ]!\n";
                _formatter = std::make_shared<Formatter>();
            }
            if (_sinks.empty()) {
                std::cout << "当前日志器：" << _logger_name << " 未检测到落地方向，默认设置为标准输出!\n";
                _sinks.push_back(std::make_shared<StdoutSink>());
            }
            Logger::ptr lp;
            if (_logger_type == Logger::Type::LOGGER_ASYNC) {
                lp = std::make_shared<AsyncLogger>(_logger_name, _formatter, _sinks, _level);
            }else {
                lp = std::make_shared<SyncLogger>(_logger_name, _formatter, _sinks, _level);
            }
            // 登记使用和本次 build 返回值相同的共享指针，成功后管理器和调用方共同持有实例。
            loggerManager::getInstance().addLogger(_logger_name, lp);
            return lp;
        }
};

}

#endif