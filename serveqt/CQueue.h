#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <vector>
#include <chrono>
#include "Packet.h"

// 队列项结构
template<typename T>
struct QueueItem {
    size_t nOperator;     // 操作类型标识
    T Data;               // 数据内容
    std::shared_ptr<void> hEvent;  // 事件句柄

    // 默认构造函数
    QueueItem() : nOperator(0), Data(), hEvent(nullptr) {}

    // 参数化构造函数
    QueueItem(size_t op, const T& data, std::shared_ptr<void> event = nullptr)
        : nOperator(op), Data(data), hEvent(event) {
    }

    // 拷贝构造函数
    QueueItem(const QueueItem& other) = default;

    // 赋值运算符
    QueueItem& operator=(const QueueItem& other) = default;
};

// 线程安全的队列类
template<typename T>
class CQueue {
public:
    CQueue() = default;
    ~CQueue() = default;

    // 入队操作
    void push(const QueueItem<T>& item);
    void push(size_t nOperator, const T& data, std::shared_ptr<void> hEvent = nullptr);

    // 出队操作
    bool pop(QueueItem<T>& item);
    bool pop(QueueItem<T>& item, std::chrono::milliseconds timeout);

    // 队列状态
    bool empty() const;
    size_t size() const;

    // 清空队列
    void clear();

    // 批量操作
    void pushBatch(const std::vector<QueueItem<T>>& items);
    std::vector<QueueItem<T>> popBatch(size_t maxCount);

    // 等待队列非空
    void waitForData();

private:
    mutable std::mutex m_mutex;
    std::queue<QueueItem<T>> m_queue;
    std::condition_variable m_condition;
};

// 专门用于数据包的队列类型
using PacketQueue = CQueue<CPacket>;
using PacketQueueItem = QueueItem<CPacket>;

// 使用 Command.h 中定义的 Type 枚举
// 不再需要重复定义操作类型
