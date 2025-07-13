#include "CircularBuffer.h"

template<typename T>
CircularBuffer<T>::CircularBuffer(size_t capacity) 
    : m_capacity(capacity), m_head(0), m_tail(0), m_size(0), m_blocking(true),
      m_writeCount(0), m_readCount(0), m_dropCount(0) {
    m_buffer.resize(capacity);
}

template<typename T>
CircularBuffer<T>::~CircularBuffer() {
    clear();
}

template<typename T>
bool CircularBuffer<T>::write(const T& item) {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    if (m_blocking) {
        m_writeCondition.wait(lock, [this] { return m_size < m_capacity; });
    } else if (m_size >= m_capacity) {
        m_dropCount++;
        return false;
    }
    
    m_buffer[m_tail] = item;
    m_tail = nextIndex(m_tail);
    m_size++;
    m_writeCount++;
    
    lock.unlock();
    m_readCondition.notify_one();
    
    return true;
}

template<typename T>
bool CircularBuffer<T>::write(T&& item) {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    if (m_blocking) {
        m_writeCondition.wait(lock, [this] { return m_size < m_capacity; });
    } else if (m_size >= m_capacity) {
        m_dropCount++;
        return false;
    }
    
    m_buffer[m_tail] = std::move(item);
    m_tail = nextIndex(m_tail);
    m_size++;
    m_writeCount++;
    
    lock.unlock();
    m_readCondition.notify_one();
    
    return true;
}

template<typename T>
bool CircularBuffer<T>::writeWithTimeout(const T& item, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    if (!m_writeCondition.wait_for(lock, timeout, [this] { return m_size < m_capacity; })) {
        m_dropCount++;
        return false;
    }
    
    m_buffer[m_tail] = item;
    m_tail = nextIndex(m_tail);
    m_size++;
    m_writeCount++;
    
    lock.unlock();
    m_readCondition.notify_one();
    
    return true;
}

template<typename T>
bool CircularBuffer<T>::read(T& item) {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    if (m_blocking) {
        m_readCondition.wait(lock, [this] { return m_size > 0; });
    } else if (m_size == 0) {
        return false;
    }
    
    item = std::move(m_buffer[m_head]);
    m_head = nextIndex(m_head);
    m_size--;
    m_readCount++;
    
    lock.unlock();
    m_writeCondition.notify_one();
    
    return true;
}

template<typename T>
bool CircularBuffer<T>::readWithTimeout(T& item, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    if (!m_readCondition.wait_for(lock, timeout, [this] { return m_size > 0; })) {
        return false;
    }
    
    item = std::move(m_buffer[m_head]);
    m_head = nextIndex(m_head);
    m_size--;
    m_readCount++;
    
    lock.unlock();
    m_writeCondition.notify_one();
    
    return true;
}

template<typename T>
bool CircularBuffer<T>::peek(T& item) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_size == 0) {
        return false;
    }
    
    item = m_buffer[m_head];
    return true;
}

template<typename T>
bool CircularBuffer<T>::peekAt(size_t index, T& item) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (index >= m_size) {
        return false;
    }
    
    size_t actualIndex = (m_head + index) % m_capacity;
    item = m_buffer[actualIndex];
    return true;
}

template<typename T>
size_t CircularBuffer<T>::size() const {
    return m_size.load();
}

template<typename T>
size_t CircularBuffer<T>::capacity() const {
    return m_capacity;
}

template<typename T>
bool CircularBuffer<T>::empty() const {
    return m_size.load() == 0;
}

template<typename T>
bool CircularBuffer<T>::full() const {
    return m_size.load() >= m_capacity;
}

template<typename T>
void CircularBuffer<T>::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_head = 0;
    m_tail = 0;
    m_size = 0;
    m_writeCondition.notify_all();
    m_readCondition.notify_all();
}

template<typename T>
void CircularBuffer<T>::reset() {
    clear();
    m_writeCount = 0;
    m_readCount = 0;
    m_dropCount = 0;
}

template<typename T>
void CircularBuffer<T>::setBlocking(bool blocking) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_blocking = blocking;
    if (!blocking) {
        m_writeCondition.notify_all();
        m_readCondition.notify_all();
    }
}

template<typename T>
size_t CircularBuffer<T>::getWriteCount() const {
    return m_writeCount.load();
}

template<typename T>
size_t CircularBuffer<T>::getReadCount() const {
    return m_readCount.load();
}

template<typename T>
size_t CircularBuffer<T>::getDropCount() const {
    return m_dropCount.load();
}

template<typename T>
size_t CircularBuffer<T>::nextIndex(size_t index) const {
    return (index + 1) % m_capacity;
}

// Explicit template instantiations
template class CircularBuffer<VideoFrame>;
template class CircularBuffer<AudioFrame>;
template class CircularBuffer<std::vector<uint8_t>>;