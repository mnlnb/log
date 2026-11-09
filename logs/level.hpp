// 定义日志级别和输出文本。Logger 按枚举的数值顺序比较消息级别与配置的阈值。
#ifndef __M_LEVEL_H__
#define __M_LEVEL_H__

namespace bitlog{
class LogLevel{
    public:
        // 从 DEBUG 到 FATAL 严重程度递增。OFF 位于所有普通消息级别之上，用作关闭这些消息的阈值。
        enum class value {
            UNKNOW = 0,
            DEBUG,
            INFO,
            WARN,
            ERROR,
            FATAL,
            OFF
        };
        // 返回指向静态字符串字面量的指针。未知枚举值走默认分支，沿用当前的 UNKNOW 拼写。
        static const char *toString(LogLevel::value l) {
            switch(l) {
                // 临时宏用 # 把枚举标识符转成文本。switch 内使用完毕后立即取消定义，避免影响其他代码。
                #define TOSTRING(name) #name
                case LogLevel::value::DEBUG: return TOSTRING(DEBUG);
                case LogLevel::value::INFO: return TOSTRING(INFO);
                case LogLevel::value::WARN: return TOSTRING(WARN);
                case LogLevel::value::ERROR: return TOSTRING(ERROR);
                case LogLevel::value::FATAL: return TOSTRING(FATAL);
                case LogLevel::value::OFF: return TOSTRING(OFF);
                #undef TOSTRING
                default: return "UNKNOW";
            }
            return "UNKNOW";
        }
};
}
#endif