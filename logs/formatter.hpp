// 把日志格式串转换为一组可复用的输出单元，再依序把 LogMsg 字段和固定文本写入流。
#ifndef __M_FMT_H__
#define __M_FMT_H__

#include "util.hpp"
#include "message.hpp"
#include "level.hpp"
#include <memory>
#include <vector>
#include <tuple>

namespace bitlog{

// 每个 FormatItem 负责输出一段内容。复合格式通过组合这些对象实现，扩展字段时可为其提供新的实现。
class FormatItem{
    public:
        using ptr = std::shared_ptr<FormatItem>;
        virtual ~FormatItem() {}
        virtual void format(std::ostream &os, const LogMsg &msg) = 0;
};
// %m 对应已经用 printf 规则生成的正文，此处直接写入，不再解释正文中的百分号。
class MsgFormatItem : public FormatItem {
    public:
        MsgFormatItem(const std::string &str = ""){}
        virtual void format(std::ostream &os, const LogMsg &msg) {
            os << msg._payload;
        }
};
// %p 通过 LogLevel 的映射将枚举转成文本。
class LevelFormatItem : public FormatItem {
    public:
        LevelFormatItem(const std::string &str = ""){}
        virtual void format(std::ostream &os, const LogMsg &msg) {
            os << LogLevel::toString(msg._level);
        }
};
// %c 写出产生消息的日志器名称。
class NameFormatItem : public FormatItem {
    public:
        NameFormatItem(const std::string &str = ""){}
        virtual void format(std::ostream &os, const LogMsg &msg) {
            os << msg._name;
        }
};
// %t 写出构造 LogMsg 时捕获的线程 ID，因此异步输出时仍标识原调用线程。
class ThreadFormatItem : public FormatItem {
    public:
        ThreadFormatItem(const std::string &str = ""){}
        virtual void format(std::ostream &os, const LogMsg &msg) {
            os << msg._tid;
        }
};
// %d 可跟随花括号包裹的 strftime 子格式。未指定或传入空子格式时使用时分秒。
class TimeFormatItem : public FormatItem {
    private:
        std::string _format;
    public:
        TimeFormatItem(const std::string &format = "%H:%M:%S"):_format(format){
            if (format.empty()) _format = "%H:%M:%S";
        }
        // 把消息时间戳转换成当地时间。localtime_r 将结果写入当前调用的栈对象，供固定大小的字符数组格式化。
        virtual void format(std::ostream &os, const LogMsg &msg) {
            time_t t = msg._ctime;
            struct tm lt;
            localtime_r(&t, &lt);
            char tmp[128];
            strftime(tmp, 127, _format.c_str(), &lt);
            os << tmp;
        }
};
// %f 和下方的 %l 分别写出调用位置的文件名和行号。
class CFileFormatItem : public FormatItem {
    public:
        CFileFormatItem(const std::string &str = ""){}
        virtual void format(std::ostream &os, const LogMsg &msg) {
            os << msg._file;
        }
};
class CLineFormatItem : public FormatItem {
    public:
        CLineFormatItem(const std::string &str = ""){}
        virtual void format(std::ostream &os, const LogMsg &msg) {
            os << msg._line;
        }
};
// %T 输出一个制表符。%n 输出换行字符，何时刷新流由输出侧决定。
class TabFormatItem : public FormatItem {
    public:
        TabFormatItem(const std::string &str = ""){}
        virtual void format(std::ostream &os, const LogMsg &msg) {
            os << "\t";
        }
};
class NLineFormatItem : public FormatItem {
    public:
        NLineFormatItem(const std::string &str = ""){}
        virtual void format(std::ostream &os, const LogMsg &msg) {
            os << "\n";
        }
};
// 普通文本也包装成一个输出单元，这样可以按顺序混排标点、固定前缀和动态字段。
class OtherFormatItem : public FormatItem {
    private:
        std::string _str;
    public:
        OtherFormatItem(const std::string &str = ""):_str(str){}
        virtual void format(std::ostream &os, const LogMsg &msg) {
            os << _str;
        }
};
// Formatter 保存原始格式串和解析得到的单元序列，同一格式可用于多条消息。
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
        // 默认行依次包含时间、线程、级别、日志器、文件和行号，随后是正文和换行。
        Formatter(const std::string &pattern = "[%d{%H:%M:%S}][%t][%p][%c][%f:%l] %m%n"):
            _pattern(pattern){
            // 当前把 parsePattern 放在 assert 表达式里。定义 NDEBUG 时该表达式会被移除，构造过程也就不会填充 _items。
            assert(parsePattern());
        }
        const std::string pattern() { return _pattern; }
        // 此重载生成完整字符串。另一个重载把内容写入调用方的流并返回该流引用。
        std::string format(const LogMsg &msg) {
            std::stringstream ss;
            for (auto &it : _items) {
                it->format(ss, msg);
            }
            return ss.str();
        }
        std::ostream& format(std::ostream &os, const LogMsg &msg) {
            for (auto &it : _items) {
                it->format(os, msg);
            }
            return os;
        }
        // 格式字符区分大小写。只有时间输出单元使用传入的子格式，其余单元的构造函数接受该参数但忽略它。
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
            return FormatItem::ptr();
        }
        //pattern解析 
        // 第一阶段从左到右切分成普通文本和占位符。第二阶段按切分顺序构造输出单元。
        bool parsePattern() {
            //std::string _pattern = "sg{}fsg%d{%H:%M:%S}%Tsdf%t%T[%p]%T[%c]%T%f:%l%T%m%n  ";
            //std::cout << _pattern << std::endl;
            //每个要素分为三部分：
            //  格式化字符  : %d  %T  %p...
            //  对应的输出子格式 ： {%H:%M:%S}
            //  对应数据的类型 ： 0-表示原始字符串，也就是非格式化字符，1-表示格式化数据类型
            //  默认格式 "%d{%H:%M:%S}%T%t%T[%p]%T[%c]%T%f:%l%T%m%n"
            std::vector<std::tuple<std::string, std::string, int>> arry;
            std::string format_key;//存放%后的格式化字符
            std::string format_val;//存放格式化字符后边 {} 中的子格式字符串
            std::string string_row;//存放原始的非格式化字符
            bool sub_format_error = false;
            int pos = 0;
            while (pos < _pattern.size()) {
                // 连续的普通字符先累积，遇到真正的占位符或到达末尾时再统一提交。
                if (_pattern[pos] != '%') {
                    string_row.append(1, _pattern[pos++]);
                    continue;
                }
                // 双百分号表示字面百分号。将它并入普通文本，跳过两个输入字符。
                if (pos+1 < _pattern.size() && _pattern[pos+1] == '%') {
                    string_row.append(1, '%');
                    pos += 2;
                    continue;
                }
                // 在提交新占位符之前先提交已积累的字面文本，保持与原格式串相同的输出顺序。
                if (string_row.empty() == false) {
                    arry.push_back(std::make_tuple(string_row, "", 0));
                    string_row.clear();
                }
                //当前位置是%字符位置
                // 百分号后只读取一个字母。是否是受支持的格式字符留到创建输出单元时检查。
                pos += 1;//pos指向格式化字符位置
                if (pos < _pattern.size() && isalpha(_pattern[pos])) {
                    format_key = _pattern[pos];//保存格式化字符
                }else {
                    std::cout << &_pattern[pos-1] << "位置附近格式错误！\n";
                    return false;
                }

                // 若有左花括号，子格式一直收集到第一个右花括号。这里不解析子格式内部的百分号，也不处理嵌套花括号。
                pos += 1;//pos指向格式化字符的下一个位置，判断是否包含有子格式 %d{%Y-%m-%d}
                if (pos < _pattern.size() && _pattern[pos] == '{') {
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
                arry.push_back(std::make_tuple(format_key, format_val, 1));
                format_key.clear();
                format_val.clear();
            }
            // 未闭合的子格式会导致解析失败。末尾仍有字面文本时需要单独提交。
            if (sub_format_error) {
                std::cout << "{}对应出错\n";
                return false;
            }
            if (string_row.empty() == false) arry.push_back(std::make_tuple(string_row, "", 0));
            if (format_key.empty() == false) arry.push_back(std::make_tuple(format_key, format_val, 1));
            // 类型标记 0 创建固定文本单元，类型标记 1 交给 createItem。识别不到的格式字符会打印诊断并返回 false。
            for (auto &it : arry) {
                if (std::get<2>(it) == 0) {
                    FormatItem::ptr fi(new OtherFormatItem(std::get<0>(it)));
                    _items.push_back(fi);
                }else {
                    FormatItem::ptr fi = createItem(std::get<0>(it), std::get<1>(it));
                    if (fi.get() == nullptr) {
                        std::cout << "没有对应的格式化字符: %" <<  std::get<0>(it) << std::endl;
                        return false;
                    }
                    _items.push_back(fi);
                }
            }
            return true;
        }
    private:
        std::string _pattern;
        std::vector<FormatItem::ptr> _items;
};
}
#endif