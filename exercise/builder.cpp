// 【模块导读 · 建造者模式练习】
// 用装配电脑说明四个角色。Computer 保存部件，MacBook 决定自己的系统，Builder 约定装配步骤，Director 统一步骤顺序。
// 读 main 时先找具体建造者，再跟 construct 看装配，最后回到 build 看拿到的产品。Logger::Builder 也用分步收集配置的思路。
// 本例的具体建造者只持有一个产品对象，调用 build 只是取这个实例，后续继续装配会改动同一对象。
#include <iostream>
#include <memory>

/*抽象电脑类*/
// 抽象产品保存主板、显示器和系统。前两项允许外部传值，系统由派生产品实现 setOs 决定。
class Computer {
    public:
        using ptr = std::shared_ptr<Computer>;
        Computer() {}
        // 这两个 setter 只给字段赋值。装配者通过它们保存调用方指定的主板名和显示器名。
        void setBoard(const std::string &board) {_board = board;}
        void setDisplay(const std::string &display) {_display = display;}
        // 纯虚函数定义了所有具体电脑都要提供的系统设置步骤。Computer 保留字段，具体产品选择填入的值。
        virtual void setOs() = 0;
        // toString 按固定顺序拼接三个部件，返回带换行的文本，让调用端能检查装配结果。
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
// MacBook 继承通用部件，覆盖 setOs 把固定文本写入 _os。主板和显示器仍由装配步骤传入。
class MacBook : public Computer {
    public:
        using ptr = std::shared_ptr<MacBook>;
        MacBook() {}
        virtual void setOs() {
            _os = "Max Os X12";
        }
};

/*抽象建造者类：包含创建一个产品对象的各个部件的抽象接口*/
// Builder 的前三个方法负责装配部件，build 返回 Computer 共享指针。调用方只依赖接口，也能接收派生产品。
class Builder {
    public:
        using ptr = std::shared_ptr<Builder>;
        virtual void buildBoard(const std::string &board) = 0;
        virtual void buildDisplay(const std::string &display) = 0;
        virtual void buildOs() = 0;
        virtual Computer::ptr build() = 0;
};

/*具体产品的具体建造者类：实现抽象接口，构建和组装各个部件*/
// MackBookBuilder 构造时就持有一个 MacBook，之后每个部件方法都把输入转交给这个产品。
class MackBookBuilder : public Builder {
    public:
        using ptr = std::shared_ptr<MackBookBuilder>;
        MackBookBuilder(): _computer(new MacBook()) {}
        // 主板步骤转交 setBoard；显示器步骤转交 setDisplay；系统步骤调用虚方法，落到 MacBook::setOs。
        virtual void buildBoard(const std::string &board) {
            _computer->setBoard(board);
        }
        virtual void buildDisplay(const std::string &display) {
            _computer->setDisplay(display);
        }
        virtual void buildOs() {
            _computer->setOs();
        }
        // 返回共享指针意味着产品可由调用者和建造者共同持有。当前代码不在返回后新建下一件产品。
        virtual Computer::ptr build() {
            return _computer;
        }
    private:
        Computer::ptr _computer;
};

/*指挥者类，提供给调用者使用，通过指挥者来获取产品*/
// Director 集中规定顺序。换一个 Builder 实现仍可沿用 construct 的三步流程。
class Director {
    public:
        // 构造时成员 shared_ptr 接管传入的裸指针。传入对象不能再由另一个独立所有者负责删除。
        Director(Builder* builder):_builder(builder){}
        // construct 用调用方给出的两个部件名执行装配，最后让具体产品确定系统。
        void construct(const std::string &board, const std::string &display) {
            _builder->buildBoard(board);
            _builder->buildDisplay(display);
            _builder->buildOs();
        }
    private:
        Builder::ptr _builder;
};

// main 先选具体建造者，再让指挥者装配，取回产品后打印三个字段。
int main()
{
    Builder *buidler = new MackBookBuilder();
    // 指挥者由 unique_ptr 管理，而它内部管理建造者。下一行的裸指针仍可在指挥者存活期间用于获取产品。
    std::unique_ptr<Director> pd(new Director(buidler));
    pd->construct("英特尔主板", "VOC显示器");
    Computer::ptr computer = buidler->build();
    std::cout << computer->toString();
    return 0;
}