// 【模块导读 · 三种写法如何接收可变数量的参数】
// 前半段演示 C 风格 va_list 和 vasprintf，中间展示自动带上源位置的宏，后半段用模板逐个输出不同类型的参数。
// Logger 的五个级别入口类似前半段，用 va_list 把正文参数交给格式化函数；bitlog.h 的宏则类似这里的 LOG。
// 阅读时可以先看 main 中一次调用的实参，再回到对应函数，看每个额外参数是由谁、按什么规则取出来的。
#include <iostream>
#include <cstdarg>
#include <memory>
#include <functional>

// print 只取一个额外的 int。fmt 作为最后一个具名参数传给 va_start，函数没有按其内容读取格式。
void print(const char *fmt, ...) {
    va_list al;
    // va_start 初始化遍历状态，va_arg 指定期望类型并取出当前参数。实际传入的类型和数量必须符合这个约定。
    va_start(al, fmt);//让al指向fmt参数之后的第一个可变参数
    int num = va_arg(al, int);
    // va_end 结束对参数列表的使用。它是与 va_start 配对的清理操作，含义不等同于把变量赋成空值。
    va_end(al);//清空可变参数列表--其实是将al置空
    std::cout << num << std::endl;
}
// printNum 用 n 说明后面有多少个 int，同时用 n 作为 va_start 的锚点。
void printNum(int n, ...) {
    va_list al;
    va_start(al, n);//让al指向fmt参数之后的第一个可变参数
    // 循环调用 va_arg 每次向前取一个 int，逐项输出。参数不足或类型不符都会违反调用约定。
    for (int i = 0; i < n; i++) {
        int num = va_arg(al, int);
        std::cout << num << std::endl;
    }
    va_end(al);//清空可变参数列表--其实是将al置空
}

// myprintf 把 fmt 和 va_list 交给 vasprintf，格式串描述应以哪些类型读取后续实参。
void myprintf(const char *fmt, ...) {
    //int vasprintf(char **strp, const char *fmt, va_list ap);
    char *res;
    va_list al;
    va_start(al, fmt);
    // 成功后 res 指向新分配的字符缓冲区。示例直接输出它再用 free 释放，现有代码没有按 len 的失败值分支。
    int len = vasprintf(&res, fmt, al);
    va_end(al);
    std::cout << res << std::endl;
    free(res);
}

// LOG 宏在调用位置生成 "[文件:行号]" 前缀，然后把调用方的格式文本和行尾拼起来。fmt 应是能相邻拼接的字符串字面量。
// ##__VA_ARGS__ 允许 GNU 编译模式省略额外参数；有额外参数时会按 printf 的约定传入。
#define LOG(fmt, ...) printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)


// 无参重载是模板递归终点，负责换行。它也可以由调用者直接调用。
void xprintf() {
    std::cout << std::endl;
}
// 模板先从参数包里拿出第一个值，用 ostream 的 << 输出，再把剩余实参完美转发给下一轮。
template<typename T, typename ...Args>
void xprintf(const T &value, Args &&...args) {
    std::cout << value << " ";
    // sizeof... 统计剩余实参数量。还有实参时递归处理，最后一个值处理完则调用无参重载。
    if ((sizeof ...(args)) > 0) {
        xprintf(std::forward<Args>(args)...);
    }else {
        xprintf();
    }
}
// main 用字符串与整数混合演示模板，用 LOG 演示调用位置，用 printNum 和 myprintf 对照两种 C 风格读取方法。
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