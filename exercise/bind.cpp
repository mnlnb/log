// 【模块导读 · 回调绑定与对象寿命】
// 这个练习展示如何把需要 Temp& 的函数绑定成不需要实参的回调，以及回调保存引用时如何依赖原对象的生命周期。
// 先看 main 的内层大括号。回调被移动进外层 vector，局部对象却在离开括号时析构，最后一行调用因此留下悬空引用。
// 理解这里后再看日志库的 AsyncLogger。它同样绑定回调，Looper 的停止和 join 则负责约束回调的使用时机。
#include <iostream>
#include <string>
#include <functional>
#include <memory>
#include <vector>

// Temp 保存数字和文本，构造与析构都打印 this 地址。跟踪同一个地址可以看出对象在哪个作用域诞生和结束。
class Temp {
    private:
        size_t _data;
        std::string _str;
    public:
        // 构造时把传入的数字与文本放进成员。_str 自己持有字符串数据，跟回调是否持有 Temp 是两件事。
        Temp(size_t data, const std::string str):_data(data), _str(str) {
            std::cout << "构造temp" << this << std::endl;
        }
        // 析构日志发生在内层大括号结束时。外层回调仍存在，但它绑定的原 Temp 已经失效。
        ~Temp() {std::cout << "析构temp" << this << std::endl;}
        // 两个访问器只读成员，const 表示调用它们不修改 Temp。返回的字符串引用也依赖该对象仍然存活。
        size_t get_data() const {return _data; }
        const std::string& get_str() const {return _str; }
};

// print 接收活着的 Temp 引用并输出其两个字段，因此调用它的前提是引用依然有效。
void print( Temp &tmp) {
    std::cout << tmp.get_data() << tmp.get_str() << std::endl;
}
// std::function<void()> 封装可无参数调用、无返回值的对象。绑定 Temp 后，print 就能装进这种回调类型。
using Functor = std::function<void()>;
int main()
{
    // vector 先创建一个空回调槽位。它位于外层作用域，生命周期比下面的 tmp 长。
    std::vector<Functor> arry(1);
    {
        Temp tmp(99, "nihao");
        // std::ref 把 tmp 包成引用包装器，让 bind 保存对原对象的引用。即使对包装器使用 move，也不会移动或延长 tmp 的生命周期。
        Functor f = std::bind(print, std::move(std::ref(tmp)));
        // 把绑定结果移动到 vector，只是转移回调对象持有的状态。原来的 tmp 仍由内层作用域管理。
        arry[0] =  std::move(f);
    }
    // 调用时 tmp 已销毁，print 会经由悬空引用访问原对象，属于未定义行为。即使某次看起来有输出，也不能依赖该结果。
    arry[0]();
    return 0;
}