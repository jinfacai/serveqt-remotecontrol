#include "CQueue.h"

template<typename T>
void CQueue<T>::push(const QueueItem<T>& item) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push(item);
    m_condition.notify_one();
}

template<typename T>
void CQueue<T>::push(size_t nOperator, const T& data, std::shared_ptr<void> hEvent) {
    push(QueueItem<T>(nOperator, data, hEvent));
}

template<typename T>
bool CQueue<T>::pop(QueueItem<T>& item) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_queue.empty()) {
        return false;
    }

    item = m_queue.front();
    m_queue.pop();
    return true;
}

template<typename T>
bool CQueue<T>::pop(QueueItem<T>& item, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_condition.wait_for(lock, timeout, [this] { return !m_queue.empty(); })) {
        return false; // 超时
    }

    item = m_queue.front();
    m_queue.pop();
    return true;
}

template<typename T>
bool CQueue<T>::empty() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.empty();
}

template<typename T>
size_t CQueue<T>::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

template<typename T>
void CQueue<T>::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_queue.empty()) {
        m_queue.pop();
    }
}

template<typename T>
void CQueue<T>::pushBatch(const std::vector<QueueItem<T>>& items) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& item : items) {
        m_queue.push(item);
    }
    m_condition.notify_all();
}

template<typename T>
std::vector<QueueItem<T>> CQueue<T>::popBatch(size_t maxCount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<QueueItem<T>> result;

    while (!m_queue.empty() && result.size() < maxCount) {
        result.push_back(m_queue.front());
        m_queue.pop();
    }

    return result;
}

template<typename T>
void CQueue<T>::waitForData() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [this] { return !m_queue.empty(); });
}

// 显式实例化模板
template class CQueue<CPacket>;
