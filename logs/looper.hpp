// 异步输出的生产者与消费者协调层。调用线程追加字节，工作线程交换缓冲区后批量调用输出回调。
#ifndef __M_LOOP_H__
#define __M_LOOP_H__
#include "util.hpp"
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>
#include "buffer.hpp"

namespace bitlog{
    // 两个缓冲交替充当写入端和消费端。互斥锁只保护共享状态和交接，后台输出不持有该锁。
    class AsyncLooper {
        public:
            using Functor = std::function<void(Buffer &buffer)>;
            using ptr = std::shared_ptr<AsyncLooper>;
            // 构造时保存回调并启动工作线程。线程成员最后声明，启动时其余成员已按声明顺序完成初始化。
            AsyncLooper(const Functor &cb): _running(true), _looper_callback(cb),
                _thread(std::thread(&AsyncLooper::worker_loop, this)) {
            }
            ~AsyncLooper() { stop(); }
            // 停止时标记运行结束，唤醒可能在等待数据的消费者，再用 join 等待线程退出。正常用法由析构调用一次 stop。
            void stop(){ 
                _running = false; 
                _pop_cond.notify_all();
                _thread.join();
            }
            // 运行标志为假时直接丢弃本次提交。通过检查后，生产者在互斥锁保护下等待写缓冲可用。
            void push(const std::string &msg){
                if (_running == false) return;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    // 谓词要求剩余尾部空间能够容纳整条消息。单条消息大于缓冲区最大可用尾部时，当前逻辑可能一直等待。
                    _push_cond.wait(lock, [&]{ return _tasks_push.writeAbleSize() >= msg.size(); });
                    _tasks_push.push(msg.c_str(), msg.size());
                }
                // 写入完成并释放互斥锁后通知消费者，消费者取得锁时即可看到已追加的数据。
                _pop_cond.notify_all();
            }
        private:
            // 工作循环处理待写批次。停止且共享写缓冲为空时退出，否则继续完成当前交接。
            void worker_loop(){
                while(1){
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        if (_running == false && _tasks_push.empty()) { return; }
                        // 等待谓词同时观察是否有数据和是否停止，处理正常唤醒和虚假唤醒。
                        _pop_cond.wait(lock, [&]{ return !_tasks_push.empty() || !_running; });
                        // 交换底层存储及读写游标。此后工作线程独占消费缓冲，生产者可继续写入新的共享缓冲。
                        _tasks_push.swap(_tasks_pop);
                    }
                    // 释放锁后先通知等待空间的生产者，再把消费批次交给回调。复位保留已分配存储以供下次交换。
                    _push_cond.notify_all();
                    _looper_callback(_tasks_pop);
                    _tasks_pop.reset();
                }
                return;
            }
        private:
            Functor _looper_callback;
        private:
            std::mutex _mutex;
            std::atomic<bool> _running;
            // _push_cond 面向生产者，_pop_cond 面向消费者。各自的等待谓词由对应缓冲状态决定。
            std::condition_variable _push_cond;
            std::condition_variable _pop_cond;
            Buffer _tasks_push;
            Buffer _tasks_pop;
            std::thread _thread;
    };
}
#endif