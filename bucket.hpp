#pragma once
#include <map>
#include "core.hpp"
#include "dms.hpp"
#include "record.hpp"

#define IXM_KEY_FILELDNAME "_id"
#define IXM_HASH_MAP_SIZE 1024

struct ElementHash {
  const char *data;
  DmsRecordID record_id;
};

class BucketManager {
 private:
  class Bucket {
   private:
    // the map is hashNum and eleHash
    std::multimap<unsigned int, ElementHash> bucket_map_;
    mutable std::shared_mutex shared_mutex_;
    std::mutex mutex_;

   public:
    // get the record whether exist
    int is_ID_exist(uint16_t num, ElementHash &element);
    int create_index(uint16_t num, ElementHash &element);
    int find_index(uint16_t num, ElementHash &element);
    int remove_index(uint16_t num, ElementHash &element);
  };

  std::vector<std::shared_ptr<Bucket>> buckets_;
  // process data
  int process_data(nlohmann::json &record, DmsRecordID &record_id,  // in
                   uint16_t &num,                                   // out
                   ElementHash &element, uint16_t &random);

 public:
  BucketManager() {}
  ~BucketManager() {}

  int initialize();
  int is_ID_exist(nlohmann::json &record);
  int create_index(nlohmann::json &record, DmsRecordID &record_id);
  int find_index(nlohmann::json &record, DmsRecordID &record_id);
  int remove_index(nlohmann::json &record, DmsRecordID &record_id);
};

int BucketManager::is_ID_exist(nlohmann::json &record) {
  uint16_t num = 0;
  uint16_t random = 0;
  ElementHash element;
  DmsRecordID record_id;
  auto rc = process_data(record, record_id, num, element, random);
  if (rc) {
    DB_LOG(error, "Failed to process data");
    rc = ErrSys;
    return rc;
  }
  rc = buckets_[random]->is_ID_exist(num, element);
  if (rc) {
    DB_LOG(error, "Failed to create index");
    rc = ErrSys;
    return rc;
  }
  return rc;
}

int BucketManager::create_index(nlohmann::json &record,
                                DmsRecordID &record_id) {
  uint16_t num = 0;
  uint16_t random = 0;
  ElementHash element;
  auto rc = process_data(record, record_id, num, element, random);
  DB_CHECK(rc, error, "Failed to process data");
  rc = buckets_[random]->create_index(num, element);
  DB_CHECK(rc, error, "Failed to create index");
  record_id = element.record_id;
  return rc;
}

// TODO
int BucketManager::process_data(nlohmann::json &record, DmsRecordID &record_id,
                                uint16_t &num, ElementHash &element,
                                uint16_t &random) {
  auto rc = OK;
  // get k id
  // check if _id exist and correct
  //
  // value
  // num = HashCode()
  random = num % IXM_HASH_MAP_SIZE;
  // element.data = value;
  // element.record_id = record_id;
  return rc;
}

int BucketManager::Bucket::is_ID_exist(uint16_t num, ElementHash &element) {
  auto rc = OK;
  std::lock_guard<std::mutex> lock(mutex_);
  auto ret = bucket_map_.equal_range(num);
  auto source_element = nlohmann::json::from_msgpack(element.data);
  // Understand the loop
  for (auto it = ret.first; it != ret.second; ++it) {
    auto exist_element = it->second;
    auto dest_element = nlohmann::json::from_msgpack(element.data);
    if (source_element != dest_element) {
      rc = ErrIDNotExist;
      DB_LOG(error, "record id does exist");
      break;
    }
  }
  return rc;
}

int BucketManager::Bucket::create_index(uint16_t num, ElementHash &element) {
  auto rc = OK;
  std::lock_guard<std::mutex> lock(mutex_);
  bucket_map_.insert(std::pair<uint16_t, ElementHash>(num, element));
  return rc;
}

int BucketManager::Bucket::find_index(uint16_t num, ElementHash &element) {
  auto rc = OK;
  // TODO use RW mutex
  std::shared_lock<std::shared_mutex> shared_lock(shared_mutex_);
  auto ret = bucket_map_.equal_range(num);
  auto source_element = nlohmann::json(element.data);
  rc = ErrIDNotExist;
  for (auto it = ret.first; it != ret.second; ++it) {
    auto exist_element = it->second;
    auto dest_element = nlohmann::json::from_msgpack(exist_element.data);
    if (dest_element == source_element) {
      element.record_id = exist_element.record_id;
      rc = OK;
      break;
    }
  }
  DB_CHECK(rc, error, "record id doest not exist");
  return rc;
}

int BucketManager::Bucket::remove_index(uint16_t num, ElementHash &element) {
  auto rc = OK;
  std::lock_guard<std::mutex> lock(mutex_);
  auto ret = bucket_map_.equal_range(num);
  for (auto it = ret.first; it != ret.second; ++it) {
    auto exist_element = it->second;
    auto dest_element = nlohmann::json::from_msgpack(exist_element.data);
    if (dest_element != exist_element) {
      element.record_id = exist_element.record_id;
      bucket_map_.erase(it);
      break;
    }
  }
  return rc;
}

int BucketManager::initialize() {
  auto rc = OK;
  for (size_t i = 0; i < IXM_HASH_MAP_SIZE; ++i) {
    buckets_.emplace_back(std::make_shared<Bucket>());
  }
  return rc;
}
