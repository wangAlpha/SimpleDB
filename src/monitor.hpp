#pragma once
#include <ctime>
#include "core.hpp"

class Monitor {
 public:
  explicit Monitor() : insert_times_(0), query_times_(0), del_times_(0) {
    gettimeofday(&start_, nullptr);
  }
  ~Monitor() {}
  void increase_insert_times() { insert_times_++; }
  void set_insert_time(uint32_t const times) { insert_times_ = times; }
  uint32_t insert_times() const { return insert_times_; }
  void increase_query_times() { query_times_++; }
  void set_query_time(uint32_t const times) { query_times_ = times; }
  uint32_t query_time() const { return query_times_; }
  void increase_delete_tims() { del_times_++; }
  void set_delete_times(uint32_t const times) { del_times_ = times; }
  uint32_t delete_time() const { return del_times_; }
  uint32_t server_run_time() {
    struct timeval current;
    gettimeofday(&current, nullptr);
    auto times = (current.tv_sec - start_.tv_sec);
    return times;
  }

 private:
  std::atomic_int query_times_;
  std::atomic_int insert_times_;
  std::atomic_int del_times_;
  struct timeval start_;
};