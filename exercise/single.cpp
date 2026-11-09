// 函数局部静态对象的单例练习。调用端通过类提供的静态入口取得共享实例。
#include <iostream>
#include <vector>
#include <list>

class Single {
    private:
        // 构造函数私有，类外不能直接默认构造。构造时打印提示，便于观察首次初始化。
        Single(){
            std::cout << "single!!\n";
        }
    public:
        // 第一次调用时初始化 eton，之后返回同一地址。C++11 保证此局部静态对象的初始化由运行时同步。
        static Single *getInstance() {
            static Single eton;
            return &eton;
        }
        // 实例方法提供可观察的调用结果。返回指针借用静态实例，调用方不应释放它。
        void print() {
            std::cout << "xxxx\n";
        }
};

// 入口先取得实例，再调用它的方法，展示访问路径。
int main() {
    Single::getInstance()->print();
    return 0;
}
