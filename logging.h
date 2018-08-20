#pragma once
#include "core.h"

#define DEFAULT_LOG_CONFIG "log.cfg"
#define DB_LOG_SEV(scl, level, message)                               \
  BOOST_LOG_SEV((scl), (level)) << __FILE__ << ":" << __LINE__ << " " \
                                << __FUNCTION__ << "() " << (message)

#define DB_LOG_TRIVAIL(level, message)                           \
  BOOST_LOG_TRIVIAL(level) << __FILE__ << ":" << __LINE__ << " " \
                           << __FUNCTION__ << "() " << (message)

// bool init_log_environment(std::string cfg = DEFAULT_LOG_CONFIG);
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/expressions/keyword.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/setup/from_stream.hpp>
#include <fstream>
#include <iostream>
#include "core.h"
#include "logging.h"

namespace logging = boost::log;
using namespace logging::trivial;

bool init_log_environment(std::string cfg = DEFAULT_LOG_CONFIG) {
  if (!boost::filesystem::exists("./log/")) {
    boost::filesystem::create_directory("./log/");
  }
  logging::add_common_attributes();

  logging::register_simple_formatter_factory<severity_level, char>("Severity");
  logging::register_simple_filter_factory<severity_level, char>("Severity");

  std::ifstream file(cfg);
  try {
    logging::init_from_stream(file);
  } catch (const std::exception& e) {
    std::cout << "init_logger is fail, read log config file fail. curse: "
              << e.what() << std::endl;
    exit(-2);
  }
  return true;
}