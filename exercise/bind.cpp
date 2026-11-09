// 通过 std::bind、std::ref 和 std::function 演示延迟调用及绑定对象的生命周期。
#include <iostream>
#include <string>
#include <functional>
#include <memory>
#include <vector>

// Temp 的构造和析构打印对象地址，用于观察局部变量何时离开作用域。
class Temp {
    private:
        size_t _data;
        std::string _str;
    public:
        Temp(size_t data, const std::string str):_data(data), _str(str) {
            std::cout << "构造temp" << this << std::endl;
        }
        ~Temp() {std::cout << "析构temp" << this << std::endl;}
        size_t get_data() const {return _data; }
        const std::string& get_str() const {return _str; }
};

// 目标函数接收 Temp 的左值引用。绑定成功后，外层 Functor 的调用者无需再提供参数。
void print( Temp &tmp) {
    std::cout << tmp.get_data() << tmp.get_str() << std::endl;
}
using Functor = std::function<void()>;
int main()
{
    // 容器预先创建一个空的回调槽位，内部作用域把生成的回调移动进去。
    std::vector<Functor> arry(1);
    {
        Temp tmp(99, "nihao");
        // std::ref 产生引用包装器。移动包装器或函数对象只转移回调的保存状态，不会复制 Temp 或延长它的生命周期。
        Functor f = std::bind(print, std::move(std::ref(tmp)));
        arry[0] =  std::move(f);
    }
    // 此处 tmp 已离开作用域。调用仍尝试经由悬空引用访问原对象，属于未定义行为。
    arry[0]();
    return 0;
}