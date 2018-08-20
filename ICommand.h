#define COMMAND_QUIT "quit"
#define COMMAND_INSERT "insert"
#define COMMAND_QUERY "query"
#define COMMAND_DELETE "delete"
#define COMMAND_HELP "help"
#define COMMAND_CONNECT "connect"
#define COMMAND_TEST "test"
#define COMMAND_SNAPSHOT "snapshot"

#include "core.h"
#include "shell.h"

class ICommand {
 public:
  int sendOrder();
  virtual int execute(Client &client, std::vector<std::string> &text) = 0;
};

class ConnectCommand : public ICommand {
  std::string address;
  std::string port;

 public:
  int sendOrder();
  int execute(Client &client, std::vector<std::string> &text) {
    if (client.isConnected()) {
      client.dissconect();
    }
    address = text[1];
    port = text[2];
    client.buildConnect(address.c_str(), port.c_str());
    return 0;
  }
};

class QuitCommand : public ICommand {
 public:
  int sendOrder();
  int execute(Client &client, std::vector<std::string> &text) {
    std::cout << "Bye!" << std::endl;
    exit(EXIT_SUCCESS);
    return 0;
  }
};

class InsertCommand : public ICommand {
 public:
  int sendOrder();
  int execute(Client &client, std::vector<std::string> &text) { return 0; }
};

class QueryCommand : public ICommand {
 public:
  int sendOrder();
  int execute(Client &client, std::vector<std::string> &text) { return 0; }
};

class DeleteCommand : public ICommand {
 public:
  int sendOrder();
  int execute(Client &client, std::vector<std::string> &text) { return 0; }
};

class SnapCommand : public ICommand {
 public:
  int sendOrder();
  int execute(Client &client, std::vector<std::string> &text) { return 0; }
};

class TestCommand : public ICommand {
 public:
  int sendOrder();
  int execute(Client &client, std::vector<std::string> &text) { return 0; }
};

class HelpCommand : public ICommand {
 public:
  int sendOrder();
  int execute(Client &client, std::vector<std::string> &text) { return 0; }
};
