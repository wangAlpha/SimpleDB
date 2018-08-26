#pragma once
#include "core.hpp"
#include "options.hpp"
#include "rtn.hpp"

class KRCB {
 public:
  std::string data_file_path() const { return data_file_path_; }
  std::string log_file_path() const { return log_file_path_; }

  void set_log_file_path(const std::string &path) { log_file_path_ = path; }
  void set_svc_name(const std::string &name) { svc_name_ = name; }
  void set_data_file_path(const std::string &path) { data_file_path_ = path; }
  void set_max_pool(size_t max_pool) { max_pool_ = max_pool; }

  RunTime *get_rtn() { return &run_time_manager_; }
  // int init(Options const &option);
  static KRCB *get_pmd_manager() {
    if (!krcb_manager_) {
      krcb_manager_ = new KRCB();
    }
    return krcb_manager_;
  }

  int init(Options const &option);

  KRCB(KRCB const &) = delete;
  void operator=(KRCB const &) = delete;

 private:
  size_t max_pool_;
  std::string data_file_path_;
  std::string log_file_path_;
  std::string svc_name_;
  RunTime run_time_manager_;
  KRCB(){};

  static KRCB *krcb_manager_;
};

int KRCB::init(Options const &option) {
  log_file_path_ = option.log_file_path();
  data_file_path_ = option.db_file_path();
  svc_name_ = option.svc_name();
  max_pool_ = option.max_pool();
  return OK;
}

