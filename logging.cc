#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/setup/from_stream.hpp>

#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/setup/from_stream.hpp>

bool init_log_environment(std::string _cfg) {
  namespace logging = boost::log;
  using namespace logging::trivial;

  if (!boost::filesystem::exists("./log/")) {
    boost::filesystem::create_directory("./log/");
  }
  logging::add_common_attributes();

  logging::register_simple_formatter_factory<severity_level, char>("Severity");
  logging::register_simple_filter_factory<severity_level, char>("Severity");

  std::ifstream file(_cfg);
  try {
    logging::init_from_stream(file);
  } catch (const std::exception& e) {
    std::cout << "init_logger is fail, read log config file fail. curse: "
              << e.what() << std::endl;
    exit(-2);
  }
  return true;
}
#include <boost/log/sources/severity_channel_logger.hpp>
int main(int argc, char* argv[]) {
  init_log_environment(argv[1]);
  namespace src = boost::log::sources;
  namespace logging = boost::log;
  using namespace logging::trivial;
  src::severity_channel_logger<severity_level, std::string> scl;
  BOOST_LOG_SEV(scl, debug) << __FILE__ << "::" << __LINE__ << " success";
  return 0;
}