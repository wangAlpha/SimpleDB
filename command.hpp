#pragma once

#include "core.hpp"
#include "icommand.hpp"

class Command {
  std::unordered_map<std::string, ICommand *> _cmdMap;

 public:
  Command();
  ~Command() {}
  ICommand *CommandProcesser(std::vector<std::string> const &text);
};

// add command map
Command::Command() {
  _cmdMap[COMMAND_QUIT] = new QuitCommand();
  _cmdMap[COMMAND_INSERT] = new InsertCommand();
  _cmdMap[COMMAND_CONNECT] = new ConnectCommand();
  _cmdMap[COMMAND_QUERY] = new QueryCommand();
  _cmdMap[COMMAND_HELP] = new HelpCommand();
  _cmdMap[COMMAND_DELETE] = new DeleteCommand();
  _cmdMap[COMMAND_TEST] = new TestCommand();
  _cmdMap[COMMAND_SNAPSHOT] = new SnapCommand();
}

// return a execute class or nullptr
ICommand *Command::CommandProcesser(std::vector<std::string> const &text) {
  auto cmd = text[0];
  auto result = _cmdMap.find(cmd);
  if (result != _cmdMap.end()) {
    return result->second;
  } else {
    return nullptr;
  }
}