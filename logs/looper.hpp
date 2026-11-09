// 【模块导读 · 异步日志的生产者和消费者】
// 生产者是业务线程。AsyncLogger 已在业务线程把记录变成一行文本，再调用 push 将文本追加到共享写缓冲。
// 消费者是工作线程。它把共享写缓冲与自己的消费缓冲交换，然后在锁外调用 AsyncLogger::backendLogIt，批量写到各个 Sink。
// 两块 Buffer 可以想成两个托盘：业务线程往一个放，后台拿另一个去输出；交换时只交接托盘，不在锁内一条条写文件。
// 两个条件变量分别处理“有数据可取”和“有空间可写”。读本文件时先看 push，再看 worker_loop，最后看 stop。
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
    // AsyncLooper 保存线程、回调、运行标志和两块缓冲。共享写缓冲的读写与交接都由同一个互斥锁保护。
    class AsyncLooper {
        public:
            // 回调收到的是当前消费批次的 Buffer 引用。回调可以同步读取，之后工作线程会复位并重复使用这块缓冲。
            using Functor = std::function<void(Buffer &buffer)>;
            using ptr = std::shared_ptr<AsyncLooper>;
            // 构造函数保存回调并启动 worker_loop。线程成员排在最后声明，它启动时前面的成员已经按声明顺序初始化完成。
            AsyncLooper(const Functor &cb): _running(true), _looper_callback(cb),
                _thread(std::thread(&AsyncLooper::worker_loop, this)) {
            }
            // 析构负责停止工作线程。AsyncLogger 持有 Looper，而回调依赖 AsyncLogger 的输出目标，因此 join 保证回调结束后才完成清理。
            ~AsyncLooper() { stop(); }
            // stop 将运行标志置为 false，通知可能正等待数据的工作线程，再等待线程退出。
            // 当前实现假设 stop 只负责一次 join；如果手动 stop 后又由析构调用，第二次 join 没有可连接线程。
            void stop(){ 
                _running = false; 
                _pop_cond.notify_all();
                _thread.join();
            }
            // push 在入口看到 _running 为 false 就放弃消息。通过检查后，进入锁保护的生产者流程。
            void push(const std::string &msg){
                if (_running == false) return;
                {
                    // 这里用 unique_lock，条件变量可以在等待时暂时释放它，使消费者有机会拿锁交换缓冲。
                    std::unique_lock<std::mutex> lock(_mutex);
                    // 等待谓词检查当前写缓冲的尾部是否能放下整条消息。被唤醒时会重新检查，空间仍不足就继续等。
                    // 因此单条消息大于缓冲本身能提供的空间时，即使交换出一块空缓冲也可能一直等；这条生产路径没有主动拆分大消息。
                    _push_cond.wait(lock, [&]{ return _tasks_push.writeAbleSize() >= msg.size(); });
                    // 在锁内把整条消息复制到写缓冲。这样不同生产线程的追加不会把一条消息的字节互相穿插。
                    _tasks_push.push(msg.c_str(), msg.size());
                }
                // 大括号结束时先释放锁，再通知消费者有数据可取。消费者成功拿锁后就能看到这一批新数据。
                _pop_cond.notify_all();
            }
        private:
            // 工作线程循环取批次。每一轮先处理锁内状态，再在锁外输出，最后复位本轮的消费缓冲。
            void worker_loop(){
                while(1){
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        // 停止且写缓冲为空时退出；停止时已有待写数据则进入下面的交接，让后台有机会写完已排队的批次。
                        if (_running == false && _tasks_push.empty()) { return; }
                        // 消费者在“有字节”或“正在停止”时离开等待。停止标志让空闲中的线程也能醒来退出。
                        _pop_cond.wait(lock, [&]{ return !_tasks_push.empty() || !_running; });
                        // 交换之后，原本由生产者填好的存储和游标移到 _tasks_pop。共享 _tasks_push 则接管上一轮已复位的缓冲。
                        _tasks_push.swap(_tasks_pop);
                    }
                    // 通知生产者重新检查空间。回调和 reset 都在锁外执行，文件 I/O 再慢也不会把共享锁一直占住。
                    _push_cond.notify_all();
                    // 一次回调拿到完整批次，字节里已包含 Formatter 生成的正文和换行。这里不重建 LogMsg 或逐条解析。
                    _looper_callback(_tasks_pop);
                    // 回调返回后才复位消费缓冲。下一次交换会把这块可复用的空间交还给生产者。
                    _tasks_pop.reset();
                }
                return;
            }
        private:
            Functor _looper_callback;
        private:
            // _mutex 保护共享缓冲；_running 是跨线程读写的原子标志。条件变量各自的等待谓词说明何时能继续。
            std::mutex _mutex;
            std::atomic<bool> _running;
            // _push_cond 用来唤醒等空间的生产者，_pop_cond 用来唤醒等消息的消费者，两者的名称指向各自等待的操作。
            std::condition_variable _push_cond;
            std::condition_variable _pop_cond;
            Buffer _tasks_push;
            Buffer _tasks_pop;
            std::thread _thread;
    };
}
#endif