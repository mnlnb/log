#include <iostream>
#include <cstdarg>
#include <memory>
#include <functional>

void print(const char *fmt, ...) {
    va_list al;
    va_start(al, fmt);//让al指向fmt参数之后的第一个可变参数
    int num = va_arg(al, int);
    va_end(al);//清空可变参数列表--其实是将al置空
    std::cout << num << std::endl;
}
void printNum(int n, ...) {
    va_list al;
    va_start(al, n);//让al指向fmt参数之后的第一个可变参数
    for (int i = 0; i < n; i++) {
        int num = va_arg(al, int);
        std::cout << num << std::endl;
    }
    va_end(al);//清空可变参数列表--其实是将al置空
}

void myprintf(const char *fmt, ...) {
    //int vasprintf(char **strp, const char *fmt, va_list ap);
    char *res;
    va_list al;
    va_start(al, fmt);
    int len = vasprintf(&res, fmt, al);
    va_end(al);
    std::cout << res << std::endl;
    free(res);
}

#define LOG(fmt, ...) printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)


void xprintf() {
    std::cout << std::endl;
}
template<typename T, typename ...Args>
void xprintf(const T &value, Args &&...args) {
    std::cout << value << " ";
    if ((sizeof ...(args)) > 0) {
        xprintf(std::forward<Args>(args)...);
    }else {
        xprintf();
    }
}
int main()
{
    xprintf("比特");
    xprintf("比特", 666);
    xprintf("比特", "就业课", 666);
    LOG("%s-%s", "hello", "比特就业课");
    printNum(3, 11,22,33);
    printNum(5, 44,55,66,77,88);
    myprintf("%s-%d", "小明", 18);
    return 0;
}