#pragma once

#include "bucket.hpp"
#include "core.hpp"
#include "dms.hpp"

// run time, provide query select and delete operate
class RunTime {
 private:
  std::shared_ptr<BucketManager> bucket_manager_;
  std::shared_ptr<DmsFile> dms_file_;

 public:
  RunTime() {}
  ~RunTime() {}

  int initialzie(const char *file_name);
  int insert(nlohmann::json &reocrd);
  int find(nlohmann::json &record, nlohmann::json &out);
  int remove(nlohmann::json &record);
};

int RunTime::initialzie(const char *file_name) {
  auto rc = OK;
  bucket_manager_ = std::make_shared<BucketManager>();
  ErrCheck(!bucket_manager_, error, ErrSys, "Failed to new bucket manager");
  // use dms initialize to load database index
  dms_file_ = std::make_shared<DmsFile>(bucket_manager_);
  ErrCheck(!dms_file_, error, ErrSys, "Failed to new dms file");
  rc = bucket_manager_->initialize();
  DB_CHECK(rc, error, "Failed to initialize bucket manager");
  rc = dms_file_->initialize(file_name);
  DB_CHECK(rc, error, "Failed to initialize dms file");
  return rc;
}

int RunTime::insert(nlohmann::json &record) {
  auto rc = bucket_manager_->is_ID_exist(record);
  DB_CHECK(rc, error, "Failed to call is_id_exist");
  DmsRecordID record_id;
  nlohmann::json out_record;
  rc = dms_file_->insert(record, out_record, record_id);
  DB_CHECK(rc, error, "Failed to call dms insert ");
  rc = bucket_manager_->create_index(out_record, record_id);
  ErrCheck(rc, error, ErrSys, "Failed to call create_index");
  return rc;
}

int RunTime::find(nlohmann::json &record, nlohmann::json &out_record) {
  DmsRecordID record_id;
  auto rc = bucket_manager_->find_index(record, record_id);
  DB_CHECK(rc, error, "Failed to call bucket find index");
  rc = dms_file_->find(record_id, out_record);
  DB_CHECK(rc, error, "Failed to call dms find");
  return rc;
}

int RunTime::remove(nlohmann::json &record) {
  DmsRecordID record_id;
  auto rc = bucket_manager_->remove_index(record, record_id);
  DB_CHECK(rc, error, "Failed to call manager remove index");
  rc = dms_file_->remove(record_id);
  DB_CHECK(rc, error, "Failed to call dms remove");
  return rc;
}
