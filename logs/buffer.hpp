// 异步日志的连续字节缓冲区。写入端追加已格式化的文本，消费端按字节区间批量提交给输出目标。
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
// 以字节为单位。初始容量为 1 MiB，扩容策略以 10 MiB 为界。
#define BUFFER_DEFAULT_SIZE (1*1024*1024)
#define BUFFER_INCREMENT_SIZE (1*1024*1024)
#define BUFFER_THRESHOLD_SIZE (10*1024*1024)
// 有效数据是 [_reader_idx, _writer_idx)。尾部可写空间从 _writer_idx 开始，已读前缀须等 reset 才能复用。
class Buffer {
    public:
        Buffer(): _reader_idx(0), _writer_idx(0), _v(BUFFER_DEFAULT_SIZE){}
        bool empty() { return _reader_idx == _writer_idx; }
        size_t readAbleSize() { return _writer_idx - _reader_idx; }
        size_t writeAbleSize() { return _v.size() - _writer_idx; }
        // 复位只移动两个游标，保留已分配的存储，也不清空底层字节。
        void reset() { _reader_idx = _writer_idx = 0; }
        // 连同存储和游标一起交换，后台线程可以接管整批数据，避免逐条复制。
        void swap(Buffer &buf)  {
            _v.swap(buf._v);
            std::swap(_reader_idx, buf._reader_idx);
            std::swap(_writer_idx, buf._writer_idx);
        }
        // 调用方负责协调并发，并保证输入指针有至少 len 个可读字节。这里保存原始字节，不附加字符串终止符。
        void push(const char *data, size_t len) { 
            // 启用断言时，尾部空间不足会先触发断言。正常的异步生产者会在调用前等待空间。
            assert(len <= writeAbleSize());
            ensureEnoughSpace(len);
            std::copy(data, data+len, &_v[_writer_idx]);
            _writer_idx += len;
        }
        // 消费方应同时使用 begin() 和 readAbleSize()。返回指针指向内部存储，扩容或交换后需重新获取。
        const char*begin() { return &_v[_reader_idx]; }
        // 推进读游标。len 应小于等于当前可读长度，整个批次处理后由消费端显式调用 reset。
        void pop(size_t len) { 
            _reader_idx += len; 
            assert(_reader_idx <= _writer_idx);
        }
    protected:
        // 可用空间不足时增大 vector 的逻辑长度。阈值以下使用倍增，达到阈值后加固定增量，两种情况都额外加上本次请求的长度。
        void ensureEnoughSpace(size_t len) {
            if (len <= writeAbleSize()) return;
            /*每次增大1M大小*/
            size_t new_capacity;
            if (_v.size() < BUFFER_THRESHOLD_SIZE)
                new_capacity = _v.size() * 2 + len;
            else
                new_capacity = _v.size() + BUFFER_INCREMENT_SIZE + len;
            _v.resize(new_capacity);
        }
    private:
        size_t _reader_idx;
        size_t _writer_idx;
        std::vector<char> _v;
};
}