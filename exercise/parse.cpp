// 日志格式解析练习。扫描固定示例并打印拆出的片段，展示字面文本、占位符和可选子格式如何区分。
#include <iostream>
#include <string>
#include <vector>
#include <tuple>
// 示例同时包含普通文本里的花括号和 %d 后的子格式。只有紧跟占位符的左花括号才进入子格式扫描。
bool parsePattern() {
    std::string _pattern = "sg{}fsg%d{%Y-%m-%d %H:%M:%S}%Tsdf%t%T[%p]%T[%c]%T%f:%l%T%m%n  ";
    std::cout << _pattern << std::endl;
    //每个要素分为三部分：
    //  格式化字符  : %d  %T  %p...
    //  对应的输出格式 ： {%Y-%m-%d %H:%M:%S}
    //  对应数据的类型 ： 0-表示原始字符串，也就是非格式化字符，1-表示格式化数据类型
    //  默认格式 "sg{}fsg%d{%Y-%m-%d %H:%M:%S}%T%t%T[%p]%T[%c]%T%f:%l%T%m%n"
    std::vector<std::tuple<std::string, std::string, int>> arry;
    std::string format_key;//存放%后的格式化字符
    std::string format_val;//存放格式化字符后边 {} 中的子格式字符串
    std::string string_row;//存放原始的非格式化字符
    bool error_flag = false;
    int pos = 0;
    // pos 始终指向下一个尚未处理的字符，string_row 收集连续的字面文本。
    while (pos < _pattern.size()) {
        if (_pattern[pos] != '%') {
            string_row.append(1, _pattern[pos++]);
            continue;
        }
        // 双百分号折叠成一个普通百分号，不开始新的格式项。
        if (pos+1 < _pattern.size() && _pattern[pos+1] == '%') {
            string_row.append(1, '%');
            pos += 2;
            continue;
        }
        // 遇到新占位符前先打印并保存之前的普通文本，标记为 0，以保留片段顺序。
        if (string_row.empty() == false) {
            std::cout << "[" << string_row << "]<-->["  << "] " << 0 <<  std::endl;
            arry.push_back(std::make_tuple(string_row, "", 0));
            string_row.clear();
        }
        //当前位置是%字符位置
        // 读取百分号后的一个字母作为格式键。此练习只检查字符形态，不验证它是否对应真实的格式器。
        pos += 1;//pos指向格式化字符位置
        if (pos < _pattern.size() && isalpha(_pattern[pos])) {
            format_key = _pattern[pos];//保存格式化字符
        }else {
            std::cout << &_pattern[pos-1] << "位置附近格式错误！\n";
            return false;
        }

        // 只有下一字符是左花括号时才继续读取子格式，读取到第一个右花括号后回到外层扫描。
        pos += 1;//pos指向格式化字符的下一个位置，判断是否包含有子格式 %d{%Y-%m-%d}
        if (pos < _pattern.size() && _pattern[pos] == '{') {
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
        // 占位符连同子格式以标记 1 保存，随后清空临时字段，供下一个占位符复用。
        std::cout << "[" << format_key << "]<-->[" << format_val << "] 1" << std::endl;
        arry.push_back(std::make_tuple(format_key, format_val, 1));
        format_key.clear();
        format_val.clear();
    }
    // 末尾没有后续占位符来触发提交，因此单独保存残留字面文本。
    if (string_row.empty() == false) arry.push_back(std::make_tuple(string_row, "", 0));
    if (format_key.empty() == false) arry.push_back(std::make_tuple(format_key, format_val, 1));
    // 到达末尾时仍找不到右花括号会报错。当前函数在成功路径末尾没有显式返回 bool 值。
    if (error_flag == true) {
        std::cout << "子格式花括号匹配错误！\n";
        return false;
    }
}

int main()
{
    parsePattern();
    return 0;
}