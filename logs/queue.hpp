/*
    实现环形队列和阻塞队列
*/

#ifndef __M_QUEUE_H__
#define __M_QUEUE_H__
#include <iostream>
#include <vector>

namespace bitlog{
    template <typename T>
    class ring_queue {
        private:
            size_t _capacity;
            size_t _read_pos;
            size_t _write_pos;
            std::vector<T> _v;
        public:
            /*explicit 防止单参数构造函数发生隐式类型转换*/
            /*避免： RingQueue r = 15; 这种写法*/
            explicit RingQueue(size_t capacity);
            void push(const T &&item);
            const T& front();
            T& front();
            void pop();
            bool empty();
            bool full();
    };

    template<typename T>
    class block_queue {
        private:
            ring_queue<T> _q;
            std::mutex _mutex;
            std::condition_variable _cond_push;
            std::condition_variable _cond_pop;
        public:
            block_queue(size_t capacity);
            void enqueue(T &&item);
            T& dequeue(std::chrono::milliseconds duration = -1);
            size_t size();
    };
}
#endif