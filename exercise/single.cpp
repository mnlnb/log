#include <iostream>
#include <vector>
#include <list>

class Single {
    private:
        Single(){
            std::cout << "single!!\n";
        }
    public:
        static Single *getInstance() {
            static Single eton;
            return &eton;
        }
        void print() {
            std::cout << "xxxx\n";
        }
};

int main() {
    Single::getInstance()->print();
    return 0;
}
