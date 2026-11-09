// 【模块导读 · 异步日志的字节缓冲】
// AsyncLooper 用两个 Buffer 协调生产和消费。每次 push 把已经格式化的日志复制到一个 vector<char>，工作线程整批读取。
// Buffer 只管理一段存储和两个游标。线程间互斥与唤醒都由 looper.hpp 负责，输出目标则只接收地址和长度。
// 可把存储想成 [已读前缀][可读数据][可写尾部]。读游标指向可读数据开头，写游标指向可写尾部开头。
// 例如初始游标都为 0，写入 5 字节后有效区间是 [0, 5)，读掉 2 字节后变为 [2, 5)，复位后两个游标再回到 0。
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <cassert>
namespace bitlog{
// 这些常量以字节计。新缓冲分配 1 MiB，通用扩容函数以 10 MiB 为界切换增长策略。
#define BUFFER_DEFAULT_SIZE (1*1024*1024)
#define BUFFER_INCREMENT_SIZE (1*1024*1024)
#define BUFFER_THRESHOLD_SIZE (10*1024*1024)
// 类的核心约束是 0 <= 读游标 <= 写游标 <= 存储长度。写入、消费和交换都必须维持这三个量的关系。
class Buffer {
    public:
        // 构造时 vector 的 size 就是这段缓冲可用的字节数，两个游标相等意味着目前没有日志可读。
        Buffer(): _reader_idx(0), _writer_idx(0), _v(BUFFER_DEFAULT_SIZE){}
        // 可读长度是写游标减读游标。可写长度是存储长度减写游标，只计算尾部，已读前缀不会马上回收。
        bool empty() { return _reader_idx == _writer_idx; }
        size_t readAbleSize() { return _writer_idx - _reader_idx; }
        size_t writeAbleSize() { return _v.size() - _writer_idx; }
        // reset 把两游标归零，表示之前的字节全部作废。存储仍保留，下次写入可以直接覆盖。
        void reset() { _reader_idx = _writer_idx = 0; }
        // 交换 vector 的内部存储和两个游标，确保游标跟随其对应的数据；这就是 Looper 交接整批日志时无需再次复制的原因。
        void swap(Buffer &buf)  {
            _v.swap(buf._v);
            std::swap(_reader_idx, buf._reader_idx);
            std::swap(_writer_idx, buf._writer_idx);
        }
        // push 的参数是一段指针和明确长度。文本里的换行由 Formatter 决定，Buffer 不插入分隔符或空字符。
        void push(const char *data, size_t len) { 
            // 当前先断言空间足够，再调用扩容函数。启用断言时，空间不足会在扩容之前失败；生产者在 Looper 里会先等足够空间。
            assert(len <= writeAbleSize());
            ensureEnoughSpace(len);
            // 将输入字节复制到写游标处，随后把游标推进 len。消费者因此可以一次拿到多条相连的日志。
            std::copy(data, data+len, &_v[_writer_idx]);
            _writer_idx += len;
        }
        // begin 指向当前读游标，须与 readAbleSize 配对使用。交换或扩容后应重新取地址。
        const char*begin() { return &_v[_reader_idx]; }
        // pop 只推进读游标，不搬动剩余字节。调用者必须保证 len 不超过可读长度；整个批次消费完后用 reset 复用空间。
        void pop(size_t len) { 
            _reader_idx += len; 
            assert(_reader_idx <= _writer_idx);
        }
    protected:
        // ensureEnoughSpace 在尾部够用时立即返回。尾部不足时，通过 resize 加大可写尾部，没有压缩或搬移已读前缀来复用空间的分支。
        void ensureEnoughSpace(size_t len) {
            if (len <= writeAbleSize()) return;
            // 下面的实际公式是：阈值以下取原大小两倍再加 len；达到阈值后取原大小加 1 MiB 再加 len。
            /*每次增大1M大小*/
            size_t new_capacity;
            if (_v.size() < BUFFER_THRESHOLD_SIZE)
                new_capacity = _v.size() * 2 + len;
            else
                new_capacity = _v.size() + BUFFER_INCREMENT_SIZE + len;
            _v.resize(new_capacity);
        }
    // _v 持有实际字节。两个下标只描述这段字节的使用情况，不能单靠 vector::empty 判断有没有日志。
    private:
        size_t _reader_idx;
        size_t _writer_idx;
        std::vector<char> _v;
};
}