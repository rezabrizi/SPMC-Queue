#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class BlockingQueue {
public:
    BlockingQueue(size_t maxSize) : maxSize(maxSize) {}

    void push(const T& item) {
        std::unique_lock<std::mutex> lock(mutex);
        condVar.wait(lock, [this]() { return queue.size() < maxSize; });
        queue.push(item);
        lock.unlock();
        condVar.notify_all();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mutex);
        condVar.wait(lock, [this]() { return !queue.empty(); });
        T item = queue.front();
        queue.pop();
        lock.unlock();
        condVar.notify_all();
        return item;
    }

private:
    std::queue<T> queue;
    std::mutex mutex;
    std::condition_variable condVar;
    size_t maxSize;
};
