// 【模块导读 · 从一次业务调用到写出日志】
// 本文件把日志器的配置、查找、级别过滤、正文格式化和输出分发串起来。建议先读 Logger::log，再对照两个 logIt，最后读建造者和管理器。
// 配置阶段由 Builder 收集名称、最低级别、Formatter、一个或多个 LogSink 以及同步或异步类型。GlobalLoggerBuilder 会把实例登记到全局表。
// 记录阶段由 debug/info/warn/error/fatal 接收调用位置和 printf 风格参数。符合级别时生成 LogMsg，再请 Formatter 组合成完整的一行文本。
// 分发阶段由 SyncLogger 在当前线程逐个写 Sink，或由 AsyncLogger 把文本交给 AsyncLooper，工作线程接管一批文本再写 Sink。
// 以 LOG_INFO(lp, "次数=%d", 3) 为例，调用点信息由 bitlog.h 补齐；这里生成正文“次数=3”，再加上配置所需的时间、级别等字段。
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
// Logger 是共同的入口与状态。调用方经由 Logger 指针使用相同的 API，输出方式由实际实例的虚函数 logIt 选择。
class Logger {
    public:
        // Type 用在建造阶段，决定创建同步还是异步派生实例。创建后具体类型负责分发。
        enum class Type {
            LOGGER_SYNC = 0,
            LOGGER_ASYNC
        };
        using ptr = std::shared_ptr<Logger>; 
        // 构造函数保存配置。_sinks 按建造者添加的顺序复制共享指针，Formatter 和 Sink 因共享所有权可在 Builder 析构后继续使用。
        Logger(const std::string &name, 
            Formatter::ptr formatter,
            std::vector<LogSink::ptr> &sinks, 
            LogLevel::value level = LogLevel::value::DEBUG): 
            _name(name),  _level(level), _formatter(formatter),
            _sinks(sinks.begin(), sinks.end()){
        }
        // 这两个访问器给调用方查看名称和当前阈值。阈值存储在原子字段里，读取会获得一次该字段的值。
        std::string loggerName() { return _name; }
        LogLevel::value loggerLevel() { return _level; }
        // 五个入口分工相同，区别是传给 shouldLog 和 log 的级别常量。低于阈值时直接返回，从而省去分配、格式化和输出。
        void debug(const char *file, size_t line, const char *fmt, ...) {
            if (shouldLog(LogLevel::value::DEBUG) == false) {
                return ;
            }
            // va_start 从最后一个具名参数 fmt 开始定位后续参数，log 接收参数列表，入口在返回前用 va_end 结束访问。
            // 调用者须让格式串与实参的类型和数量匹配，例如 %d 对应 int、%s 对应有效的 C 字符串。
            va_list al;
            va_start(al, fmt);
            log(LogLevel::value::DEBUG, file, line, fmt, al);
            va_end(al);
        }
        // INFO 表示普通运行进度。阈值为 INFO 或 DEBUG 时，这个入口会把记录交给共用的 log。
        void info(const char *file, size_t line, const char *fmt, ...) {
            if (shouldLog(LogLevel::value::INFO) == false) return ;
            va_list al;
            va_start(al, fmt);
            log(LogLevel::value::INFO, file, line, fmt, al);
            va_end(al);
        }
        // WARN 表示值得关注的状态。过滤通过后沿用同样的可变参数处理流程。
        void warn(const char *file, size_t line, const char *fmt, ...) {
            if (shouldLog(LogLevel::value::WARN) == false) return ;
            va_list al;
            va_start(al, fmt);
            log(LogLevel::value::WARN, file, line, fmt, al);
            va_end(al);
        }
        // ERROR 表示一次操作发生错误。级别常量也会进入 LogMsg，供格式中的 %p 使用。
        void error(const char *file, size_t line, const char *fmt, ...) {
            if (shouldLog(LogLevel::value::ERROR) == false) return ;
            va_list al;
            va_start(al, fmt);
            log(LogLevel::value::ERROR, file, line, fmt, al);
            va_end(al);
        }
        // FATAL 是五个入口中的最高级别。函数体的工作仍是记录日志，调用它不会自动终止程序。
        void fatal(const char *file, size_t line, const char *fmt, ...) {
            if (shouldLog(LogLevel::value::FATAL) == false) return ;
            va_list al;
            va_start(al, fmt);
            log(LogLevel::value::FATAL, file, line, fmt, al);
            va_end(al);
        }
    public:
        // Builder 先累积配置，再由派生建造者实现 build。适合一次配置多个输出目标或复用同一个 Formatter。
        class Builder {
            public:
                using ptr = std::shared_ptr<Builder>;
                // 初始阈值为 DEBUG，默认同步。名称、Formatter 和目标列表留给调用方设置或由 build 补齐。
                Builder():_level(LogLevel::value::DEBUG), 
                    _logger_type(Logger::Type::LOGGER_SYNC) {}
                // 前三个配置方法分别设置管理器查找键、记录阈值和同步方式。调用 build 前最后一次设置的值生效。
                void buildLoggerName(const std::string &name) { _logger_name = name; }
                void buildLoggerLevel(LogLevel::value level) { _level = level; }
                void buildLoggerType(Logger::Type type) { _logger_type = type; }
                // 传入模式串时新建 Formatter，适合给某个日志器定义专用格式；传入共享指针时可沿用已有 Formatter。
                void buildFormatter(const std::string pattern) { _formatter = std::make_shared<Formatter>(pattern); }
                void buildFormatter(const Formatter::ptr &formatter) { _formatter = formatter; }
                // buildSink 的类型参数决定创建哪个 Sink。后面的参数原样转给构造器，比如 FileSink 接收路径，RollSink 还接收大小上限。
                template<typename SinkType, typename ...Args>
                void buildSink(Args &&...args) { 
                    auto sink = SinkFactory::create<SinkType>(std::forward<Args>(args)...);
                    // 每次添加目标都会追加一个共享指针。输出时按这个顺序把同一行或同一批文本发给全部目标。
                    _sinks.push_back(sink); 
                }
                // build 是建造阶段的最终步骤。基类保存配置，LocalLoggerBuilder 与 GlobalLoggerBuilder 决定是否注册。
                virtual Logger::ptr build() = 0;
            protected:
                Logger::Type _logger_type;
                std::string _logger_name;
                LogLevel::value _level;
                Formatter::ptr _formatter;
                std::vector<LogSink::ptr> _sinks;
        };
    protected:
        // 过滤条件是消息级别大于等于阈值。同档消息保留；例如 WARN 阈值保留 WARN、ERROR、FATAL。
        bool shouldLog(LogLevel::value level) { return level >= _level; }
        // 【一次日志调用如何变成完整文本】
        // 这里做两次不同的格式化。vasprintf 展开正文里的 %d、%s 等；Formatter 再按日志模式填时间、级别和正文。
        void log(LogLevel::value level, const char *file, size_t line, const char *fmt, va_list al) {
            char *buf;
            std::string msg;
            // vasprintf 为正文分配内存，返回正文长度。成功时按长度复制到 std::string，再释放 C 缓冲；失败时使用固定的诊断正文。
            int len = vasprintf(&buf, fmt, al);
            if (len < 0) {
                msg = "格式化日志消息失败！！";
            }else {
                // assign 按返回长度复制，而不是自己用 strlen 搜索结尾。这段正文随后由 std::string 管理生命周期。
                msg.assign(buf, len);
                free(buf);
            }
            //LogMsg(name, file, line, payload, level)
            // 把格式化正文、日志器名、调用位置和级别放进 LogMsg。构造记录时还捕获当前线程和时间，后台输出时这些信息依然属于调用线程。
            LogMsg lm(_name, file, line, std::move(msg), level);
            // stringstream 临时接收 Formatter 依次写入的项目。ss.str() 取出完整文本，交给实际日志器的 logIt。
            std::stringstream ss;
            _formatter->format(ss, lm);
            logIt(std::move(ss.str()));
        }
        // logIt 的参数已经可以直接输出。派生类只需要决定立即写入还是交给后台。
        virtual void logIt(const std::string &msg) = 0;
    protected:
        // _mutex 供同步派生类串行化输出，_name 和 _level 用于识别与过滤，_formatter 和 _sinks 描述文本布局及目标集合。
        std::mutex _mutex;
        std::string _name;
        Formatter::ptr _formatter;
        std::atomic<LogLevel::value> _level;
        std::vector<LogSink::ptr> _sinks;
};

