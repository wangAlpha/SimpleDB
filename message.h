#pragma once

#include "core.h"
#include "logging.h"

#define CODE_REPLY 1
#define CODE_QUERY 2
#define CODE_INSERT 3
#define CODE_DELETE 4
#define CODE_SNAPSHOT 5
#define CODE_SHUTDOWN 6
#define CODE_RETURN 0
// for convenience

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
  uint32_t key[0];
};

struct MessageQuery {
  Header header;
  uint32_t key[0];
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
// remove Tab and Enter character
std::string Extract_Buffer(char *argc, size_t length) {
  std::string buffer(argc, argc + length);
  buffer.erase(std::remove(buffer.begin(), buffer.end(), kTab_Char));
  buffer.erase(std::remove(buffer.begin(), buffer.end(), kEnter_Char));
  std::vector<uint8_t> buf(buffer.begin(), buffer.end());
  auto header = (Header *)buf.data();
  printf("len: %d type:Code: %d \n", header->len, header->typeCode);
  try {
    auto insert_value = (MessageInsert *)buf.data();
    auto j = nlohmann::json::from_msgpack((char *)&insert_value->data[0]);
    std::cout << j.dump() << j.size() << std::endl;
  } catch (nlohmann::json::exception const &e) {
    DB_LOG_TRIVAIL(error, e.what()) << std::endl;
  }
  return buffer;
}

int BuildReply(char *buffer, int *buffer_size, int returnCode,
               std::vector<uint8_t> &pack) {
  MessageReply *reply = nullptr;
  auto size = sizeof(MessageReply);
  if (pack.size() != 0) {
    // serialize to MessagePack
    size += pack.size();
  }
  reply = (MessageReply *)(*buffer);
  // build header
  reply->header.len = size;
  reply->header.typeCode = CODE_REPLY;
  // build body
  reply->returnCode = returnCode;
  reply->numReturn = (pack.size() == 0);

  // json object
  if (pack.size()) {
    memcpy((void *)&reply->data[0], pack.data(), pack.size());
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
  memcpy(&insert_value->data[0], pack.data(), pack.size() * sizeof(pack[0]));
  printf("len : %d, CODE: %d\n", size, CODE_INSERT);
  return OK;
}

// 解析插入消息
int ExtractInsert(char *buffer, int &insert_num, const char **objs) {
  auto rc = OK;
  auto message = (MessageInsert *)buffer;
  if (message->header.len < sizeof(MessageInsert)) {
    DB_LOG_TRIVAIL(error, "Invaild length of insert message");
    rc = ErrInvaildArg;
    goto done;
  }

  if (message->header.typeCode != CODE_INSERT) {
    DB_LOG_TRIVAIL(error, "Invaild code of insert message");
    rc = ErrInvaildArg;
    goto done;
  }
  insert_num = message->length;
  *objs = (0 == insert_num) ? nullptr : (char *)&message->data[0];
done:
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
  // build del key
  memcpy((void *)&del_pack->key[0], pack.data(), pack.size());
  printf("len: %ld, CODE: %d\n", size, del_pack->header.typeCode);
  return OK;
}

// 删除消息包解析
int ExtractDelete(char *buffer, nlohmann::json &key) {
  int rc = OK;
  auto message = (MessageDelete *)buffer;
  if (message->header.len < sizeof(Header)) {
    DB_LOG_TRIVAIL(error, "Invaild length of delete message");
    rc = ErrInvaildArg;
  }
  if (message->header.typeCode != CODE_DELETE) {
    DB_LOG_TRIVAIL(error, "Invaild code of delete message");
    rc = ErrInvaildArg;
    goto done;
  }
  try {
    key = nlohmann::json::from_msgpack((char *)&message->key[0]);
  } catch (nlohmann::json::exception const &e) {
    DB_LOG_TRIVAIL(error, "json parse from mesg pack fail!");
    rc = ErrPackParse;
    goto done;
  }
done:
  return rc;
}

int BuildQuery(char *buffer, size_t *buffer_size, std::vector<uint8_t> &key) {
  auto rc = OK;
  auto size = sizeof(MessageQuery) + key.size();
  // check buffer size
  // TODO
  auto pack = (MessageQuery *)buffer;
  // build header
  pack->header.len = size;
  pack->header.typeCode = CODE_QUERY;
  // json oject
  memcpy((void *)&pack->key[0], key.data(), key.size());
  return rc;
}

int ExtractQuery(char *buffer, nlohmann::json &key) {
  int rc = OK;
  auto message = (MessageQuery *)buffer;
  if (message->header.len < sizeof(Header)) {
    DB_LOG_TRIVAIL(error, "Invaild length of query message");
    rc = ErrInvaildArg;
  }
  if (message->header.typeCode != CODE_QUERY) {
    DB_LOG_TRIVAIL(error, "Invaild code of query message");
    rc = ErrInvaildArg;
    goto done;
  }
  try {
    key = nlohmann::json::from_msgpack((char *)&message->key[0]);
  } catch (nlohmann::json::exception const &e) {
    DB_LOG_TRIVAIL(error, "json parse from mesg pack fail!");
    rc = ErrPackParse;
    goto done;
  }
done:
  return rc;
}

std::string Command_Handle(std::string &command) {
  // std::vector<std::string>> result;
  std::vector<std::string> text;

  return "";
}
