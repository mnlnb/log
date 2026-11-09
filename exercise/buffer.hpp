// 固定容量缓冲区练习。它独立于 logs/ 下的异步日志缓冲实现，用读写游标管理连续字节。
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <cassert>

// 默认容量为 10 MiB。构造时分配整段空间，也可由调用方传入其他容量。
#define MAX_BUFFER_SIZE 10*1024*1024
class Buffer {
    private:
        // 有效数据为 [_reader_idx, _writer_idx)。写游标之后是可直接使用的连续空间。
        size_t _reader_idx;
        size_t _writer_idx;
        size_t _capacity;
        std::vector<char> _v;
    public:
        Buffer(size_t capacity = MAX_BUFFER_SIZE):
            _reader_idx(0), _writer_idx(0), 
            _capacity(capacity), _v(_capacity){}
        // full 检查尾部是否已用完，即使头部已有已读空间也可能为真。读空后才能复用整个缓冲。
        bool full() { return _writer_idx == _capacity; }
        bool empty() { return _reader_idx == _writer_idx; }
        size_t readAbleSize() { return _writer_idx - _reader_idx; }
        size_t writeAbleSize() { return _capacity - _writer_idx; }
        void reset() { _reader_idx = _writer_idx = 0; }
        // 交换存储时同步交换容量和两个游标，确保它们始终描述同一段底层内存。
        void swap(Buffer &buf)  {
            _v.swap(buf._v);
            std::swap(_reader_idx, buf._reader_idx);
            std::swap(_writer_idx, buf._writer_idx);
            std::swap(_capacity, buf._capacity);
        }
        // 调用方须保证输入长度不超过尾部剩余空间，写入按长度复制原始字节并推进写游标。
        void push(const char *data, size_t len) { 
            assert(len <= writeAbleSize());
            std::copy(data, data+len, &_v[_writer_idx]);
            _writer_idx += len;
        }
        // begin 返回内部地址，读取时应同时参考可读长度，不能将它直接当成以空字符结尾的字符串。
        const char*begin() { return &_v[_reader_idx]; }
        // 弹出仅推进游标。调用方仍须保证 len 不大于可读长度，当前断言只检查游标未超过总容量。
        void pop(size_t len) { 
            _reader_idx += len; 
            assert(_reader_idx<=_capacity);
            // 恰好读空时把两个游标归零，复用已分配的空间。
            if (empty()) reset();
        }
};