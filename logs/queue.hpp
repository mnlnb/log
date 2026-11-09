/*
    实现环形队列和阻塞队列
*/

// 这是队列接口的练习草稿。下方只列出了数据成员和声明，实际异步日志路径由 AsyncLooper 与 Buffer 承担。
#ifndef __M_QUEUE_H__
#define __M_QUEUE_H__
#include <iostream>
#include <vector>

namespace bitlog{
    // 环形队列预留固定容量的存储，读写位置分别用于定位队头和下次插入点。位置回绕和满空判定尚未实现。
    template <typename T>
    class ring_queue {
        private:
            size_t _capacity;
            size_t _read_pos;
            size_t _write_pos;
            std::vector<T> _v;
        // 这些声明描述入队、访问队头和出队操作。当前构造声明使用 RingQueue 拼写，与类名 ring_queue 不一致，两个 front 也没有用成员 const 区分重载。
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

    // 阻塞队列计划在环形队列外加入互斥锁和条件变量。等待谓词、超时语义及返回引用的有效期仍需由实现定义。
    template<typename T>
    class block_queue {
        private:
            ring_queue<T> _q;
            // 按成员命名，_cond_push 用于通知等待空间的生产者，_cond_pop 用于通知等待元素的消费者。
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