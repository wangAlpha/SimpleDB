#include "core.hpp"
#include "handle_message.hpp"
#include "logging.hpp"
#include "options.hpp"
#include "threadpool.hpp"

using boost::asio::ip::tcp;
std::atomic<bool> gQuit(false);

static int SignalHandler() {
  signal(SIGINT, SIG_IGN);
  return 0;
}

int main(int argc, char *argv[]) {
  Options option;
  option.ReadCmd(argc, argv);
  auto pmd_manager = KRCB::get_pmd_manager();

  SignalHandler();
  pmd_manager->init(option);
  init_log_environment(option.log_file_path());
  try {
    boost::asio::io_service io_service;
    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), option.port()));
    // Create a thread pool
    ThreadPool pool(option.max_pool());
    while (!gQuit) {
      auto socket = std::make_shared<tcp::socket>(io_service);
      acceptor.accept(*socket);
      BOOST_LOG_TRIVIAL(info) << "Accept a new connect from client ";

      // Add connnect to thread pool
      pool.execute([sock = socket] {
        bool disconnect = false;
        while (!disconnect) {
          boost::system::error_code err;
          boost::array<char, RECV_BUFFER_SIZE> buf;
          boost::system::error_code ignored_error;
          std::vector<uint8_t> reply_message(SEND_BUFFER_SIZE, 0);

          auto len = sock->read_some(boost::asio::buffer(buf), err);
          HandleMessage(buf.data(), &len, &disconnect, &reply_message);
          boost::asio::write(*sock, boost::asio::buffer(reply_message, len),
                             boost::asio::transfer_all(), ignored_error);
          if (err) {
            DB_LOG(error, err.message());
            return;
          }
        }
      });
    }
    DB_LOG(debug, "Shutdown...");
  } catch (std::exception const &e) {
    DB_LOG(error, e.what());
  }

  return 0;
}
