#include "ThreadPool.h"
#include "core.h"

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
using boost::asio::ip::tcp;

// Logging file output init
void init_logging() {
  logging::add_file_log(keywords::file_name = "logging.log",
                        keywords::format =
                            "[%TimeStamp%] [%ThreadID%] [%Severity%] "
                            "[%ProcessID%] [%LineID%] %Message%",
                        keywords::open_mode = std::ios_base::app);
  logging::core::get()->set_filter(logging::trivial::severity >=
                                   logging::trivial::trace);
  logging::add_console_log(keywords::format =
                               "[%TimeStamp%] [%ThreadID%] [%Severity%] "
                               "[%ProcessID%] [%LineID%] %Message%");
  logging::add_common_attributes();
}
bool Quit = false;
int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage SimpleDB <port>" << std::endl;
    exit(EXIT_FAILURE);
  }
  init_logging();
  try {
    boost::asio::io_service io_service;
    tcp::acceptor acceptor(io_service,
                           tcp::endpoint(tcp::v4(), std::atoi(argv[1])));
    ThreadPool pool(128);
    while (!Quit) {
      auto socket = std::make_shared<tcp::socket>(io_service);
      acceptor.accept(*socket);
      BOOST_LOG_TRIVIAL(info) << "Accept a new TCP connect";
      // Add connnect to thread pool
      pool.execute([socket]() {
        while (true) {
          boost::system::error_code err;
          boost::array<char, 1024> buf;
          boost::system::error_code ignored_error;
          std::string message = "Hello World";
          socket->read_some(boost::asio::buffer(buf), err);
          std::cout << buf.data() << std::endl;
          boost::asio::write(*socket, boost::asio::buffer(message),
                             boost::asio::transfer_all(), ignored_error);
          if (err) {
            // Disconnect
            BOOST_LOG_TRIVIAL(error) << err.message();
            return;
          }
        }
      });
    }
  } catch (std::exception const &e) {
    BOOST_LOG_TRIVIAL(error) << e.what();
  }

  return 0;
}