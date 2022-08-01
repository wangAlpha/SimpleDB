#pragma once

#include "core.hpp"
#include "hash_code.hpp"
#include "logging.hpp"
#include "record.hpp"

constexpr size_t IXM_HASH_MAP_SIZE = 1117;
extern const char *DMS_KEY_FIELDNAME;
struct ElementHash {
  std::vector<char> data;
  DmsRecordID record_id;
};

class BucketManager {
 private:
  class Bucket {
   private:
    // the map is hashNum and eleHash
    std::multimap<unsigned int, ElementHash> bucket_map_;
    mutable boost::shared_mutex shared_mutex_;

   public:
    // get the record whether exist
    int is_ID_exist(uint16_t num, ElementHash &element);
    int create_index(uint16_t num, ElementHash &element);
    int find_index(uint16_t num, ElementHash &element);
    int remove_index(uint16_t num, ElementHash &element);
  };

  std::vector<std::shared_ptr<Bucket>> buckets_;
  // process data
  int process_data(nlohmann::json const &record,
                   DmsRecordID const &record_id,  // in
                   uint16_t &hash_num,            // out
                   ElementHash &element, uint16_t &random);

 public:
  BucketManager() {}
  ~BucketManager() {}

  int initialize();
  int is_ID_exist(nlohmann::json const &record);
  int create_index(nlohmann::json const &record, DmsRecordID &record_id);
  int find_index(nlohmann::json const &record, DmsRecordID &record_id);
  int remove_index(nlohmann::json const &record, DmsRecordID &record_id);
};

int BucketManager::initialize() {
  auto rc = OK;
  for (size_t i = 0; i < IXM_HASH_MAP_SIZE; ++i) {
    buckets_.emplace_back(std::make_shared<Bucket>());
  }
  return rc;
}

int BucketManager::is_ID_exist(nlohmann::json const &record) {
  uint16_t hash_num = 0;
  // bucket index
  uint16_t random = 0;
  ElementHash hash_element;
  DmsRecordID record_id;
  auto rc = process_data(record, record_id, hash_num, hash_element, random);
  ErrCheck(rc, error, ErrSys, "Failed to process data");
  rc = buckets_[random]->is_ID_exist(hash_num, hash_element);
  ErrCheck(rc, error, ErrSys, "Failed to create index");
  return rc;
}

int BucketManager::create_index(nlohmann::json const &record,
                                DmsRecordID &record_id) {
  uint16_t hash_num = 0;
  uint16_t random = 0;
  ElementHash element;
  auto rc = process_data(record, record_id, hash_num, element, random);
  DB_CHECK(rc, error, "Failed to process data");
  rc = buckets_[random]->create_index(hash_num, element);
  DB_CHECK(rc, error, "Failed to create index");
  record_id = element.record_id;
  return rc;
}

int BucketManager::find_index(nlohmann::json const &record,
                              DmsRecordID &record_id) {
  auto rc = OK;
  uint16_t hash_num = 0;
  uint16_t random = 0;
  ElementHash element;
  rc = process_data(record, record_id, hash_num, element, random);
  DB_CHECK(rc, error, "Failed to process data");
  rc = buckets_[random]->find_index(hash_num, element);
  DB_CHECK(rc, error, "Failed to find index");
  record_id = element.record_id;
  return rc;
}

int BucketManager::remove_index(nlohmann::json const &record,
                                DmsRecordID &record_id) {
  auto rc = OK;
  uint16_t hash_num = 0;
  uint16_t random = 0;
  ElementHash element;
  rc = process_data(record, record_id, hash_num, element, random);
  DB_CHECK(rc, error, "Failed to process data");
  rc = buckets_[random]->remove_index(hash_num, element);
  DB_CHECK(rc, error, "Failed to remove index");
  // return record_id
  record_id.page_id = element.record_id.page_id;
  record_id.slot_id = element.record_id.slot_id;
  return rc;
}

// get hash code and record data
int BucketManager::process_data(nlohmann::json const &record,
                                DmsRecordID const &record_id,
                                uint16_t &hash_num, ElementHash &hash_element,
                                uint16_t &random) {
  auto rc = OK;
  std::string element;
  try {
    element = record[DMS_KEY_FIELDNAME];
  } catch (nlohmann::json::exception const &e) {
    DB_LOG(error, e.what());
    return ErrInvaildArg;
  }
  hash_num = HashCode(element.c_str(), element.size());
  random = hash_num % IXM_HASH_MAP_SIZE;
  std::copy(element.begin(), element.end(),
            std::back_inserter(hash_element.data));
  hash_element.record_id = record_id;
  return rc;
}

// find hash element in bucket
int BucketManager::Bucket::is_ID_exist(uint16_t const hash_num,
                                       ElementHash &element) {
  auto rc = OK;
  boost::shared_lock<boost::shared_mutex> read_lock(shared_mutex_);
  auto ret = bucket_map_.equal_range(hash_num);
  nlohmann::json source_element;
  // compare key
  for (auto it = ret.first; it != ret.second; ++it) {
    auto exist_element = it->second;
    if (exist_element.data.size() == element.data.size() &&
        exist_element.data == element.data) {
      rc = ErrIDExist;
      DB_LOG(error, "record id does exist");
      break;
    }
  }
  return rc;
}

int BucketManager::Bucket::create_index(uint16_t hash_num,
                                        ElementHash &element) {
  auto rc = OK;
  // write lock
  boost::unique_lock<boost::shared_mutex> write_lock(shared_mutex_);
  bucket_map_.insert(std::pair<uint16_t, ElementHash>(hash_num, element));
  return rc;
}

int BucketManager::Bucket::find_index(uint16_t hash_num, ElementHash &element) {
  auto rc = OK;
  boost::shared_lock<boost::shared_mutex> read_lock(shared_mutex_);
  auto ret = bucket_map_.equal_range(hash_num);
  rc = ErrIDNotExist;
  for (auto it = ret.first; it != ret.second; ++it) {
    auto exist_element = it->second;
    if (exist_element.data.size() == element.data.size() &&
        exist_element.data == element.data) {
      element.record_id = exist_element.record_id;
      rc = OK;
      break;
    }
  }
  DB_CHECK(rc, error, "record id doest not exist");
  return rc;
}

int BucketManager::Bucket::remove_index(uint16_t hash_num,
                                        ElementHash &element) {
  auto rc = OK;
  boost::unique_lock<boost::shared_mutex> write_lock(shared_mutex_);
  auto ret = bucket_map_.equal_range(hash_num);
  for (auto it = ret.first; it != ret.second; ++it) {
    auto exist_element = it->second;
    if (exist_element.data == element.data) {
      element.record_id = exist_element.record_id;
      bucket_map_.erase(it);
      return rc;
    }
  }
  rc = ErrInvaildArg;
  DB_LOG(error, "record _id does not exist");
  return rc;
}
