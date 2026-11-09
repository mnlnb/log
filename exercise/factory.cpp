// 在同一组水果和动物产品上展示简单工厂、工厂方法以及按类别选择工厂的两层结构。
#include <iostream>
#include <string>
#include <memory>

// Fruit 定义统一的展示接口。Apple 和 Banana 通过各自实现输出产品身份。
class Fruit {
    public:
        Fruit(){}
        virtual void show() = 0;
};
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
class FruitFactory1 {
    public:
        // 简单工厂在一次调用中按名称选择具体水果。未识别的名称返回空共享指针。
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
// 工厂方法把创建操作交给具体工厂。调用端可通过统一基类切换 AppleFactory 与 BananaFactory。
class FruitFactory2 {
    public:
        virtual std::shared_ptr<Fruit> create() = 0;
};
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

// 第二类产品使用 Animal 接口，具体动物通过 voice 表现各自行为。
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

// 两层工厂示例的基类同时声明水果与动物入口，返回的产品分别用各自基类指针表示。
class Factory {
    public:
        virtual std::shared_ptr<Fruit> getFruit(const std::string &name) = 0;
        virtual std::shared_ptr<Animal> getAnimal(const std::string &name) = 0;
};

// 水果工厂仅处理水果名称，调用其动物入口时返回空指针。
class FruitFactory : public Factory {
    public:
        virtual std::shared_ptr<Animal> getAnimal(const std::string &name) {
            return std::shared_ptr<Animal>();
        }
        virtual std::shared_ptr<Fruit> getFruit(const std::string &name) {
            if (name == "苹果") {
                return std::make_shared<Apple>();
            }else if(name == "香蕉") {
                return std::make_shared<Banana>();
            }
            return std::shared_ptr<Fruit>();
        }
};

// 动物工厂仅处理动物名称，调用其水果入口时返回空指针。未知动物名称也返回空指针。
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

// 先按类别选择具体工厂，再按产品名创建对象。当前仅识别动物类别，其他输入都走水果工厂分支。
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

// 入口依次运行三种创建方式，测试只使用已知名称，因此直接调用返回对象的方法。
int main()
{
    std::shared_ptr<Fruit> fruit1 = FruitFactory1::create("苹果");
    fruit1->show();
    fruit1 = FruitFactory1::create("香蕉");
    fruit1->show();

    std::shared_ptr<FruitFactory2> factory(new AppleFactory());
    fruit1 = factory->create();
    fruit1->show();
    factory.reset(new BananaFactory());
    fruit1 = factory->create();
    fruit1->show();

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