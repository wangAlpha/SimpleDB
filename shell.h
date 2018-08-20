
#pragma once
#include "core.h"

using boost::asio::ip::tcp;
class Client {
 private:
  static const size_t BUFFER_SIZE = 1024;
  bool connected_;
  boost::asio::io_service io_service_;
  std::shared_ptr<tcp::socket> socket_;
  boost::system::error_code error_;
  boost::array<char, BUFFER_SIZE> buf_;
  std::shared_ptr<tcp::acceptor> acceptor_;

 public:
  Client() : connected_(false) {}
  ~Client() {}

  bool isConnected() const { return connected_; }
  void buildConnect(char const *address, char const *port) {
    try {
      tcp::resolver resolver(io_service_);
      tcp::resolver::query query(address, port);
      auto endpoint_iterator = resolver.resolve(query);
      socket_ = std::make_shared<tcp::socket>(io_service_);
      boost::asio::connect(*socket_, endpoint_iterator);
      connected_ = true;
    } catch (std::exception const &e) {
      std::cerr << e.what() << std::endl;
    }
  }
  char *recv() {
    if (!check_connected()) {
      throw std::runtime_error("Socket is dissconection!");
    }
    auto len = socket_->read_some(boost::asio::buffer(buf_), error_);
    return buf_.data();
  }
  int send(std::string &message) {
    boost::asio::write(*socket_, boost::asio::buffer(message), error_);
    return 0;
  }
  bool check_connected() {
    if (connected_) {
      std::string checkout = " ";
      boost::asio::write(*socket_, boost::asio::buffer(checkout), error_);
      if (boost::asio::error::eof == error_) {
        connected_ = false;
      }
    }
    return connected_;
  }
  void dissconect() {
    boost::system::error_code ec;
    socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    connected_ = false;
  }

  std::shared_ptr<tcp::socket> &getSocket() { return socket_; }
};

class Shell {
 private:
  Client client;

  void prompt();

 public:
  Shell() {}
  ~Shell() {}
  void start();
};
