#pragma once

#include "core.hpp"
#include "logging.hpp"

constexpr int CODE_REPLY = 1;
constexpr int CODE_QUERY = 2;
constexpr int CODE_INSERT = 3;
constexpr int CODE_DELETE = 4;
constexpr int CODE_SNAPSHOT = 5;
constexpr int CODE_SHUTDOWN = 6;
constexpr int CODE_CONNECT = 7;
constexpr int CODE_DISCONNECT = 8;

constexpr int CODE_RETURN = 0;

const char *INSERT_TIME = "insert_time";
const char *DEL_TIME = "del_time";
const char *QUERY_TIME = "query_time";
const char *RUN_TIME = "run_time";

// [0]为变长数组，用于取出数据地址,不占用空间
struct Header {
  uint32_t len;
  uint32_t typeCode;
};

struct MessageInsert {
  Header header;
  uint32_t length;
  char data[0];
};

struct MessageDelete {
  Header header;
  char key[0];
};

struct MessageQuery {
  Header header;
  char key[0];
};

struct MessageSnapshot {
  Header header;
  char data[0];
};

struct MessageShutdown {
  Header header;
  char data[0];
};

struct MessageReply {
  Header header;
  uint32_t returnCode;
  uint32_t numReturn;
  char data[0];
};

int BuildReply(char *buffer, int *buffer_size, int return_code,
               nlohmann::json const &json_obj) {
  auto size = sizeof(MessageReply);
  std::vector<uint8_t> pack;
  try {
    pack = nlohmann::json::to_msgpack(json_obj);
  } catch (nlohmann::json::exception const &e) {
    DB_LOG(error, e.what());
    return ErrSys;
  }
  *buffer_size = size + pack.size();
  auto reply = (MessageReply *)buffer;
  // build header
  reply->header.len = (uint32_t)size;
  reply->header.typeCode = CODE_REPLY;
  // build body
  reply->returnCode = return_code;
  reply->numReturn = (pack.size() == 0);
  // json object
  if (pack.size()) {
    memcpy((void *)reply->data, pack.data(), pack.size());
  }
  return OK;
}

int ExtractReply(char *buffer, int &return_code, int &numReturn,
                 const char **obj) {
  auto rc = OK;
  auto reply = (MessageReply *)buffer;
  ErrCheck(reply->header.len < sizeof(MessageReply), error, ErrRecvCode,
           "rely message length is error");
  ErrCheck(reply->header.typeCode != CODE_REPLY, error, ErrRecvCode,
           "rely message type code is error");

  return_code = reply->returnCode;
  numReturn = reply->numReturn;
  *obj = (0 == numReturn) ? nullptr : reply->data;

  return rc;
}

// TODO ---------未加入缓冲区检测
// 构建插入消息包
int BuildInsert(char *buffer, size_t *buffer_size, std::vector<uint8_t> &pack) {
  auto size = sizeof(MessageInsert) + pack.size();
  // buffere is allocated
  auto insert_value = (MessageInsert *)(buffer);
  // build header
  insert_value->header.len = size;
  insert_value->header.typeCode = CODE_INSERT;
  // build object
  insert_value->length = 1;
  *buffer_size = size;
  memcpy((void *)insert_value->data, pack.data(),
         pack.size() * sizeof(pack[0]));
  printf("len : %lu, CODE: %d\n", size, CODE_INSERT);
  return OK;
}

// 解析插入消息
int ExtractInsert(char *buffer, int &insert_num, char **objs) {
  auto rc = OK;
  auto message = (MessageInsert *)buffer;
  ErrCheck(message->header.len < sizeof(MessageInsert), error, ErrInvaildArg,
           "Invaild length of insert message");
  ErrCheck(message->header.typeCode != CODE_INSERT, error, ErrInvaildArg,
           "Invaild code of insert message");

  insert_num = message->length;
  *objs = (0 == insert_num) ? nullptr : message->data;
  return rc;
}

// 删除消息包构建
int BuildDelete(char *buffer, size_t *buffer_size, std::vector<uint8_t> &pack) {
  int rc = OK;
  auto size = sizeof(MessageDelete) + pack.size();
  // buffer is deallocated
  auto del_pack = (MessageDelete *)(buffer);
  // build header
  del_pack->header.typeCode = CODE_DELETE;
  del_pack->header.len = size;
  // build del key message
  *buffer_size = size;
  memcpy((void *)del_pack->key, pack.data(), pack.size());
  printf("len: %ld, CODE: %d\n", size, del_pack->header.typeCode);
  return rc;
}

// 删除消息包解析
int ExtractDelete(const char *buffer, nlohmann::json &key) {
  int rc = OK;
  auto message = (MessageDelete *)buffer;
  ErrCheck(message->header.len < sizeof(Header), error, ErrInvaildArg,
           "Invaild length of delete message");
  ErrCheck(message->header.typeCode != CODE_DELETE, error, ErrInvaildArg,
           "Invaild code of delete message");

  try {
    key = nlohmann::json::from_msgpack(message->key);
  } catch (nlohmann::json::exception const &e) {
    DB_LOG(error, "json parse from mesg pack fail!");
    rc = ErrPackParse;
  }

  return rc;
}

int BuildQuery(char *buffer, size_t *buffer_size, std::vector<uint8_t> &key) {
  auto rc = OK;
  auto size = sizeof(MessageQuery) + key.size();
  auto pack = (MessageQuery *)buffer;
  // build header
  pack->header.len = size;
  pack->header.typeCode = CODE_QUERY;
  *buffer_size = size;
  // json oject
  memcpy((void *)pack->key, key.data(), key.size());
  return rc;
}

int ExtractQuery(const char *buffer, nlohmann::json &key) {
  int rc = OK;
  auto message = (MessageQuery *)buffer;
  ErrCheck(message->header.len < sizeof(Header), error, ErrInvaildArg,
           "Invaild length of query message");
  ErrCheck(message->header.typeCode != CODE_QUERY, error, ErrInvaildArg,
           "Invaild code of query message");

  try {
    key = nlohmann::json::from_msgpack(message->key);
  } catch (nlohmann::json::exception const &e) {
    DB_LOG(error, "json parse from mesg pack fail!");
    rc = ErrPackParse;
  }

  return rc;
}

std::vector<uint8_t> JsonToMsgpack(nlohmann::json const &json_obj) {
  std::vector<uint8_t> pack;
  try {
    pack = nlohmann::json::to_msgpack(json_obj);
    pack.push_back(0);
  } catch (nlohmann::json::exception const &e) {
    DB_LOG(error, e.what());
  }
  return pack;
}

nlohmann::json MsgpackToJson(char *pack) {
  nlohmann::json json_obj;
  try {
    json_obj = nlohmann::json::from_msgpack(pack);
  } catch (nlohmann::json::exception const &e) {
    DB_LOG(error, e.what());
  }
  return json_obj;
}
