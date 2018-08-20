#pragma once

#include "core.h"

#define CODE_REPLY 1
#define CODE_QUERY 2
#define CODE_INSERT 3
#define CODE_DELETE 4
#define CODE_SNAPSHOT 5
#define CODE_SHUTDOWN 6
#define CODE_RETURN 0

struct Header {
  uint32_t len;
  uint32_t typeCode;
};

struct MessageInsert {
  Header header;
  uint32_t length;
  uint32_t data[0];
};

struct MessageDelete {
  Header header;
  uint32_t data[0];
};

struct MessageQuery {
  Header header;
  uint32_t data[0];
};

struct MessageSnapshot {
  Header header;
  uint32_t data[0];
};

struct MessageShutdown {
  Header header;
  uint32_t data[0];
};

struct MessageReply {
  Header header;
  uint32_t data[0];
};

const char kTab_Char = '\t';
const char kEnter_Char = '\n';

// remove \n and \t character
std::string Extract_Buffer(char *argc, size_t length) {
  // build a new string
  std::string buffer(argc, argc + length);
  buffer.erase(std::remove(buffer.begin(), buffer.end(), kTab_Char));
  buffer.erase(std::remove(buffer.begin(), buffer.end(), kEnter_Char));
  return buffer;
}

std::string Command_Handle(std::string &command) {
  // std::vector<std::string>> result;

  return "";
}
