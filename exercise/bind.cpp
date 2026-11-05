#include <iostream>
#include <string>
#include <functional>
#include <memory>
#include <vector>

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

void print( Temp &tmp) {
    std::cout << tmp.get_data() << tmp.get_str() << std::endl;
}
using Functor = std::function<void()>;
int main()
{
    std::vector<Functor> arry(1);
    {
        Temp tmp(99, "nihao");
        Functor f = std::bind(print, std::move(std::ref(tmp)));
        arry[0] =  std::move(f);
    }
    arry[0]();
    return 0;
}