// 【同步路径 · 调用线程直接写】
// 调用者完成格式化后，在返回之前依次调用每个 Sink。适合观察最直观的调用链，目标 I/O 的耗时会计入调用耗时。
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
        // 写入前锁住本日志器。锁范围覆盖整轮遍历，同一个同步日志器的多个调用者不会交错执行这一段。
        virtual void logIt(const std::string &msg) {
            // 在持有本日志器互斥锁期间写入全部输出目标，防止同一日志器的多个线程交错输出。
            std::unique_lock<std::mutex> lock(_mutex);
            // 没有输出目标就直接返回。正常 Builder 构建时会补一个默认标准输出目标。
            if (_sinks.empty()) { return ; }
            for (auto &it : _sinks) {
                // 每个 Sink 得到相同的指针和长度。Sink 的 log 负责具体输出，文件目标与标准输出可以同时接到同一份内容。
                it->log(msg.c_str(), msg.size());
            }
        }
};

// 【异步路径 · 调用线程提交，工作线程写】
// 仍由调用线程过滤并生成文本，交给 AsyncLooper 后，后台线程处理文件或终端输出。提交速度和后台刷写速度需要分开理解。
class AsyncLogger : public Logger {
    public:
        using ptr = std::shared_ptr<AsyncLogger>;
        AsyncLogger(const std::string &name, 
            Formatter::ptr formatter, 
            std::vector<LogSink::ptr> &sinks, 
            LogLevel::value level = LogLevel::value::DEBUG): 
            Logger(name, formatter, sinks, level),
            // std::bind 将当前 AsyncLogger 的 backendLogIt 绑定为回调。_1 预留给工作线程交来的消费缓冲。
            _looper(std::make_shared<AsyncLooper>(std::bind(&AsyncLogger::backendLogIt, this, std::placeholders::_1))) {
            std::cout << LogLevel::toString(level) << "异步日志器: " << name << "创建成功...\n";
        }
    protected:
        // push 将完整文本复制到写缓冲，所以这个函数返回后，调用端的临时 std::string 可以结束生命周期。
        // 空间不足时生产者会等缓冲交换。异步省去调用线程内的 Sink I/O，但生产过快时仍会产生背压。
        virtual void logIt(const std::string &msg) {
            _looper->push(msg);
        }
        // 后台回调拿到的是多条日志拼成的字节区间。这里一次调用一个 Sink，将整个批次送出，消息边界由文本自身的换行体现。
        void backendLogIt(Buffer &msg) {
            if (_sinks.empty()) { return; }
            for (auto &it : _sinks) {
                // Buffer::begin 给出首地址，readAbleSize 给出长度。输出目标收到的是有效区间，因此无需访问已读前缀或尾部空位。
                it->log(msg.begin(), msg.readAbleSize());
            }
        }
    protected:
        // _looper 在派生类析构期间释放，其析构等待工作线程退出。回调所需的基类 Sink 成员随后才会析构。
        AsyncLooper::ptr _looper;
};

