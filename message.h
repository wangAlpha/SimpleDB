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
  uint32_t returnCode;
  uint32_t numReturn;
  uint32_t data[0];
};

constexpr char kTab_Char = '\t';
constexpr char kEnter_Char = '\n';
// remove \n and \t character
std::string Extract_Buffer(char *argc, size_t length) {
  // build a new string
  std::string buffer(argc, argc + length);
  buffer.erase(std::remove(buffer.begin(), buffer.end(), kTab_Char));
  buffer.erase(std::remove(buffer.begin(), buffer.end(), kEnter_Char));
  return buffer;
}

using json = nlohmann::json;
int BuildReply(char **buffer, int *buffer_size, int returnCode,
               json *objReturn) {
  MessageReply *reply = nullptr;
  auto size = sizeof(MessageReply);
  if (objReturn != nullptr) {
    // size += objReturn.
  }
  reply = (MessageReply *)(*buffer);
  // build header
  reply->header.len = size;
  reply->header.typeCode = CODE_REPLY;
  // build body
  reply->returnCode = returnCode;
  reply->numReturn = (objReturn == nullptr);

  // json object
  if (objReturn) {
    // memcpy(&reply->data[0], objReturn->objectdata(), objReturn->objSize());
  }
  return OK;
}

int ExtractReply(char *buffer, int &returnCode, int &numReturn,
                 const char **obj) {
  auto reply = (MessageReply *)buffer;
  if (reply->header.len < sizeof(MessageReply)) {
    // Error
    return -1;
  }
  if (reply->header.typeCode != CODE_REPLY) {
    // Error
    return -1;
  }
  returnCode = reply->returnCode;
  numReturn = reply->numReturn;

  if (0 == numReturn) {
    *obj = nullptr;
  } else {
    *obj = (char *)(&reply->data[0]);
  }
  return OK;
}

int BuildInsert(char **buffer, int *buffer_size, json &obj) {
  // TODO
  auto size = sizeof(MessageInsert) + obj.size();
  // buffere is allocated
  auto insert_value = (MessageInsert *)(*buffer);
  // build header
  insert_value->header.len = size;
  insert_value->header.typeCode = CODE_INSERT;
  // build object
  insert_value->length = 1;
  // memcpy(&insert_value->data[0], )
  return OK;
}

int BuildInsert(char **buffer, int *buffer_size, std::vector<json *> &objs) {
  size_t size = 0;
  for (auto &it : objs) {
    // size += it->size();
  }

  // 批量复制
  return OK;
}

int ExtractInsert(char *buffer, int &num_insert, const char **objs) {
  auto insert_message = (MessageInsert *)buffer;
  if (insert_message->header.len < sizeof(MessageInsert)) {
    // message error
    return -1;
  }

  if (insert_message->header.typeCode != CODE_INSERT) {
    // message error
    return -1;
  }

  num_insert = insert_message->length;
  if (0 == num_insert) {
    *objs = nullptr;
  } else {
    *objs = (char *)&insert_message->data[0];
  }
  return OK;
}

int BuildDelete(char **buffer, int *buffer_size, json &key) {
  auto size = sizeof(MessageDelete) + key.size();
  // check buffer
  //
  auto del_message = (MessageDelete *)(*buffer);
  // build header
  del_message = (MessageDelete *)(*buffer);
  // build header
  del_message->header.len = size;
  del_message->header.typeCode = CODE_DELETE;
  // bson object
  // memcpy(&)
}

std::string Command_Handle(std::string &command) {
  // std::vector<std::string>> result;
  std::vector<std::string> text;

  return "";
}
