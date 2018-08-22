#include <boost/asio.hpp>
#include "core.h"
#include "logging.h"
#include "message.h"
#include "threadpool.h"

using boost::asio::ip::tcp;
bool Quit = false;

void TCPListener(std::shared_ptr<tcp::socket> &sock) {}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage SimpleDB <port>" << std::endl;
    exit(EXIT_FAILURE);
  }
  // init_log_environment(DEFAULT_LOG_CONFIG);
  try {
    boost::asio::io_service io_service;
    tcp::acceptor acceptor(io_service,
                           tcp::endpoint(tcp::v4(), std::atoi(argv[1])));
    // Create a thread pool
    ThreadPool pool(128);
    while (!Quit) {
      auto socket = std::make_shared<tcp::socket>(io_service);
      acceptor.accept(*socket);
      BOOST_LOG_TRIVIAL(info) << "Accept a new connect from client ";

      // Add connnect to thread pool
      pool.execute([sock = socket] {
        while (true) {
          boost::system::error_code err;
          boost::array<char, 1024> buf;
          boost::system::error_code ignored_error;
          std::string message = "Hello World";
          auto len = sock->read_some(boost::asio::buffer(buf), err);
          auto command = Extract_Buffer(buf.data(), len);
          if (command.size() > 0) {
            auto result = Command_Handle(command);
            boost::asio::write(*sock, boost::asio::buffer(message),
                               boost::asio::transfer_all(), ignored_error);
          }
          if (err) {
            DB_LOG_TRIVAIL(error, err.message());
            return;
          }
        }
      });
    }
  } catch (std::exception const &e) {
    DB_LOG_TRIVAIL(error, e.what());
  }

  return 0;
}