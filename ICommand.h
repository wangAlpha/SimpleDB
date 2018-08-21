#pragma once

#include "core.h"
#include "logging.h"
#include "message.h"
#include "shell.h"

#define COMMAND_QUIT "quit"
#define COMMAND_INSERT "insert"
#define COMMAND_QUERY "query"
#define COMMAND_DELETE "delete"
#define COMMAND_HELP "help"
#define COMMAND_CONNECT "connect"
#define COMMAND_TEST "test"
#define COMMAND_SNAPSHOT "snapshot"

class ICommand {
 public:
  int sendOrder(Client &client, int code) {
    Header header;
    header.len = sizeof(header);
    header.typeCode = code;

    client.send((void *)&header, sizeof(header));
  }
  template <class Fn>
  int sendOrder(Client &client, Fn &fun, std::string &message) {}
  int getError(int err) { return OK; }
  int handleReply();
  virtual int execute(Client &client, std::vector<std::string> &text) = 0;
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
  int execute(Client &client, std::vector<std::string> &text) {
    std::cout << "Bye!" << std::endl;
    // TODO
    exit(EXIT_SUCCESS);
    return 0;
  }
};

class InsertCommand : public ICommand {
 public:
  int recvReply(Client &clien) { return 0; }
  int execute(Client &client, std::vector<std::string> &text) {
    int rc = OK;
    if (!client.isConnected()) {
      return getError(ErrConnected);
    }
    if (text.size() != 2) {
      return getError(ErrSubCommand);
    }
    void *buffer = nullptr;
    size_t size = 0;
    // BuildInsert(buffer, &size, );
    rc = sendOrder(client, BuildInsert, text[2]);

    rc = recvReply(client);

    rc = handleReply();
    return OK;
  }
};

class QueryCommand : public ICommand {
 public:
  int execute(Client &client, std::vector<std::string> &text) { return 0; }
};

class DeleteCommand : public ICommand {
 public:
  int execute(Client &client, std::vector<std::string> &text) { return 0; }
};

class SnapCommand : public ICommand {
 public:
  int execute(Client &client, std::vector<std::string> &text) { return 0; }
};

class TestCommand : public ICommand {
 public:
  int execute(Client &client, std::vector<std::string> &text) { return 0; }
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
    printf("Type \"help\" command for help \n");
    return 0;
  }
};
