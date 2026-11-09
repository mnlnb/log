// 对照演示 C 风格可变参数、带源码位置的可变参数宏，以及 C++ 可变参数模板。
#include <iostream>
#include <cstdarg>
#include <memory>
#include <functional>

// 以最后一个具名参数 fmt 初始化 va_list。这里只读取第一个额外参数并按 int 解释，不解析格式串。
void print(const char *fmt, ...) {
    va_list al;
    va_start(al, fmt);//让al指向fmt参数之后的第一个可变参数
    int num = va_arg(al, int);
    // va_end 结束本次遍历并清理关联状态，调用后不得继续用该列表取参。
    va_end(al);//清空可变参数列表--其实是将al置空
    std::cout << num << std::endl;
}
// n 是要读取的 int 参数个数，也是 va_start 使用的最后一个具名参数。调用方必须提供数量和类型匹配的参数。
void printNum(int n, ...) {
    va_list al;
    va_start(al, n);//让al指向fmt参数之后的第一个可变参数
    for (int i = 0; i < n; i++) {
        int num = va_arg(al, int);
        std::cout << num << std::endl;
    }
    va_end(al);//清空可变参数列表--其实是将al置空
}

// 把参数列表交给 vasprintf 生成字符串。成功时缓冲由调用方用 free 释放，当前示例未分支处理失败返回值。
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

// 宏在展开位置附加文件名和行号，并拼接行尾换行。fmt 需能与相邻字符串字面量拼接。
#define LOG(fmt, ...) printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)


// 无参重载提供递归终点，只输出换行。
void xprintf() {
    std::cout << std::endl;
}
// 模板每次输出首个参数，再完美转发剩余参数。这样可在同一次调用中传入可写入流的不同类型。
template<typename T, typename ...Args>
void xprintf(const T &value, Args &&...args) {
    std::cout << value << " ";
    if ((sizeof ...(args)) > 0) {
        xprintf(std::forward<Args>(args)...);
    }else {
        xprintf();
    }
}
// 入口用不同参数数量与类型展示三种接口的调用方式。
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