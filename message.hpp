#pragma once

#include "core.hpp"
#include "logging.hpp"

#define CODE_REPLY 1
#define CODE_QUERY 2
#define CODE_INSERT 3
#define CODE_DELETE 4
#define CODE_SNAPSHOT 5
#define CODE_SHUTDOWN 6
#define CODE_CONNECT 7
#define CODE_DISCONNECT 8

#define CODE_RETURN 0

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
               std::vector<uint8_t> &pack) {
  MessageReply *reply = nullptr;
  auto size = sizeof(MessageReply);
  if (pack.size() != 0) {
    // serialize to MessagePack
    size += pack.size();
  }
  reply = (MessageReply *)buffer;
  // build header
  reply->header.len = size;
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
  if (reply->header.len < sizeof(MessageReply)) {
    DB_LOG(error, "rely message length is error");
    rc = ErrRecvCode;
    goto error;
  }
  if (reply->header.typeCode != CODE_REPLY) {
    DB_LOG(error, "rely message type code is error");
    rc = ErrRecvCode;
    goto error;
  }
  return_code = reply->returnCode;
  numReturn = reply->numReturn;
  *obj = (0 == numReturn) ? nullptr : reply->data;

error:
  return rc;
}

// TODO ---------未加入缓冲区检测
// 构建插入消息包
int BuildInsert(char *buffer, size_t *buffer_size, std::vector<uint8_t> &pack) {
  // TODO
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
  if (message->header.len < sizeof(MessageInsert)) {
    DB_LOG(error, "Invaild length of insert message");
    rc = ErrInvaildArg;
    goto error;
  }

  if (message->header.typeCode != CODE_INSERT) {
    DB_LOG(error, "Invaild code of insert message");
    rc = ErrInvaildArg;
    goto error;
  }
  insert_num = message->length;
  *objs = (0 == insert_num) ? nullptr : (char *)&message->data[0];
error:
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
  if (message->header.len < sizeof(Header)) {
    DB_LOG(error, "Invaild length of delete message");
    rc = ErrInvaildArg;
    goto error;
  }
  if (message->header.typeCode != CODE_DELETE) {
    DB_LOG(error, "Invaild code of delete message");
    rc = ErrInvaildArg;
    goto error;
  }
  try {
    key = nlohmann::json::from_msgpack(message->key);
  } catch (nlohmann::json::exception const &e) {
    DB_LOG(error, "json parse from mesg pack fail!");
    rc = ErrPackParse;
    goto error;
  }

error:
  return rc;
}

int BuildQuery(char *buffer, size_t *buffer_size, std::vector<uint8_t> &key) {
  auto rc = OK;
  auto size = sizeof(MessageQuery) + key.size();
  // TODO check buffer size
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
  if (message->header.len < sizeof(Header)) {
    DB_LOG(error, "Invaild length of query message");
    rc = ErrInvaildArg;
    goto done;
  }
  if (message->header.typeCode != CODE_QUERY) {
    DB_LOG(error, "Invaild code of query message");
    rc = ErrInvaildArg;
    goto done;
  }
  try {
    key = nlohmann::json::from_msgpack(message->key);
  } catch (nlohmann::json::exception const &e) {
    DB_LOG(error, "json parse from mesg pack fail!");
    rc = ErrPackParse;
    goto done;
  }

done:
  return rc;
}