// 【本地建造者 · 创建后由调用方持有】
// “本地”指不登记到全局名称表。调用方可直接使用返回的 Logger 指针，按名称的 getLogger 查不到这个实例。
class LocalLoggerBuilder: public Logger::Builder {
    public:
        virtual Logger::ptr build() {
            // 先检查名称。名称为空会打印诊断并终止进程，因此通常把 buildLoggerName 作为配置第一步。
            if (_logger_name.empty()) {
                std::cout << "日志器名称不能为空！！";
                abort();
            }
            // 缺少 Formatter 时创建默认 Formatter。其实际默认格式以 formatter.hpp 的构造参数为准。
            if (_formatter.get() == nullptr) {
                std::cout << "当前日志器：" << _logger_name << " 未检测到日志格式，默认设置为[ %d{%H:%M:%S}%T%t%T[%p]%T[%c]%T%f:%l%T%m%n ]!\n";
                _formatter = std::make_shared<Formatter>();
            }
            // 缺少输出目标时追加 StdoutSink。这样只设置名称也能建出一个可往标准输出写的日志器。
            if (_sinks.empty()) {
                std::cout << "当前日志器：" << _logger_name << " 未检测到落地方向，默认设置为标准输出!\n";
                _sinks.push_back(std::make_shared<StdoutSink>());
            }
            // 最后用类型值选择 AsyncLogger 或 SyncLogger，传入准备好的同一组配置，然后返回基类共享指针。
            Logger::ptr lp;
            if (_logger_type == Logger::Type::LOGGER_ASYNC) {
                lp = std::make_shared<AsyncLogger>(_logger_name, _formatter, _sinks, _level);
            }else {
                lp = std::make_shared<SyncLogger>(_logger_name, _formatter, _sinks, _level);
            }
            return lp;
        }
};


