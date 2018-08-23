#include <boost/asio.hpp>
#include "core.h"
#include "logging.h"
#include "message.h"
#include "options.h"
#include "threadpool.h"

using boost::asio::ip::tcp;
bool Quit = false;

int main(int argc, char *argv[]) {
  // Options option;
  // try {
  //   option.ReadCmd(argc, argv);
  // } catch (std::exception const &e) {
  //   std::cerr << e.what() << std::endl;
  // }
  // init_log_environment(option.log_path());
  try {
    boost::asio::io_service io_service;
    tcp::acceptor acceptor(io_service,
                           tcp::endpoint(tcp::v4(), std::atoi(argv[1])));
    // Create a thread pool
    ThreadPool pool(80);
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
            DB_LOG(error, err.message());
            return;
          }
        }
      });
    }
  } catch (std::exception const &e) {
    DB_LOG(error, e.what());
  }

  return 0;
}