// 【模块导读 · 队列接口草稿】
// 这一文件用于练习环形队列与阻塞队列的分层。内层考虑“元素放在哪里”，外层考虑“多线程何时能放或取”。
// 正式的异步日志路径走 looper.hpp 和 buffer.hpp，本头文件只声明队列结构，没有给出方法体，也不在该路径上。
// 可先读 ring_queue 的状态，再读 block_queue 的锁和条件变量。遇到声明缺口时按草稿理解。
/*
    实现环形队列和阻塞队列
*/

// 头文件保护宏避免同一个翻译单元重复包含定义；它不会补齐下方尚未实现的接口。
#ifndef __M_QUEUE_H__
#define __M_QUEUE_H__
#include <iostream>
#include <vector>

namespace bitlog{
    // 环形队列的设想是固定槽位用完后回到起点。读位置表示队头，写位置表示下一次放入的位置。
    template <typename T>
    class ring_queue {
        private:
            // 仅保存容量和两个位置时，还需约定满与空如何区分。当前声明里没有给出计数、留空槽或其他具体策略。
            size_t _capacity;
            size_t _read_pos;
            size_t _write_pos;
            std::vector<T> _v;
        // 这里列出入队、查看队头和移除队头三个动作。常见用法先 front 看元素，再 pop 移除；该草稿未定义空队列时的行为。
        public:
            /*explicit 防止单参数构造函数发生隐式类型转换*/
            /*避免： RingQueue r = 15; 这种写法*/
            // 源码当前把构造声明写成 RingQueue，与类名 ring_queue 不一致。两个 front 也只在返回类型上不同，C++ 不能仅凭返回类型重载。
            explicit RingQueue(size_t capacity);
            // push 声明接收 const 右值引用；真正是否可移动取决于后续实现和元素类型。这里没有移动或复制元素的代码。
            void push(const T &&item);
            const T& front();
            T& front();
            void pop();
            bool empty();
            bool full();
    };

    // 阻塞队列把环形队列包装进同步层。预期队满时生产者等待空位，队空时消费者等待数据。
    template<typename T>
    class block_queue {
        private:
            ring_queue<T> _q;
            // 互斥锁保护队列状态。_cond_push 和 _cond_pop 预留给两类等待者，当前尚无等待谓词和通知代码。
            std::mutex _mutex;
            std::condition_variable _cond_push;
            std::condition_variable _cond_pop;
        public:
            // 构造、入队、带 duration 参数的取出与 size 都只有声明。超时之后返回什么、取出的引用何时失效，需要实现明确。
            block_queue(size_t capacity);
            void enqueue(T &&item);
            T& dequeue(std::chrono::milliseconds duration = -1);
            size_t size();
    };
}
#endif