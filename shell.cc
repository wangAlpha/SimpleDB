#include "shell.hpp"
#include "command.hpp"
#include "core.hpp"

bool gQuit = false;
void Shell::prompt() {
  std::cout << "Shell > ";
  std::string buffer;
  getline(std::cin, buffer, '\n');
  std::vector<std::string> text;

  boost::split(text, buffer, [](char ch) { return ch == ' '; });
  Command cmd;
  auto result = cmd.CommandProcesser(text);
  if (result != nullptr) {
    result->execute(client, text);
  } else {
    std::cerr << "Shell: command not found: " << buffer << std::endl;
  }
}

void Shell::start() {
  std::cout << "Welcome to SimpleDB Shell!" << std::endl;
  std::cout << "Simple help for help, Ctrl+c or quit to exit\n" << std::endl;

  while (gQuit == false) {
    prompt();
  }
}

int main(int argc, const char *argv[]) {
  Shell shell;
  shell.start();
  return 0;
}