// 【全局日志器管理器 · 名称到实例的注册表】
// bitlog.h 的 getLogger 和 rootLogger 都转到这里。名称表保证调用方可以在不同函数中拿到同一个实例。
class loggerManager{
    private:
        // 表保存共享指针，因此即使创建函数返回，已注册实例仍会存活。互斥锁串行化查询和插入。
        std::mutex _mutex;
        Logger::ptr _root_logger;
        std::unordered_map<std::string, Logger::ptr> _loggers;
    private:
        // 初始化管理器时用本地建造者创建同步 root，避免在初始化阶段通过全局建造者再次调用同一个管理器。
        loggerManager(){ 
            std::unique_ptr<LocalLoggerBuilder> slb(new LocalLoggerBuilder());
            slb->buildLoggerName("root");
            slb->buildLoggerType(Logger::Type::LOGGER_SYNC);
            // _root_logger 与名称为 root 的表项持有同一个实例。默认宏走直接访问方法，显式 getLogger("root") 走名称表。
            _root_logger = slb->build();
            _loggers.insert(std::make_pair("root", _root_logger));
        }
        // 删除复制构造和赋值，防止调用方把管理器复制成另一份注册表。
        loggerManager(const loggerManager&) = delete;
        loggerManager &operator=(const loggerManager&) = delete;
    public:
        // 函数局部静态对象在第一次调用时初始化一次。C++11 保证并发的首次初始化由运行时同步。
        static loggerManager& getInstance() {
            static loggerManager lm;
            return lm;
        }
        // hasLogger 只回答检查当时是否已登记。锁在返回时释放，因此“先检查再注册”跨两次调用，并非一个原子操作。
        bool hasLogger(const std::string &name)  {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _loggers.find(name);
            if (it == _loggers.end()) {
                return false;
            }
            return true;
        }
        // addLogger 用 insert 登记。键已存在时保持旧条目，当前方法没有返回 insert 是否成功。
        void addLogger(const std::string &name, const Logger::ptr logger) {
            std::unique_lock<std::mutex> lock(_mutex);
            _loggers.insert(std::make_pair(name, logger));
        }
        // getLogger 在锁内查表并复制指针。查不到返回空指针；查到的共享指针可在解锁后继续使用。
        Logger::ptr getLogger(const std::string &name) {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _loggers.find(name);
            if (it == _loggers.end()) {
                return Logger::ptr();
            }
            return it->second;
        }
        // rootLogger 在锁内取出默认实例的共享指针，调用方无需手动查字符串键。
        Logger::ptr rootLogger() {
            std::unique_lock<std::mutex> lock(_mutex);
            return _root_logger;
        }
};

// 【全局建造者 · 创建并登记名称】
// 它与本地建造者共享配置接口，build 额外把实例加入管理器。示例和基准代码都靠这一步配合 getLogger(name)。
class GlobalLoggerBuilder: public Logger::Builder {
    public:
        virtual Logger::ptr build() {
            if (_logger_name.empty()) {
                std::cout << "日志器名称不能为空！！";
                abort();
            }
            // 断言检查名称未重复。定义 NDEBUG 时不会执行这个预检，后面的 insert 遇到重名会保留原注册值。
            assert(loggerManager::getInstance().hasLogger(_logger_name) == false);
            // 与本地建造者一样，未指定的格式器和输出目标会补齐，随后按类型创建派生实例。
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
            // 先登记再返回。同一个实例由注册表和调用者持有，之后 bitlog::getLogger(name) 可以访问它。
            loggerManager::getInstance().addLogger(_logger_name, lp);
            return lp;
        }
};

}

#endif