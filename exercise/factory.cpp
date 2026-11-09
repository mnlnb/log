// 【模块导读 · 三种创建对象的组织方式】
// 本例把产品创建与业务使用拆开。main 依次演示按名字选产品、按工厂类型选产品、先选工厂类别再按名字选产品。
// 可以先看最后的 main，对照三段调用找回各工厂；再看 Fruit 和 Animal，就能发现调用端始终通过统一产品接口 show 或 voice 使用对象。
// 日志库的 SinkFactory 也把构造集中起来，不过调用时用模板类型指定输出目标。本练习用于理解为什么要封装创建。
#include <iostream>
#include <string>
#include <memory>

// Fruit 定义所有水果都要提供的 show。工厂返回 Fruit 指针后，调用方无须知道具体对象是苹果还是香蕉。
class Fruit {
    public:
        Fruit(){}
        virtual void show() = 0;
};
// Apple 与 Banana 各自实现 show，动态派发让同一个 Fruit 接口输出不同结果。
class Apple : public Fruit {
    public:
        Apple() {}
        virtual void show() {
            std::cout << "我是一个苹果" << std::endl;
        }
    private:
        std::string _color;
};
class Banana : public Fruit {
    public:
        Banana() {}
        virtual void show() {
            std::cout << "我是一个香蕉" << std::endl;
        }
};
//简单工厂模式：通过参数控制可以生产任何产品
//  优点：简单粗暴，直观易懂。使用一个工厂生产同一等级结构下的任意产品
//  缺点：
//      1. 所有东西生产在一起，产品太多会导致代码量庞大
//      2. 开闭原则遵循(开放拓展，关闭修改)的不是太好，要新增产品就必须修改工厂方法。
// 简单工厂只有一个入口。由一个字符串键决定创建分支，代码集中，新增产品时也需要扩展这张分支表。
class FruitFactory1 {
    public:
        // 已知名称立即构造具体类型并隐式转为 Fruit 共享指针；未知名称走空共享指针返回，调用方应判断再调用。
        static std::shared_ptr<Fruit> create(const std::string &name) {
            if (name == "苹果") {
                return std::make_shared<Apple>();
            }else if(name == "香蕉") {
                return std::make_shared<Banana>();
            }
            return std::shared_ptr<Fruit>();
        }
};

//工厂方法：定义一个创建对象的接口，但是由子类来决定创建哪种对象，使用多个工厂分别生产指定的固定产品
//  优点：  
//      1. 减轻了工厂类的负担，将某类产品的生产交给指定的工厂来进行
//      2. 开闭原则遵循较好，添加新产品只需要新增产品的工厂即可，不需要修改原先的工厂类
//  缺点：对于某种可以形成一组产品族的情况处理较为复杂,需要创建大量的工厂类.
// 工厂方法把“选什么产品”落实到工厂派生类型。通过 FruitFactory2 接口调用 create，得到该工厂负责的产品。
class FruitFactory2 {
    public:
        virtual std::shared_ptr<Fruit> create() = 0;
};
// AppleFactory 固定创建 Apple。BananaFactory 固定创建 Banana；添加新工厂可沿用 FruitFactory2 的调用接口。
class AppleFactory : public FruitFactory2 {
    public:
        virtual std::shared_ptr<Fruit> create() {
            return std::make_shared<Apple>();
        }
};
class BananaFactory : public FruitFactory2 {
    public:
        virtual std::shared_ptr<Fruit> create() {
            return std::make_shared<Banana>();
        }
};

//抽象工厂：围绕一个超级工厂创建其他工厂。每个生成的工厂按照工厂模式提供对象。
//  思想：将工厂抽象成两层，抽象工厂 & 具体简单工厂子类， 在工厂子类种生产不同类型的子产品

// Animal 为第二类产品定义 voice。示例中的 Lamp 和 Dog 输出不同叫声，作为区分具体动物的可观察结果。
class Animal {
    public:
        virtual void voice() = 0;
};
class Lamp: public Animal {
    public:
        void voice() { std::cout << "咩咩咩\n"; }
};
class Dog: public Animal {
    public:
        void voice() { std::cout << "汪汪汪\n"; }
};

// Factory 声明水果和动物两类入口，让调用者先统一接收工厂，再调用对应的产品创建方法。
class Factory {
    public:
        virtual std::shared_ptr<Fruit> getFruit(const std::string &name) = 0;
        virtual std::shared_ptr<Animal> getAnimal(const std::string &name) = 0;
};

// FruitFactory 根据名称创建水果。它的动物入口返回空共享指针，所以先选对类别再调用很重要。
class FruitFactory : public Factory {
    public:
        virtual std::shared_ptr<Animal> getAnimal(const std::string &name) {
            return std::shared_ptr<Animal>();
        }
        // 水果名称解析与简单工厂的行为一致，"苹果" 和 "香蕉" 各自映射到对应产品，未知名称走空指针分支。
        virtual std::shared_ptr<Fruit> getFruit(const std::string &name) {
            if (name == "苹果") {
                return std::make_shared<Apple>();
            }else if(name == "香蕉") {
                return std::make_shared<Banana>();
            }
            return std::shared_ptr<Fruit>();
        }
};

// AnimalFactory 对水果入口返回空指针。动物入口识别 "小羊" 和 "小狗"，其他名字返回空指针。
class AnimalFactory : public Factory {
    protected:
    public:
        virtual std::shared_ptr<Fruit> getFruit(const std::string &name) {
            return std::shared_ptr<Fruit>();
        }
        virtual std::shared_ptr<Animal> getAnimal(const std::string &name) {
            if (name == "小羊") {
                return std::make_shared<Lamp>();
            }else if(name == "小狗") {
                return std::make_shared<Dog>();
            }
            return std::shared_ptr<Animal>();
        }
};

// FactoryProducer 是工厂入口的上一层。传入 "动物" 创建 AnimalFactory，当前其余类别值都走 FruitFactory 分支。
class FactoryProducer {
    public:
        static std::shared_ptr<Factory> getFactory(const std::string &name) {
            if (name == "动物") {
                return std::make_shared<AnimalFactory>();
            }else {
                return std::make_shared<FruitFactory>();
            }
        }
};

// main 的第一组用一个字符串选产品，第二组替换具体工厂，第三组先拿工厂再创建水果和动物。
int main()
{
    std::shared_ptr<Fruit> fruit1 = FruitFactory1::create("苹果");
    fruit1->show();
    fruit1 = FruitFactory1::create("香蕉");
    fruit1->show();

    // reset 把工厂指针换成 BananaFactory 前后，调用代码仍是 factory->create()，对象类型由新工厂的实现决定。
    std::shared_ptr<FruitFactory2> factory(new AppleFactory());
    fruit1 = factory->create();
    fruit1->show();
    factory.reset(new BananaFactory());
    fruit1 = factory->create();
    fruit1->show();

    // 先按类别取得水果工厂，再按具体名字拿水果。下面的动物组同理，用 voice 观察具体产品。
    std::shared_ptr<Factory> fruit_factory = FactoryProducer::getFactory("水果");
    std::shared_ptr<Fruit> fruit = fruit_factory->getFruit("苹果");
    fruit->show();
    fruit = fruit_factory->getFruit("香蕉");
    fruit->show();

    std::shared_ptr<Factory> animal_factory = FactoryProducer::getFactory("动物");
    std::shared_ptr<Animal> animal = animal_factory->getAnimal("小羊");
    animal->voice();
    animal = animal_factory->getAnimal("小狗");
    animal->voice();
    return 0;
}