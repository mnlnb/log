#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <cassert>

#define MAX_BUFFER_SIZE 10*1024*1024
class Buffer {
    private:
        size_t _reader_idx;
        size_t _writer_idx;
        size_t _capacity;
        std::vector<char> _v;
    public:
        Buffer(size_t capacity = MAX_BUFFER_SIZE):
            _reader_idx(0), _writer_idx(0), 
            _capacity(capacity), _v(_capacity){}
        bool full() { return _writer_idx == _capacity; }
        bool empty() { return _reader_idx == _writer_idx; }
        size_t readAbleSize() { return _writer_idx - _reader_idx; }
        size_t writeAbleSize() { return _capacity - _writer_idx; }
        void reset() { _reader_idx = _writer_idx = 0; }
        void swap(Buffer &buf)  {
            _v.swap(buf._v);
            std::swap(_reader_idx, buf._reader_idx);
            std::swap(_writer_idx, buf._writer_idx);
            std::swap(_capacity, buf._capacity);
        }
        void push(const char *data, size_t len) { 
            assert(len <= writeAbleSize());
            std::copy(data, data+len, &_v[_writer_idx]);
            _writer_idx += len;
        }
        const char*begin() { return &_v[_reader_idx]; }
        void pop(size_t len) { 
            _reader_idx += len; 
            assert(_reader_idx<=_capacity);
            if (empty()) reset();
        }
};