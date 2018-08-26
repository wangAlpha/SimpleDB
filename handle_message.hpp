#pragma once

#include "core.hpp"
#include "message.hpp"
#include "pmd.hpp"

constexpr char kTab_Char = '\t';
constexpr char kEnter_Char = '\n';

// remove Tab and Enter character
int HandleMessage(char *argc, size_t length) {
  auto rc = OK;
  std::string buffer(argc, argc + length);
  // reomve 0x020 maigc number
  if (buffer[buffer.size()]) {
    buffer.pop_back();
  }

  auto header = (Header *)buffer.data();
  auto pack_len = header->len;
  auto header_code = header->typeCode;

  auto error_handle = [](auto rc) {
    switch (rc) {
      case ErrInvaildArg: {
        DB_LOG(error, "Argument error");
      } break;
      case ErrIDNotExist: {
        DB_LOG(error, "Record does not exist");
      } break;
      default:
        assert(false);
    }
    return rc;
  };

  // remove invalid message
  if (buffer.size() < sizeof(Header) || header->len < sizeof(Header)) {
    rc = ErrInvaildArg;
    return error_handle(rc);
  }
  // print a frame data
  auto print_pack = [&](auto &pack) {
	printf("len: %d type:Code: %d \n[", pack_len, header_code);
	std::for_each(pack.begin(), pack.end(),
                [](auto &e) { printf("%x ", e); });
    printf("]\n");
  };

  print_pack(buffer);
  auto pmd_manager = KRCB::get_pmd_manager();
  auto rtn = pmd_manager->get_rtn();

  try {
    if (CODE_INSERT == header_code) {
      int record_num = 0;
      char *data = nullptr;

      DB_LOG(debug, "Insert request received");
      rc = ExtractInsert((char *)buffer.data(), record_num, &data);
      if (rc) {
        DB_LOG(error, "Failed to read insert packet");
        rc = ErrInvaildArg;
        return error_handle(rc);
      }
      try {
        auto json_obj = nlohmann::json::from_msgpack(data);
        std::cout << json_obj.dump() << std::endl;
        rc = rtn->insert(json_obj);
      } catch (nlohmann::json::exception const &e) {
        DB_LOG(error, e.what());
        rc = ErrInvaildArg;
        return error_handle(rc);
      }
    } else if (CODE_QUERY == header_code) {
      int record_num = 0;
      nlohmann::json key;
      nlohmann::json value;
      DB_LOG(debug, "query request received");
      rc = ExtractQuery(buffer.data(), key);
      if (rc) {
        DB_LOG(error, "Failed to read query packet");
        rc = ErrInvaildArg;
        return error_handle(rc);
      }
      try {
        std::cout << key.dump() << std::endl;
        std::cout << "query condition" << key.dump() << std::endl;
        // query key
        rc = rtn->find(key, value);
      } catch (nlohmann::json::exception const &e) {
        DB_LOG(error, e.what());
        rc = ErrInvaildArg;
        return error_handle(rc);
      }
    } else if (CODE_DELETE == header_code) {
      // char *data = nullptr;
      nlohmann::json key;
      DB_LOG(debug, "delete request received");
      rc = ExtractDelete(buffer.data(), key);
      if (rc) {
        DB_LOG(error, "Failed to read delete packet");
        rc = ErrInvaildArg;
        return error_handle(rc);
      }
      try {
        std::cout << "delete condition" << std::endl;
        std::cout << key.dump() << std::endl;
        // remove key
        rc = rtn->remove(key);
      } catch (nlohmann::json::exception const &e) {
        DB_LOG(error, e.what());
        rc = ErrInvaildArg;
        return error_handle(rc);
      }
    } else if (CODE_SNAPSHOT == header_code) {
      try {
        int record_num = 0;
        char *data = nullptr;
        DB_LOG(debug, "snapshot request request");
      } catch (nlohmann::json::exception const &e) {
        DB_LOG(error, e.what());
        rc = ErrInvaildArg;
        return error_handle(rc);
      }
    }
  } catch (std::exception const &e) {
    DB_LOG(error, e.what());
  } catch (nlohmann::json::exception const &e) {
    DB_LOG(error, e.what());
  }
  return rc;
}
