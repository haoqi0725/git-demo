#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

template<typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue;
    mutable std::mutex mtx;
    std::condition_variable cv;
    bool finished = false;

public:
    ThreadSafeQueue() = default;
    
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    // 添加元素
    void Push(const T& value) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            queue.push(value);
        }
        cv.notify_one();
    }
    
    void Push(T&& value) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            queue.push(std::move(value));
        }
        cv.notify_one();
    }
    
    // 非阻塞弹出
    bool TryPop(T& out) {
        std::lock_guard<std::mutex> lock(mtx);
        if (queue.empty()) return false;
        out = std::move(queue.front());
        queue.pop();
        return true;
    }
    
    // 检查是否为空
    bool Empty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.empty();
    }
    
    // ✅ 获取队列大小
    size_t Size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.size();
    }
    
    // ✅ 清空队列
    void Clear() {
        std::lock_guard<std::mutex> lock(mtx);
        while (!queue.empty()) {
            queue.pop();
        }
    }
    
    // ✅ 标记完成并唤醒所有等待线程
    void Finish() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            finished = true;
        }
        cv.notify_all();
    }
    
    // 重置队列
    void Reset() {
        std::lock_guard<std::mutex> lock(mtx);
        finished = false;
        while (!queue.empty()) {
            queue.pop();
        }
    }
};

#endif // THREAD_SAFE_QUEUE_H