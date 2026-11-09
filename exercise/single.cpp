// 【模块导读 · 进程内只有一个共享入口】
// 这个文件演示函数局部静态对象的用法。第一次调用 getInstance 时构造对象，之后调用复用同一实例。
// 先看私有构造，再看 getInstance 中的 static，最后读 main 的调用。正式日志库的 loggerManager::getInstance 使用相同思路保存全局注册表。
#include <iostream>
#include <vector>
#include <list>

// 把创建入口收进类内部，让调用端先取实例再执行 print。此练习重点在访问路径与初始化时机。
class Single {
    private:
        // 构造函数私有，外部不能直接默认构造。构造函数打印提示，所以首次访问时会观察到额外输出。
        Single(){
            std::cout << "single!!\n";
        }
    public:
        // eton 是函数局部静态对象，首次走到其声明时初始化，生命周期持续到程序退出。
        // C++11 的初始化由运行时同步，因此并发首次进入也只初始化一次；这一点不等同于以后所有成员访问都自动加锁。
        static Single *getInstance() {
            static Single eton;
            // 返回地址指向静态对象，调用者只借用，不能 delete。若需要进一步禁止复制，还要在类定义中显式约束复制操作。
            return &eton;
        }
        // print 只输出一行演示文本，用来观察取得实例后可正常调用成员。
        void print() {
            std::cout << "xxxx\n";
        }
};

// main 先取实例再调用 print。构造提示出现在成员输出之前，可用来验证延迟初始化顺序。
int main() {
    Single::getInstance()->print();
    return 0;
}
