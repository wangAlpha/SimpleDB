#pragma once

#include "core.hpp"
#include "message.hpp"
#include "pmd.hpp"

constexpr char kTab_Char = '\t';
constexpr char kEnter_Char = '\n';
extern std::atomic<bool> gQuit;

// message preprocess
int MesssagePreTreatment(char *argc, size_t length, std::string *out_buffer) {
  auto rc = OK;
  std::string buffer(argc, argc + length);
  // reomve 0x020 maigc number
  if (buffer[buffer.size() - 1] == MagicNumber) {
    buffer.pop_back();
  }

  auto header = (Header *)buffer.data();
  auto pack_len = header->len;
  auto header_code = header->typeCode;

  // remove invalid message
  if (buffer.size() < sizeof(Header) || header->len < sizeof(Header)) {
    return ErrCommand;
  }
  // print a frame data
  printf("len: %d type:Code: %d \n[", pack_len, header_code);
  std::for_each(buffer.begin(), buffer.end(),
                [](auto &e) { printf("%x ", e); });
  printf("]\n");

  *out_buffer = buffer;
  return rc;
}

// remove Tab and Enter character
int HandleMessage(char *argc, size_t *length, bool *disconnect,
                  std::vector<uint8_t> *reply_message) {
  auto rc = OK;
  std::string buffer;
  if (MesssagePreTreatment(argc, *length, &buffer) != OK) {
    return OK;
  }

  auto header = (Header *)buffer.data();
  auto header_code = header->typeCode;
  auto pmd_manager = KRCB::get_pmd_manager();
  auto rtn = pmd_manager->get_rtn();
  nlohmann::json return_json;

  try {
    switch (header_code) {
      case CODE_INSERT: {
        int record_num = 0;
        char *data = nullptr;
        DB_LOG(debug, "Insert request received");
        rc = ExtractInsert((char *)buffer.data(), record_num, &data);
        ErrCheck(rc, error, ErrInvaildArg, "Failed to read insert packet");
        auto json_obj = MsgpackToJson(data);
        // insert key-value
        std::cout << "insert object: " << json_obj.dump() << std::endl;
        rc = rtn->insert(json_obj);
        std::cout << "Insert Successful!" << std::endl;
      } break;
      case CODE_QUERY: {
        nlohmann::json key;
        DB_LOG(debug, "query request received");
        rc = ExtractQuery(buffer.data(), key);
        ErrCheck(rc, error, ErrInvaildArg, "Failed to read query packet");
        std::cout << "query condition: " << key.dump() << std::endl;
        // query key
        rc = rtn->find(key, return_json);
      } break;
      case CODE_DELETE: {
        DB_LOG(debug, "delete request received");
        nlohmann::json key;
        rc = ExtractDelete(buffer.data(), key);
        ErrCheck(rc, error, ErrInvaildArg, "Failed to read delete packet");
        std::cout << "delete condition: " << key.dump() << std::endl;
        // remove key
        rc = rtn->remove(key);
      } break;
      case CODE_SNAPSHOT: {
        DB_LOG(debug, "snapshot request");
        return_json = {{"insert_time", 100},
                       {"del_time", 100},
                       {"query_time", 100},
                       {"run_time", 100}};
      } break;
      case CODE_DISCONNECT: {
        DB_LOG(debug, "quit request packet");
        *disconnect = true;
      } break;
      case CODE_SHUTDOWN: {
        DB_LOG(info, "shutodown...");
        *disconnect = true;
        gQuit = true;
      } break;
      default:
        break;
    }
  } catch (std::exception const &e) {
    DB_LOG(error, e.what());
    rc = ErrInvaildArg;
  } catch (nlohmann::json::exception const &e) {
    DB_LOG(error, e.what());
    rc = ErrInvaildArg;
  }
  *length = SEND_BUFFER_SIZE;
  char pack_buffer[SEND_BUFFER_SIZE] = {0};
  memset((void *)pack_buffer, 0, sizeof(pack_buffer));
  if (rc != 0) {
    return_json = R"({"_id":"key","value": "error"})"_json;
  }
  switch (header_code) {
    case CODE_QUERY:
    case CODE_SNAPSHOT:
      BuildReply((char *)reply_message->data(), (int *)length, rc, return_json);
      std::cout << "len: " << *length << "json: " << return_json.dump() << '\n';
      break;
    default: {
      BuildReply((char *)reply_message->data(), (int *)length, rc, return_json);
    } break;
  }
  return rc;
}
