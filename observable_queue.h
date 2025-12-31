// observable_queue.h
#pragma once
#include <vector>
#include <functional>
#include <mutex>
#include "thread_safe_queue.h"

template<typename T>
class ObservableQueue {
public:
    using ObserverCallback = std::function<void(const T&)>;

    explicit ObservableQueue(size_t max_size = 100)
        : queue_(new ThreadSafeQueue<T>(max_size)) {}

    // 注册观察者（同步回调）
    void Subscribe(ObserverCallback callback) {
        std::lock_guard<std::mutex> lock(observer_mutex_);
        observers_.push_back(std::move(callback));
    }

    // Push 并通知观察者
    bool Push(T item) {
        if (!queue_->Push(std::move(item))) return false;

        // 注意：观察者收到的是 item 的拷贝（对 shared_ptr 是安全的）
        T item_copy = item;
        {
            // 修改 Push 中的通知部分：
            std::vector<ObserverCallback> observers_copy;
            {
                std::lock_guard<std::mutex> lock(observer_mutex_);
                observers_copy = observers_;  // 拷贝回调列表
            }
            // 直接在锁内调用（遍历），容易死锁
            for (const auto& observer : observers_copy) {
                try {
                    observer(item_copy);
                }
                catch (...) {
                    // 忽略观察者异常，避免影响主流程
                }
            }
        }
        return true;
    }
    bool TryPush(T item) {
        if (!queue_->TryPush(std::move(item))) return false;

        // 注意：观察者收到的是 item 的拷贝（对 shared_ptr 是安全的）
        T item_copy = item;
        {
            // 修改 Push 中的通知部分：
            std::vector<ObserverCallback> observers_copy;
            {
                std::lock_guard<std::mutex> lock(observer_mutex_);
                observers_copy = observers_;  // 拷贝回调列表
            }
            // 直接在锁内调用（遍历），容易死锁
            for (const auto& observer : observers_copy) {
                try {
                    observer(item_copy);
                }
                catch (...) {
                    // 忽略观察者异常，避免影响主流程
                }
            }
        }
        return true;
    }

    // 阻塞 Pop
    void Pop(T& out_item) {
        queue_->Pop(out_item);
    }

    // 非阻塞 TryPop
    bool TryPop(T& out_item) {
        return queue_->TryPop(out_item);
    }

    void Clear() {
        queue_->Clear();
    }

    size_t Size() const {
        return queue_->Size();
    }

private:
    std::unique_ptr<ThreadSafeQueue<T>> queue_;
    std::vector<ObserverCallback> observers_;
    mutable std::mutex observer_mutex_;
};