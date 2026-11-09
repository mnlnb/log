// 建造者模式练习。产品保存部件，建造者提供逐步设置接口，指挥者封装一套装配顺序。
#include <iostream>
#include <memory>

/*抽象电脑类*/
// 产品基类保存主板、显示器和系统信息。具体产品实现系统设置，toString 汇总部件便于观察结果。
class Computer {
    public:
        using ptr = std::shared_ptr<Computer>;
        Computer() {}
        void setBoard(const std::string &board) {_board = board;}
        void setDisplay(const std::string &display) {_display = display;}
        virtual void setOs() = 0;
        std::string toString() {
            std::string computer = "Computer:{\n";
            computer += "\tboard=" + _board + ",\n"; 
            computer += "\tdisplay=" + _display + ",\n"; 
            computer += "\tOs=" + _os + ",\n"; 
            computer += "}\n";
            return computer;
        }
    protected:
        std::string _board;
        std::string _display;
        std::string _os;
};

/*具体产品类*/
// 具体产品选择固定的操作系统字符串，其余部件由外部装配步骤赋值。
class MacBook : public Computer {
    public:
        using ptr = std::shared_ptr<MacBook>;
        MacBook() {}
        virtual void setOs() {
            _os = "Max Os X12";
        }
};

/*抽象建造者类：包含创建一个产品对象的各个部件的抽象接口*/
// 抽象建造者定义部件步骤与返回产品的接口。调用端可以统一使用 Computer 指针接收产品。
class Builder {
    public:
        using ptr = std::shared_ptr<Builder>;
        virtual void buildBoard(const std::string &board) = 0;
        virtual void buildDisplay(const std::string &display) = 0;
        virtual void buildOs() = 0;
        virtual Computer::ptr build() = 0;
};

/*具体产品的具体建造者类：实现抽象接口，构建和组装各个部件*/
// 构造建造者时创建一个 MacBook。各装配步骤都写入同一实例，build 返回该实例的共享指针。
class MackBookBuilder : public Builder {
    public:
        using ptr = std::shared_ptr<MackBookBuilder>;
        MackBookBuilder(): _computer(new MacBook()) {}
        virtual void buildBoard(const std::string &board) {
            _computer->setBoard(board);
        }
        virtual void buildDisplay(const std::string &display) {
            _computer->setDisplay(display);
        }
        virtual void buildOs() {
            _computer->setOs();
        }
        // build 不创建下一件产品或重置当前状态。后续装配调用会继续修改这个实例。
        virtual Computer::ptr build() {
            return _computer;
        }
    private:
        Computer::ptr _computer;
};

/*指挥者类，提供给调用者使用，通过指挥者来获取产品*/
// 指挥者持有建造者并规定先设置主板、再设置显示器、最后设置系统的顺序。
class Director {
    public:
        // 传入的裸指针被成员 shared_ptr 接管。调用方需保证它尚未由另一个独立的智能指针接管。
        Director(Builder* builder):_builder(builder){}
        void construct(const std::string &board, const std::string &display) {
            _builder->buildBoard(board);
            _builder->buildDisplay(display);
            _builder->buildOs();
        }
    private:
        Builder::ptr _builder;
};

// 演示选择具体建造者、执行装配和获取结果的过程。借用的建造者指针在指挥者存活时使用。
int main()
{
    Builder *buidler = new MackBookBuilder();
    std::unique_ptr<Director> pd(new Director(buidler));
    pd->construct("英特尔主板", "VOC显示器");
    Computer::ptr computer = buidler->build();
    std::cout << computer->toString();
    return 0;
}