#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <vector>
#include <chrono>
#include "Packet.h"

// ������ṹ
template<typename T>
struct QueueItem {
    size_t nOperator;     // �������ͱ�ʶ
    T Data;               // ��������
    std::shared_ptr<void> hEvent;  // �¼����

    // Ĭ�Ϲ��캯��
    QueueItem() : nOperator(0), Data(), hEvent(nullptr) {}

    // ���������캯��
    QueueItem(size_t op, const T& data, std::shared_ptr<void> event = nullptr)
        : nOperator(op), Data(data), hEvent(event) {
    }

    // �������캯��
    QueueItem(const QueueItem& other) = default;

    // ��ֵ�����
    QueueItem& operator=(const QueueItem& other) = default;
};

// �̰߳�ȫ�Ķ�����
template<typename T>
class CQueue {
public:
    CQueue() = default;
    ~CQueue() = default;

    // ��Ӳ���
    void push(const QueueItem<T>& item);
    void push(size_t nOperator, const T& data, std::shared_ptr<void> hEvent = nullptr);

    // ���Ӳ���
    bool pop(QueueItem<T>& item);
    bool pop(QueueItem<T>& item, std::chrono::milliseconds timeout);

    // ����״̬
    bool empty() const;
    size_t size() const;

    // ��ն���
    void clear();

    // ��������
    void pushBatch(const std::vector<QueueItem<T>>& items);
    std::vector<QueueItem<T>> popBatch(size_t maxCount);

    // �ȴ����зǿ�
    void waitForData();

private:
    mutable std::mutex m_mutex;
    std::queue<QueueItem<T>> m_queue;
    std::condition_variable m_condition;
};

// ר���������ݰ��Ķ�������
using PacketQueue = CQueue<CPacket>;
using PacketQueueItem = QueueItem<CPacket>;

// ʹ�� Command.h �ж���� Type ö��
// ������Ҫ�ظ������������
