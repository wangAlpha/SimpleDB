#pragma once

#include "core.hpp"
#include "logging.hpp"
#include "message.hpp"
#include "shell.hpp"

#define COMMAND_QUIT "quit"
#define COMMAND_INSERT "insert"
#define COMMAND_QUERY "query"
#define COMMAND_DELETE "delete"
#define COMMAND_HELP "help"
#define COMMAND_CONNECT "connect"
#define COMMAND_TEST "test"
#define COMMAND_SNAPSHOT "snapshot"
#define COMMAND_SHUTDOWN "shutdown"

extern bool gQuit;

class ICommand {
 public:
  int sendOrder(Client &client, int code) {
    Header header;
    header.len = sizeof(header);
    header.typeCode = code;

    auto rc = client.send((void *)&header, sizeof(header));
    if (rc) {
      return getError(rc);
    }
    return rc;
  }

  template <class Fn>
  int sendOrder(Client &client, Fn &fun, std::string &message) {
    int ret = OK;
    std::vector<uint8_t> pack;
    try {
      auto json_obj = nlohmann::json::parse(message);
      pack = nlohmann::json::to_msgpack(json_obj);
    } catch (nlohmann::json::exception const &e) {
      std::cerr << e.what() << std::endl;
      return getError(ErrCommand);
    }
    auto size = SEND_BUFFER_SIZE;
    auto send_buffer = Send_Buffer;

    fun(send_buffer, &size, pack);
    std::cout << "send size: " << size << std::endl;
    ret = client.send((void *)send_buffer, size);
    if (ret) {
      return getError(ret);
    }
    return ret;
  }

  int getError(int code) {
    switch (code) {
      case OK:
        std::cout << "execute successful" << std::endl;
        break;
      case ErrConnected: {
        std::cerr << "net is closeed" << std::endl;
        break;
      }
      default: {
        std::cerr << "Occur error" << std::endl;
        break;
      }
    }
    return code;
  }

  int recvReply(Client &client) {
    auto &len = recv_buffer_length_;
    len = RECV_BUFFER_SIZE;
	memset((void*)recv_buffer_, 0, sizeof(recv_buffer_));
    client.recv(recv_buffer_, len);
    if (len >= RECV_BUFFER_SIZE) {
      return getError(Err_Recv_Length);
    }
    if (recv_buffer_[len - 1] == MagicNumber) {
      recv_buffer_[len - 1] = '\0';
    }
    for (size_t i = 0; i < len; ++i) {
      printf("%x ", recv_buffer_[i]);
    }
    return OK;
  }
  int handleReply();
  virtual int execute(Client &client, std::vector<std::string> &text) = 0;
  size_t recv_buffer_length_;
  char recv_buffer_[RECV_BUFFER_SIZE];
};

class ConnectCommand : public ICommand {
  std::string address_;
  std::string port_;

 public:
  int execute(Client &client, std::vector<std::string> &text) {
    if (client.isConnected()) {
      client.dissconect();
    }
    address_ = text[1];
    port_ = text[2];
    client.buildConnect(address_.c_str(), port_.c_str());
    return OK;
  }
};

class QuitCommand : public ICommand {
 public:
  int handleReply() {
    std::cerr << "Bye!" << std::endl;
    gQuit = true;
    return OK;
  }

  int execute(Client &client, std::vector<std::string> &text) {
    auto ret = OK;
    if (!client.isConnected()) {
      ret = sendOrder(client, CODE_DISCONNECT);
      client.dissconect();
    }
    ret = handleReply();
    return 0;
  }
};

class ShutdownCommand : public ICommand {
 public:
  int handleReply() {
    std::cerr << "Shutdown...!" << std::endl;
    return OK;
  }

  int execute(Client &client, std::vector<std::string> &text) {
    auto ret = OK;
    if (!client.isConnected()) {
      return getError(ErrSockNotConnect);
    }
    ret = sendOrder(client, CODE_SHUTDOWN);
    ret = recvReply(client);
    client.dissconect();
    ret = handleReply();
    return 0;
  }
};
class InsertCommand : public ICommand {
 public:
  int handleReply() {
    auto message = (MessageReply *)recv_buffer_;
    auto returnCode = message->returnCode;
    return getError(returnCode);
  }

  int execute(Client &client, std::vector<std::string> &text) {
    int rc = OK;
    if (!client.isConnected()) {
      return getError(ErrConnected);
    }
    if (text.size() != 2) {
      return getError(ErrSubCommand);
    }
    rc = sendOrder(client, BuildInsert, text[1]);
    ErrCheck(rc, error, ErrInvaildArg, "Send order fail");
    rc = recvReply(client);
    rc = handleReply();
    return rc;
  }
};

class QueryCommand : public ICommand {
 public:
  int handleReply() {
    auto message = (MessageReply *)recv_buffer_;
    auto return_code = message->returnCode;
    auto ret = getError(return_code);
    if (ret) {
      return ret;
    }
    try {
      auto data = nlohmann::json::from_msgpack(message->data);
      std::cout << data.dump() << std::endl;
    } catch (nlohmann::json::exception const &e) {
      DB_LOG(error, e.what());
      ret = ErrInvaildArg;
    }
    return ret;
  }
  int execute(Client &client, std::vector<std::string> &text) {
    auto rc = OK;
    if (text.size() < 2) {
      return getError(ErrInvaildArg);
    }
    rc = sendOrder(client, BuildQuery, text[1]);
    ErrCheck(rc, error, ErrInvaildArg, "Failed to send order to query");
    rc = recvReply(client);
    rc = handleReply();
    return rc;
  }
};

class DeleteCommand : public ICommand {
 public:
  int handleReply() {
    auto message = (MessageReply *)recv_buffer_;
    auto return_code = message->returnCode;
    auto ret = getError(return_code);
    return ret;
  }

  int execute(Client &client, std::vector<std::string> &text) {
    auto rc = OK;
    if (text.size() < 2) {
      return getError(ErrInvaildArg);
    }
    rc = sendOrder(client, BuildDelete, text[1]);
    ErrCheck(rc, error, ErrInvaildArg, "Failed to send delete order to server");
    rc = recvReply(client);
    rc = handleReply();
    return rc;
  }
};

class SnapCommand : public ICommand {
 public:
  int handleReply() {
    auto ret = OK;
    auto message = (MessageReply *)recv_buffer_;
    auto return_code = message->returnCode;
    if (ret) {
      return getError(return_code);
    }
    nlohmann::json json_obj;
    try {
      json_obj =
          nlohmann::json::from_msgpack(message->data, recv_buffer_length_);
      std::cout << json_obj.dump() << std::endl;
    } catch (nlohmann::json::exception const &e) {
      DB_LOG(error, e.what());
      ret = ErrInvaildArg;
    }
    printf("insert time is %u\n", json_obj[INSERT_TIME].get<uint32_t>());
    printf("del time is %u\n", json_obj[DEL_TIME].get<uint32_t>());
    printf("query time is %u\n", json_obj[QUERY_TIME].get<uint32_t>());
    printf("server run time id %um\n", json_obj[RUN_TIME].get<uint32_t>());
    return ret;
  }

  int execute(Client &client, std::vector<std::string> &text) {
    auto rc = OK;
    if (!client.isConnected()) {
      return getError(ErrSockNotConnect);
    }
    rc = sendOrder(client, CODE_SNAPSHOT);
    ErrCheck(rc, error, ErrInvaildArg, "Failed to send order snapshot ");
    rc = recvReply(client);
    rc = handleReply();
    return 0;
  }
};

class TestCommand : public ICommand {
 public:
  int execute(Client &client, std::vector<std::string> &text) {
    // 需要编写测试用例
    return OK;
  }
};

class HelpCommand : public ICommand {
 public:
  int execute(Client &client, std::vector<std::string> &text) {
    printf("List of classes of commands:\n\n");
    printf("%s [server] [port]-- connecting SimpleDB server\n",
           COMMAND_CONNECT);
    printf("%s -- sending a insert command to SimpleDB server\n",
           COMMAND_INSERT);
    printf("%s -- sending a query command to SimpleDB server\n", COMMAND_QUERY);
    printf("%s -- sending a delete command to SimpleDB server\n",
           COMMAND_DELETE);
    printf("%s [number]-- sending a test command to SimpleDB server\n",
           COMMAND_TEST);
    printf("%s -- providing current number of record inserting\n",
           COMMAND_SNAPSHOT);
    printf("%s -- quitting command\n\n", COMMAND_QUIT);
    printf("%s -- shutdown command\n\n", COMMAND_SHUTDOWN);
    printf("Type \"help\" command for help \n");
    return OK;
  }
};
