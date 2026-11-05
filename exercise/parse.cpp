#include <iostream>
#include <string>
#include <vector>
#include <tuple>
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
    while (pos < _pattern.size()) {
        if (_pattern[pos] != '%') {
            string_row.append(1, _pattern[pos++]);
            continue;
        }
        if (pos+1 < _pattern.size() && _pattern[pos+1] == '%') {
            string_row.append(1, '%');
            pos += 2;
            continue;
        }
        if (string_row.empty() == false) {
            std::cout << "[" << string_row << "]<-->["  << "] " << 0 <<  std::endl;
            arry.push_back(std::make_tuple(string_row, "", 0));
            string_row.clear();
        }
        //当前位置是%字符位置
        pos += 1;//pos指向格式化字符位置
        if (pos < _pattern.size() && isalpha(_pattern[pos])) {
            format_key = _pattern[pos];//保存格式化字符
        }else {
            std::cout << &_pattern[pos-1] << "位置附近格式错误！\n";
            return false;
        }

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
        std::cout << "[" << format_key << "]<-->[" << format_val << "] 1" << std::endl;
        arry.push_back(std::make_tuple(format_key, format_val, 1));
        format_key.clear();
        format_val.clear();
    }
    if (string_row.empty() == false) arry.push_back(std::make_tuple(string_row, "", 0));
    if (format_key.empty() == false) arry.push_back(std::make_tuple(format_key, format_val, 1));
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