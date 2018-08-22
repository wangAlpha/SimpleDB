#include "cmdline.h"
#include "core.h"

#define DEFAULT_DB_PATH "./data.db"
#define DEFAULT_LOG_PATH "./log.cfg"
#define DEFAULT_CONF_PATH "./db.cfg"
#define DEFAULT_SVC_NAME "simpe_db"
#define DEFAULT_MAX_POOL 4

class Options {
 public:
  Options() {}
  ~Options() {}

  std::string db_path() const { return db_path_; }
  std::string log_path() const { return log_path_; }
  std::string conf_path() const { return config_path_; }
  size_t max_pool() const { return max_pool_; }

 private:
  std::string db_path_;
  std::string log_path_;
  std::string config_path_;
  std::string svc_name_;
  size_t max_pool_;
};