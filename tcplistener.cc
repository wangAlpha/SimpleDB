#include <boost/asio.hpp>
#include "ThreadPool.h"
#include "core.h"
#include "logging.h"

using boost::asio::ip::tcp;
bool Quit = false;

std::string Extract_Buffer(char *argc, size_t length) {
  *std::remove(argc, argc + length, '\n') = '\0';
  return std::string(argc);
}
std::string Command_Handle(std::string &&command) {
  std::string result("");

  return result;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage SimpleDB <port>" << std::endl;
    exit(EXIT_FAILURE);
  }
  init_log_environment();
  try {
    boost::asio::io_service io_service;
    tcp::acceptor acceptor(io_service,
                           tcp::endpoint(tcp::v4(), std::atoi(argv[1])));
    ThreadPool pool(128);
    while (!Quit) {
      auto socket = std::make_shared<tcp::socket>(io_service);
      acceptor.accept(*socket);
      BOOST_LOG_TRIVIAL(info) << "Accept a new client connect";
      // Add connnect to thread pool
      pool.execute([sock = socket]() {
        while (true) {
          boost::system::error_code err;
          boost::array<char, 1024> buf;
          boost::system::error_code ignored_error;
          std::string message = "Hello World";
          auto len = sock->read_some(boost::asio::buffer(buf), err);
          // DB_LOG_TRIVAIL(info,;

          std::cout << "recv a message len:" << len << "data: " << buf.data()
                    << '\n';
          auto command = Extract_Buffer(buf.data(), len);
          if (command.size() > 0) {
            auto result = Command_Handle(std::move(command));
            std::cout << "Extract result: " << command;
            boost::asio::write(*sock, boost::asio::buffer(message),
                               boost::asio::transfer_all(), ignored_error);
          }
          if (err) {
            // Disconnect
            DB_LOG_TRIVAIL(error, err.message());
            DB_LOG_TRIVAIL(info, "Disconnect client");
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