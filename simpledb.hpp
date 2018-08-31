#pragma once

#include "core.hpp"

class SimpleDB {
 public:
  SimpleDB(std::string const &host) {}
  SimpleDB(const char *host) {}
  ~SimpleDB() { quit(); }

  int insert(std::string const &cmd) { return 0; }
  int del() { return 0; }
  std::string query(std::string const &value) { return ""; }
  int update() { return 0; }
  int snapshot() { return 0; }
  int quit() { return 0; }

  int commit() { return 0; }

 private:
};