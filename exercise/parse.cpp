// 【模块导读 · 把日志模式拆成小片段】
// 这是 formatter.hpp 中模板解析的独立练习。它从固定字符串读入字符，把普通文字和占位符拆出来，并打印扫描过程。
// 可用 "A[%d{%H:%M}] B%%" 在纸上跟踪，依次得到字面 "A["、键 d 与子格式 "%H:%M"、字面 "] B%"。
// 练习到切片为止。正式 Formatter 在这之后还会把片段转换成输出对象，并用真实的 LogMsg 生成文本。
#include <iostream>
#include <string>
#include <vector>
#include <tuple>
// 示例包含普通文本里的花括号，也包含 %d 紧跟的子格式。只有占位符后紧接的左花括号会启动子格式扫描。
bool parsePattern() {
    std::string _pattern = "sg{}fsg%d{%Y-%m-%d %H:%M:%S}%Tsdf%t%T[%p]%T[%c]%T%f:%l%T%m%n  ";
    std::cout << _pattern << std::endl;
    //每个要素分为三部分：
    //  格式化字符  : %d  %T  %p...
    //  对应的输出格式 ： {%Y-%m-%d %H:%M:%S}
    //  对应数据的类型 ： 0-表示原始字符串，也就是非格式化字符，1-表示格式化数据类型
    //  默认格式 "sg{}fsg%d{%Y-%m-%d %H:%M:%S}%T%t%T[%p]%T[%c]%T%f:%l%T%m%n"
    // arry 按扫描顺序保存三元组。普通片段使用类型 0，格式片段使用类型 1；紧随其后的局部变量分别保存键、子格式和普通字符。
    std::vector<std::tuple<std::string, std::string, int>> arry;
    std::string format_key;//存放%后的格式化字符
    std::string format_val;//存放格式化字符后边 {} 中的子格式字符串
    std::string string_row;//存放原始的非格式化字符
    bool error_flag = false;
    // pos 指向下一处待扫描位置。外层循环一次消费一个普通字符、一个转义百分号，或一整个占位符。
    int pos = 0;
    // pos 始终指向下一个尚未处理的字符，string_row 收集连续的字面文本。
    while (pos < _pattern.size()) {
        // 不等于 '%' 时加入 string_row，pos 向前移动一格。普通位置遇到花括号也走这条分支。
        if (_pattern[pos] != '%') {
            string_row.append(1, _pattern[pos++]);
            continue;
        }
        // 遇到两个连续百分号，加入一个普通 '%'，pos 跳过两格。它会与相邻普通字符合并成同一片段。
        if (pos+1 < _pattern.size() && _pattern[pos+1] == '%') {
            string_row.append(1, '%');
            pos += 2;
            continue;
        }
        // 遇到真正的占位符先打印并保存已有普通文字，然后清空累积器。这样普通文本始终位于该占位符之前。
        if (string_row.empty() == false) {
            std::cout << "[" << string_row << "]<-->["  << "] " << 0 <<  std::endl;
            arry.push_back(std::make_tuple(string_row, "", 0));
            string_row.clear();
        }
        //当前位置是%字符位置
        // 越过 '%' 后只读一个字母作为键。这里检查形状，正式版本还要检查该字母是否有对应的输出单元。
        pos += 1;//pos指向格式化字符位置
        if (pos < _pattern.size() && isalpha(_pattern[pos])) {
            format_key = _pattern[pos];//保存格式化字符
        }else {
            std::cout << &_pattern[pos-1] << "位置附近格式错误！\n";
            return false;
        }

        // 再前进一格检查左花括号。如果有，就保存内部字符直到第一个右花括号，子格式中的百分号按原字节保留。
        pos += 1;//pos指向格式化字符的下一个位置，判断是否包含有子格式 %d{%Y-%m-%d}
        if (pos < _pattern.size() && _pattern[pos] == '{') {
            // 先置 error_flag，直到遇到右花括号才清除。若直接读到结尾，外层检查会识别未闭合的子格式。
            error_flag = true;
            pos += 1;//pos指向花括号下一个字符处
            while(pos < _pattern.size()) {
                if (_pattern[pos] == '}') {
                    error_flag = false;
                    pos += 1;//让pos指向}的下一个字符处
                    break;
                }
                format_val.append(1, _pattern[pos++]);
            }
        }
        // 打印当前格式片段，连同标记 1 一起保存，再清空两个暂存字符串准备处理下一个键。
        std::cout << "[" << format_key << "]<-->[" << format_val << "] 1" << std::endl;
        arry.push_back(std::make_tuple(format_key, format_val, 1));
        format_key.clear();
        format_val.clear();
    }
    // 循环结束时补存末尾普通片段。这个片段后面没有新 '%'，因此没有在循环内的提交分支保存。
    if (string_row.empty() == false) arry.push_back(std::make_tuple(string_row, "", 0));
    if (format_key.empty() == false) arry.push_back(std::make_tuple(format_key, format_val, 1));
    // 未闭合时打印诊断并返回 false。当前成功路径走到函数末尾没有显式返回 bool 值，这是练习源码已有的边界问题。
    if (error_flag == true) {
        std::cout << "子格式花括号匹配错误！\n";
        return false;
    }
}

// main 运行一次固定样例，用打印结果观察切片顺序。它没有根据 parsePattern 的返回值继续执行逻辑。
int main()
{
    parsePattern();
    return 0;
}