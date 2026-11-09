// 【模块导读 · 固定大小的缓冲区练习】
// 练习重点是如何用两个下标管理一段连续内存。它独立于 logs/buffer.hpp，使用固定容量，读空时自动复位。
// 先读 push 看写下标怎么走，再读 pop 看读下标怎么走，最后对照 full、empty、swap 理解缓冲状态。
// 例如容量为 8，push 3 字节后有效区间是 [0, 3)；pop 2 后只剩 [2, 3)；再 pop 1 后读写下标都回到 0。
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <cassert>

// 默认值按字节计，10 * 1024 * 1024 即 10 MiB。构造时也可显式传入更小的容量做演示。
#define MAX_BUFFER_SIZE 10*1024*1024
class Buffer {
    private:
        // 读下标是下一次读取的起点，写下标是下一次追加的起点，_capacity 记录底层 vector 的固定长度。
        size_t _reader_idx;
        size_t _writer_idx;
        size_t _capacity;
        std::vector<char> _v;
    public:
        // 构造先将两个下标设为 0，再为 vector 分配 capacity 个字节。初始可读长度是 0，尾部可写长度等于容量。
        Buffer(size_t capacity = MAX_BUFFER_SIZE):
            _reader_idx(0), _writer_idx(0), 
            _capacity(capacity), _v(_capacity){}
        // full 只检查尾部是否用完。即使已经读过前缀，写下标碰到末端也会报告满，当前代码没有循环绕回前缀。
        bool full() { return _writer_idx == _capacity; }
        // empty 比较两个下标。可读长度是两者的差，可写长度是容量减写下标。
        bool empty() { return _reader_idx == _writer_idx; }
        size_t readAbleSize() { return _writer_idx - _reader_idx; }
        size_t writeAbleSize() { return _capacity - _writer_idx; }
        // reset 将两个下标归零，令所有旧字节不再属于有效区间。底层空间保留供下次写入。
        void reset() { _reader_idx = _writer_idx = 0; }
        // swap 交换底层 vector、容量和两个下标。容量与数据一并交换，适用于两个初始大小不同的 Buffer。
        void swap(Buffer &buf)  {
            _v.swap(buf._v);
            std::swap(_reader_idx, buf._reader_idx);
            std::swap(_writer_idx, buf._writer_idx);
            std::swap(_capacity, buf._capacity);
        }
        // push 接收指针及长度，调用者保证输入可读且尾部有足够空间。断言检查长度，然后复制并推进写下标。
        void push(const char *data, size_t len) { 
            assert(len <= writeAbleSize());
            std::copy(data, data+len, &_v[_writer_idx]);
            _writer_idx += len;
        }
        // begin 指向当前读下标处，读取须配合 readAbleSize，避免将这段字节误当作有结束符的 C 字符串。
        const char*begin() { return &_v[_reader_idx]; }
        // pop 仅推进读下标。调用者应保证 len 不超过可读长度；现有断言只检查是否超过总容量，无法捕获所有过量消费。
        void pop(size_t len) { 
            _reader_idx += len; 
            assert(_reader_idx<=_capacity);
            // 两个下标恰好相等时，已有数据已全部消费。此时复位即可重新利用整个固定缓冲区。
            if (empty()) reset();
        }
};