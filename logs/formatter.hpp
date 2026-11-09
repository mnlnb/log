// 【模块导读 · 把记录排成可读的一行】
// 输入一是格式模板，例如 "[%p][%c] %m%n"；输入二是一条 LogMsg。给定 INFO、demo 和正文“开始”，结果就是 "[INFO][demo] 开始" 再加一个换行。
// Formatter 在构造时先解析模板，生成按输出顺序排列的一组 FormatItem；每次写日志时重复使用这些单元，把记录里的值写到流中。
// 模板可混合普通文本、% 占位符和花括号子格式。比如 "[%d{%Y-%m-%d}]" 的方括号直接输出，%d 输出时间，花括号提供时间格式。
// 建议先读 Formatter 的构造与两个 format 重载，再看 parsePattern 的两阶段流程，最后按 %m、%p、%d 回看对应的单元。
#ifndef __M_FMT_H__
#define __M_FMT_H__

#include "util.hpp"
#include "message.hpp"
#include "level.hpp"
#include <memory>
#include <vector>
#include <tuple>

namespace bitlog{

// FormatItem 表示“一段输出”。统一接口接收同一个 LogMsg 和目标流，各派生类型只负责自己那一段。
class FormatItem{
    public:
        using ptr = std::shared_ptr<FormatItem>;
        // 基类用虚析构函数支持通过 FormatItem 共享指针管理派生对象。
        virtual ~FormatItem() {}
        virtual void format(std::ostream &os, const LogMsg &msg) = 0;
};
// %m 读取 _payload。业务传入的 printf 风格参数已经在 Logger::log 展开，因此这里直接输出正文里的字符。
class MsgFormatItem : public FormatItem {
    public:
        MsgFormatItem(const std::string &str = ""){}
        virtual void format(std::ostream &os, const LogMsg &msg) {
            os << msg._payload;
        }
};
// %p 读取 _level，再用 LogLevel::toString 转成人能读的级别名，例如 ERROR。
class LevelFormatItem : public FormatItem {
    public:
        LevelFormatItem(const std::string &str = ""){}
        virtual void format(std::ostream &os, const LogMsg &msg) {
            os << LogLevel::toString(msg._level);
        }
};
// %c 输出 _name，即注册或配置的日志器名称。多套日志器写到同一目标时可借它区分来源。
class NameFormatItem : public FormatItem {
    public:
        NameFormatItem(const std::string &str = ""){}
        virtual void format(std::ostream &os, const LogMsg &msg) {
            os << msg._name;
        }
};
// %t 输出 LogMsg 构造时捕获的线程 ID。即便最终由后台线程写文件，这个字段仍描述调用日志的线程。
class ThreadFormatItem : public FormatItem {
    public:
        ThreadFormatItem(const std::string &str = ""){}
        virtual void format(std::ostream &os, const LogMsg &msg) {
            os << msg._tid;
        }
};
// %d 使用保存在 _format 中的 strftime 子格式。空子格式默认是 "%H:%M:%S"，因此单写 %d 会得到时分秒。
class TimeFormatItem : public FormatItem {
    private:
        std::string _format;
    public:
        TimeFormatItem(const std::string &format = "%H:%M:%S"):_format(format){
            if (format.empty()) _format = "%H:%M:%S";
        }
        // 先把秒级时间戳转为当地时间，再按 _format 格式化到栈上的字符数组。localtime_r 把转换结果写进本次调用的结构体。
        virtual void format(std::ostream &os, const LogMsg &msg) {
            time_t t = msg._ctime;
            struct tm lt;
            localtime_r(&t, &lt);
            char tmp[128];
            // strftime 把结果写入 tmp。当前实现使用固定大小的数组，也直接将结果送入流，适合常见的短时间格式。
            strftime(tmp, 127, _format.c_str(), &lt);
            os << tmp;
        }
};
// %f 使用记录里的源码文件名，%l 使用记录里的源码行号。常把两者用冒号拼在一起定位调用点。
class CFileFormatItem : public FormatItem {
    public:
        CFileFormatItem(const std::string &str = ""){}
        virtual void format(std::ostream &os, const LogMsg &msg) {
            os << msg._file;
        }
};
// %l 取的是记录创建时携带的行号，通常对应业务侧写 LOG_INFO 等宏的那一行。
class CLineFormatItem : public FormatItem {
    public:
        CLineFormatItem(const std::string &str = ""){}
        virtual void format(std::ostream &os, const LogMsg &msg) {
            os << msg._line;
        }
};
// %T 写入制表符，适合模板中需要对齐的位置；它和普通空格在最终文本中是不同字符。
class TabFormatItem : public FormatItem {
    public:
        TabFormatItem(const std::string &str = ""){}
        virtual void format(std::ostream &os, const LogMsg &msg) {
            os << "\t";
        }
};
// %n 写入换行字符。它决定文本的换行，刷新或写入文件由下游 Sink 及其流处理。
class NLineFormatItem : public FormatItem {
    public:
        NLineFormatItem(const std::string &str = ""){}
        virtual void format(std::ostream &os, const LogMsg &msg) {
            os << "\n";
        }
};
// OtherFormatItem 保存一段字面文本，例如 "[" 或 "] "。它实现同一输出接口，所以普通文字和占位符可放在同一个序列。
class OtherFormatItem : public FormatItem {
    private:
        std::string _str;
    public:
        OtherFormatItem(const std::string &str = ""):_str(str){}
        virtual void format(std::ostream &os, const LogMsg &msg) {
            os << _str;
        }
};
// Formatter 持有原始模板和解析后的输出序列。常规用法在日志器配置时构造一次，处理每条记录时遍历已解析的序列。
class Formatter {
    public:
        using ptr = std::shared_ptr<Formatter>;
        /*
            %d 日期
            %T 缩进
            %t 线程id
            %p 日志级别
            %c 日志器名称
            %f 文件名
            %l 行号
            %m 日志消息
            %n 换行
        */
        //时间{年-月-日 时:分:秒}缩进 线程ID 缩进 [日志级别] 缩进 [日志名称] 缩进 文件名:行号 缩进 消息换行
        // 实际默认模式的顺序是时间、线程、级别、名称、文件及行号，再接一个空格、正文和换行。上方旧注释中的制表符示例可作为自定义模式参考。
        Formatter(const std::string &pattern = "[%d{%H:%M:%S}][%t][%p][%c][%f:%l] %m%n"):
            _pattern(pattern){
            // 构造中的 assert 既调用解析也检查其返回值。若定义 NDEBUG，整个表达式会省略，当前实现也就不会构造 _items。
            assert(parsePattern());
        }
        // pattern 返回保存的模板文本。解析后的 FormatItem 序列另存于 _items。
        const std::string pattern() { return _pattern; }
        // 这个重载把各项依序写进临时字符串流，最后返回整行。适合调用者直接拿到 std::string。
        std::string format(const LogMsg &msg) {
            std::stringstream ss;
            for (auto &it : _items) {
                it->format(ss, msg);
            }
            return ss.str();
        }
        // 这个重载用调用者已有的 ostream，写完把同一个流引用返回。Logger::log 调用的是此版本。
        std::ostream& format(std::ostream &os, const LogMsg &msg) {
            for (auto &it : _items) {
                it->format(os, msg);
            }
            return os;
        }
        // createItem 将一个格式字母派发到具体类型。大小写分别比较，比如 %t 是线程 ID、%T 是制表符。
        FormatItem::ptr createItem(const std::string &fc, const std::string &subfmt) {
            if (fc == "m") return FormatItem::ptr(new MsgFormatItem(subfmt));
            if (fc == "p") return FormatItem::ptr(new LevelFormatItem(subfmt));
            if (fc == "c") return FormatItem::ptr(new NameFormatItem(subfmt));
            if (fc == "t") return FormatItem::ptr(new ThreadFormatItem(subfmt));
            if (fc == "n") return FormatItem::ptr(new NLineFormatItem(subfmt));
            if (fc == "d") return FormatItem::ptr(new TimeFormatItem(subfmt));
            if (fc == "f") return FormatItem::ptr(new CFileFormatItem(subfmt));
            if (fc == "l") return FormatItem::ptr(new CLineFormatItem(subfmt));
            if (fc == "T") return FormatItem::ptr(new TabFormatItem(subfmt));
            // 找不到匹配字符时返回空共享指针，外层解析器负责提示未知占位符并报告失败。
            return FormatItem::ptr();
        }
        //pattern解析 
        // 【模板解析 · 先切片，再建对象】
        // 先扫描模板生成三元组，再把三元组变成 FormatItem。用 "[%p] %m%n" 想一遍，会按顺序得到 "["、p、"] "、m、n。
        bool parsePattern() {
            //std::string _pattern = "sg{}fsg%d{%H:%M:%S}%Tsdf%t%T[%p]%T[%c]%T%f:%l%T%m%n  ";
            //std::cout << _pattern << std::endl;
            //每个要素分为三部分：
            //  格式化字符  : %d  %T  %p...
            //  对应的输出子格式 ： {%H:%M:%S}
            //  对应数据的类型 ： 0-表示原始字符串，也就是非格式化字符，1-表示格式化数据类型
            //  默认格式 "%d{%H:%M:%S}%T%t%T[%p]%T[%c]%T%f:%l%T%m%n"
            // 三元组前两项保存文本或格式键、可选子格式，最后一项区分普通文本（0）与占位符（1）。
            std::vector<std::tuple<std::string, std::string, int>> arry;
            // format_key 与 format_val 保存当前占位符，string_row 累积相邻字面字符。pos 指向下一段待处理输入。
            std::string format_key;//存放%后的格式化字符
            std::string format_val;//存放格式化字符后边 {} 中的子格式字符串
            std::string string_row;//存放原始的非格式化字符
            bool sub_format_error = false;
            int pos = 0;
            while (pos < _pattern.size()) {
                // 普通字符直接并入 string_row，并推进 pos。连续文字因此可合并成一个 OtherFormatItem。
                if (_pattern[pos] != '%') {
                    string_row.append(1, _pattern[pos++]);
                    continue;
                }
                // 遇到 %% 就把它当作单个字面百分号。跳过两个输入字符后继续扫描，例如 "100%%" 输出 "100%"。
                if (pos+1 < _pattern.size() && _pattern[pos+1] == '%') {
                    string_row.append(1, '%');
                    pos += 2;
                    continue;
                }
                // 遇到真正的占位符前，先提交已经积累的普通文字并清空。这样占位符前的方括号和标点不会被调换顺序。
                if (string_row.empty() == false) {
                    arry.push_back(std::make_tuple(string_row, "", 0));
                    string_row.clear();
                }
                //当前位置是%字符位置
                // 百分号后的第一个字符必须是字母。若到达模板末尾或遇到非字母，立即打印该位置附近的错误。
                pos += 1;//pos指向格式化字符位置
                if (pos < _pattern.size() && isalpha(_pattern[pos])) {
                    format_key = _pattern[pos];//保存格式化字符
                }else {
                    std::cout << &_pattern[pos-1] << "位置附近格式错误！\n";
                    return false;
                }

                // 键后紧接左花括号时扫描子格式。从左花括号后开始收集，遇到第一个右花括号即结束。
                // 例如 %d{%H:%M} 把 d 存为键、%H:%M 存为子格式。子格式中的百分号留给时间格式化处理。
                pos += 1;//pos指向格式化字符的下一个位置，判断是否包含有子格式 %d{%Y-%m-%d}
                if (pos < _pattern.size() && _pattern[pos] == '{') {
                    // 先记为未闭合，读到右花括号才清掉标志。模板用尽还没读到时，由外层统一报告括号错误。
                    sub_format_error = true;
                    pos += 1;//pos指向花括号下一个字符处
                    while(pos < _pattern.size()) {
                        if (_pattern[pos] == '}') {
                            sub_format_error = false;
                            pos += 1;//让pos指向}的下一个字符处
                            break;
                        }
                        format_val.append(1, _pattern[pos++]);
                    }
                }
                // 一个占位符结束后按标记 1 保存，清空当前键与子格式，确保下一个占位符独立解析。
                arry.push_back(std::make_tuple(format_key, format_val, 1));
                format_key.clear();
                format_val.clear();
            }
            // 未闭合子格式会使解析返回 false。通过检查后，保存扫描末尾可能剩余的字面文本。
            if (sub_format_error) {
                std::cout << "{}对应出错\n";
                return false;
            }
            if (string_row.empty() == false) arry.push_back(std::make_tuple(string_row, "", 0));
            if (format_key.empty() == false) arry.push_back(std::make_tuple(format_key, format_val, 1));
            // 第二阶段按出现顺序遍历三元组。文字交给 OtherFormatItem，格式键交给 createItem，最后把生成对象追加到 _items。
            for (auto &it : arry) {
                if (std::get<2>(it) == 0) {
                    FormatItem::ptr fi(new OtherFormatItem(std::get<0>(it)));
                    _items.push_back(fi);
                }else {
                    FormatItem::ptr fi = createItem(std::get<0>(it), std::get<1>(it));
                    // 例如 %z 的 z 在第一阶段是合法字母，但在这一步找不到对应单元，因此报错并返回 false。
                    if (fi.get() == nullptr) {
                        std::cout << "没有对应的格式化字符: %" <<  std::get<0>(it) << std::endl;
                        return false;
                    }
                    _items.push_back(fi);
                }
            }
            // 走到这里表示所有片段都已转换。此后 format 只按顺序调用 _items 中每个对象的 format。
            return true;
        }
    private:
        std::string _pattern;
        std::vector<FormatItem::ptr> _items;
};
}
#endif