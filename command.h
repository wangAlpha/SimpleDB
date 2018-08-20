#pragma once

#include "ICommand.h"
#include "core.h"

class Command {
  std::unordered_map<std::string, ICommand *> _cmdMap;

 public:
  Command();
  ~Command() {}
  ICommand *CommandProcesser(std::vector<std::string> const &text);
};