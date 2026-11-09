// 【模块导读 · 日志级别】
// 本文件给消息严重程度一个可比较的枚举值，同时提供打印这个级别所需的文本。
// 建造者设定日志器的最低级别，Logger::shouldLog 比较消息和阈值，Formatter 的 %p 用这里的 toString 输出级别。
// 例如把日志器设成 WARN，DEBUG 与 INFO 在 Logger 入口就返回，WARN、ERROR、FATAL 继续进入格式化和输出。
#ifndef __M_LEVEL_H__
#define __M_LEVEL_H__

namespace bitlog{
class LogLevel{
    public:
        // 枚举顺序参与比较，修改顺序会改变过滤规则。OFF 大于五种普通日志级别，因此能阻止这五个入口提交消息。
        // UNKNOW 是现有的兜底枚举名，数值是 0。这里沿用源码的拼写。
        enum class value {
            UNKNOW = 0,
            DEBUG,
            INFO,
            WARN,
            ERROR,
            FATAL,
            OFF
        };
        // 把枚举转换为具有静态存储期的字符串字面量，返回值可直接送入输出流，调用方不用释放。
        static const char *toString(LogLevel::value l) {
            switch(l) {
                // 预处理器的 # 把宏参数变为字符串。例如 TOSTRING(DEBUG) 展开为 "DEBUG"。
                #define TOSTRING(name) #name
                case LogLevel::value::DEBUG: return TOSTRING(DEBUG);
                case LogLevel::value::INFO: return TOSTRING(INFO);
                case LogLevel::value::WARN: return TOSTRING(WARN);
                case LogLevel::value::ERROR: return TOSTRING(ERROR);
                case LogLevel::value::FATAL: return TOSTRING(FATAL);
                case LogLevel::value::OFF: return TOSTRING(OFF);
                // 这个临时宏只服务于转换表，使用后立即取消，避免影响包含本头文件的其他代码。
                #undef TOSTRING
                // 传入 UNKNOW 或未列出的枚举值时，走 default 返回同一个兜底文本。
                default: return "UNKNOW";
            }
            return "UNKNOW";
        }
};
}
#endif