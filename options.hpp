#pragma once

#include "cmdline.hpp"
#include "core.hpp"

#define DEFAULT_PORT 80
#define DEFAULT_DB_PATH "./data.db"
#define DEFAULT_LOG_PATH "./log.cfg"
#define DEFAULT_CONF_PATH "./db.cfg"
#define DEFAULT_SVC_NAME "simpe_db"
#define DEFAULT_MAX_POOL 4

class Options {
 public:
  explicit Options() {}
  Options(int argc, char *argv[]) { ReadCmd(argc, argv); }
  ~Options() {}

  int ReadCmd(int argc, char *argv[]) {
    cmdline::parser parser;
    parser.add<uint16_t>("port", 'p', "port number", false, 80,
                         cmdline::range(1, 65535));
    parser.add<size_t>("pool", 'n', "max thread pool", false, DEFAULT_MAX_POOL,
                       cmdline::range(1, 65535));
    parser.add<std::string>("log", 'l', "log config path", false,
                            DEFAULT_LOG_CONFIG);
    parser.add<std::string>("db", 'd', "db data file name", false,
                            DEFAULT_DB_PATH);
    parser.add<std::string>("svc", 's', "svc name", false, DEFAULT_SVC_NAME);
    parser.add<std::string>("conf", 'c', "config file name", false,
                            DEFAULT_CONF_PATH);

    parser.parse_check(argc, argv);

    port_ = parser.get<uint16_t>("port");
    max_pool_ = parser.get<size_t>("pool");
    db_path_ = parser.get<std::string>("db");
    log_path_ = parser.get<std::string>("log");
    config_path_ = parser.get<std::string>("conf");
    svc_name_ = parser.get<std::string>("svc");

    printf("port: %hu\n", port_);
    printf("max pool: %lu\n", max_pool_);
    printf("db data path: %s\n", db_path_.c_str());
    printf("log config path: %s\n", log_path_.c_str());
    printf("svc name: %s\n", svc_name_.c_str());
    printf("config path name: %s\n", config_path_.c_str());
    return OK;
  }

  std::string db_file_path() const { return db_path_; }
  std::string log_file_path() const { return log_path_; }
  std::string conf_file_path() const { return config_path_; }
  std::string svc_name() const { return svc_name_; }
  uint16_t port() const { return port_; }
  size_t max_pool() const { return max_pool_; }

 private:
  std::string db_path_;
  std::string log_path_;
  std::string config_path_;
  std::string svc_name_;
  uint16_t port_;
  size_t max_pool_;
};
