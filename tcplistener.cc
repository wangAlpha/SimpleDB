#include "core.hpp"
#include "logging.hpp"
#include "options.hpp"
#include "threadpool.hpp"
#include "handle_message.hpp"

using boost::asio::ip::tcp;
bool Quit = false;

int main(int argc, char *argv[]) {
  Options option;
  option.ReadCmd(argc, argv);
  auto pmd_manager = KRCB::get_pmd_manager();

  pmd_manager->init(option);
  init_log_environment(option.log_file_path());
  try {
    boost::asio::io_service io_service;
    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), option.port()));
    // Create a thread pool
    ThreadPool pool(option.max_pool());
    while (!Quit) {
      auto socket = std::make_shared<tcp::socket>(io_service);
      acceptor.accept(*socket);
      BOOST_LOG_TRIVIAL(info) << "Accept a new connect from client ";

      // Add connnect to thread pool
      pool.execute([sock = socket] {
        while (true) {
          boost::system::error_code err;
          boost::array<char, 4096> buf;
          boost::system::error_code ignored_error;
          auto len = sock->read_some(boost::asio::buffer(buf), err);
          auto command = HandleMessage(buf.data(), len);
          std::string reply_message = "Successly";
          boost::asio::write(*sock, boost::asio::buffer(reply_message),
                             boost::asio::transfer_all(), ignored_error);
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
