#pragma once

#include "core.hpp"

// 线程池
class ThreadPool {
 public:
  explicit ThreadPool(size_t const thread_count)
      : data_(std::make_shared<data>()) {
    for (size_t i = 0; i < thread_count; ++i) {
      std::thread([data = data_] {
        std::unique_lock<std::mutex> lock(data->mutex_);
        for (;;) {
          if (!data->tasks_.empty()) {
            auto task = std::move(data->tasks_.front());
            data->tasks_.pop();
            lock.unlock();
            task();
            lock.lock();
          } else if (data->is_shutdown_) {
            break;
          } else {
            data->cond_.wait(lock);
          }
        }
      })
          .detach();
    }
  }
  ThreadPool() = default;
  ThreadPool(ThreadPool&&) = default;
  ~ThreadPool() {
    if (bool(data_)) {
      {
        std::lock_guard<std::mutex> lock(data_->mutex_);
        data_->is_shutdown_ = true;
      }
      data_->cond_.notify_all();
    }
  }
  // 任务提交
  template <class Fn>
  void execute(Fn&& task) {
    {
      std::lock_guard<std::mutex> lock(data_->mutex_);
      data_->tasks_.emplace(std::forward<Fn>(task));
    }
    data_->cond_.notify_one();
  }

 private:
  struct data {
    std::mutex mutex_;
    std::condition_variable cond_;
    bool is_shutdown_ = false;
    std::queue<std::function<void()> > tasks_;
  };
  std::shared_ptr<data> data_;
};
