#pragma once

#include "bucket.hpp"
#include "core.hpp"
#include "logging.hpp"
#include "message.hpp"
#include "mmapfile.hpp"
#include "record.hpp"
struct DmsRecord {
  uint32_t size;
  uint32_t flag;
  char data[0];
};

// dms header
const char *DMS_HEADER_EYECATCHER = "DMSH";
constexpr size_t DMS_HEADER_EYECATCHER_LEN = 4;
constexpr uint32_t DMS_HEADER_FLAG_NORMAL = 0;
constexpr uint32_t DMS_HEADER_FLAG_DROPPED = 1;
struct DmsHeader {
  char eye_catcher[DMS_HEADER_EYECATCHER_LEN];
  uint32_t size;
  uint16_t flag;
  uint16_t version;
};

// page structure
/*-------------------------
PAGE STRUCTURE
------------------
| PAGE HEADER    |
------------------
| Slot List      |
------------------
| Free Space     |
------------------
| Data           |
------------------
--------------------------*/
using SlotOff = unsigned int;
constexpr size_t DMS_EXTEND_SIZE = 65536ul;
// 4MB for page size
constexpr size_t DMS_PAGE_SIZE = 1024 * 1024ul * 4;
constexpr size_t DMS_MAX_RECORD =
    (DMS_PAGE_SIZE - sizeof(DmsHeader) - sizeof(DmsRecord) - sizeof(SlotOff));
constexpr size_t DMS_MAX_PAGES = 1024 * 256ul;
constexpr SlotOff DMS_INVALID_SLOTID = 0xFFFFFFFF;
constexpr SlotOff DMS_INVALID_PAGEID = 0xFFFFFFFF;

const char *DMS_KEY_FIELDNAME = "_id";

// each record has the following header, include 4 bytes size and 4 bytes flag
constexpr uint32_t DMS_RECORD_FLAG_NORMAL = 0;
constexpr uint32_t DMS_RECORD_FLAG_DROPPED = 1;

constexpr uint32_t DMS_HEADER_VERSION = 0;
constexpr uint32_t DMS_HEADER_VERSION_CURRENT = DMS_HEADER_VERSION;

const char *DMS_PAGE_EYECATCHER = "PAGH";
constexpr size_t DMS_PAGE_EYECATCHER_LEN = 4;
constexpr uint32_t DMS_PAGE_FLAG_NORMAL = 0;
constexpr uint32_t DMS_PAGE_FLAG_UNALLOC = 1;
constexpr SlotID DMS_SLOT_EMPTY = 0xFFFFFFFF;

struct DmsPageHeader {
  char eye_catcher[DMS_PAGE_EYECATCHER_LEN];
  uint32_t size;
  uint32_t flag;
  uint32_t num_slots;
  uint32_t slot_offset;
  uint32_t free_space;
  uint32_t free_offset;
  char data[0];
};

constexpr size_t DMS_FILE_SEGMENT_SIZE = 1024 * 1024ul * 128;
constexpr size_t DMS_FILE_HEADER_SIZE = 1024ul * 64;
constexpr size_t DMS_PAGES_PER_SEGMENT = DMS_FILE_SEGMENT_SIZE / DMS_PAGE_SIZE;
constexpr size_t DMS_MAX_SEGMENTS =
    DMS_PAGES_PER_SEGMENT / DMS_PAGES_PER_SEGMENT;

// Data management system
class DmsFile : public MmapFile {
 public:
  explicit DmsFile(std::shared_ptr<BucketManager> &bucket_manager)
      : header_(nullptr), bucket_manager_(bucket_manager) {}
  ~DmsFile() {}

  int initialize(const char *filename);
  int insert(nlohmann::json const &record, nlohmann::json &out_record,
             DmsRecordID &rid);
  int remove(DmsRecordID const &rid);
  int find(DmsRecordID const &rid, nlohmann::json &result);

  size_t getNumSegments() const { return segments_.size(); }
  size_t getNumPages() const {
    return getNumSegments() * DMS_PAGES_PER_SEGMENT;
  }

  char *pageToOffset(PageID page_id) {
    if (page_id >= getNumPages()) {
      return nullptr;
    } else {
      return segments_[page_id / DMS_PAGES_PER_SEGMENT] +
             DMS_PAGE_SIZE * (page_id % DMS_PAGES_PER_SEGMENT);
    }
  }

  bool valid_size(size_t size) {
    if (size < DMS_FILE_HEADER_SIZE) {
      return false;
    }
    size -= DMS_FILE_HEADER_SIZE;
    return (size % DMS_FILE_SEGMENT_SIZE != 0) ? false : true;
  }

 private:
  DmsHeader *header_;
  // store file segement
  std::vector<char *> segments_;
  // free space to page id map
  std::multimap<uint16_t, PageID> free_space_map_;
  // RW mutex
  boost::shared_mutex shared_mutex_;
  std::string file_name_;
  std::shared_ptr<BucketManager> bucket_manager_;

  // create a new segment for the current file
  int extend_segment();
  // init from empty file, creating header only
  int init_new();
  // extend the file for given bytes
  int extend_file(int const size);
  // load data from beginning
  int map_segments();
  // load data to index
  int load_data(char *segment);
  // search slot
  int search_slot(char *page, DmsRecordID const &record_id, SlotOff &slot);
  // recover space
  void recover_space(char *page);
  // update free space
  void update_free_space(DmsPageHeader *header, int change_size,
                         PageID page_id);
  // find a page id to insert, return invalid page id if there's no page cache
  // found for required size bytes
  PageID find_page(size_t required_size);
};

int DmsFile::insert(nlohmann::json const &record, nlohmann::json &out_record,
                    DmsRecordID &rid) {
  auto rc = OK;

  auto record_pack = JsonToMsgpack(record);
  auto record_size = record_pack.size();
  ErrCheck(record_size == 0, error, ErrPackParse, "json serilize fail");
  // check record size
  ErrCheck(record_size > DMS_MAX_RECORD, error, ErrInvaildArg,
           "record cannot bigger than 4MB");
  // make sure _id exists
  // ErrCheck(record.find(DMS_KEY_FIELDNAME) == record.end(), error,
  // ErrInvaildArg, "record must be with _id");
  boost::unique_lock<boost::shared_mutex> write_lock(shared_mutex_);
retry:
  auto page_id = find_page(record_size + sizeof(DmsRecord));
  if (DMS_INVALID_PAGEID == page_id) {
    // try to extend segment
    rc = extend_segment();
    ErrCheck(rc, error, ErrSys, "Failed to extend segment");
    goto retry;
  }

  // find the in-memeory offset for the page
  auto page = pageToOffset(page_id);
  ErrCheck(page == nullptr, error, ErrSys, "Failed to find the page");
  // set page header
  auto page_header = (DmsPageHeader *)page;
  ErrCheck((memcmp((void *)page_header, DMS_PAGE_EYECATCHER,
                   DMS_PAGE_EYECATCHER_LEN) != 0),
           error, ErrSys, "Invalid page header");
  // if there's no free space excluding holes
  if ((page_header->free_space >
       page_header->free_offset - page_header->slot_offset) &&
      (page_header->slot_offset + record_size + sizeof(DmsRecord) +
           sizeof(SlotID) >
       page_header->free_offset)) {
    // recover empty hold from page
    recover_space(page);
  }
  // slots offset is the last bytes of slots;
  // free offset is the first byte of data
  // so freeOffset - slotOffset is the actual free space excluding holes
  if ((page_header->free_space <
       record_size + sizeof(DmsHeader) + sizeof(SlotID)) ||
      (page_header->free_offset - page_header->slot_offset <
       record_size + sizeof(DmsRecord) + sizeof(SlotID))) {
    DB_LOG(error, "Something big wrong!!");
    rc = ErrSys;
    return rc;
  }
  auto offset_tmp = page_header->free_offset - record_size - sizeof(DmsRecord);
  DmsRecord record_header;
  record_header.size = record_size + sizeof(DmsRecord);
  record_header.flag = DMS_RECORD_FLAG_NORMAL;
  // copy the slot
  *(SlotOff *)(page + sizeof(DmsPageHeader) +
               page_header->num_slots * sizeof(SlotOff)) = offset_tmp;
  // copy the header
  // FIX
  memcpy(page + offset_tmp, (char *)&record_header, sizeof(DmsRecord));
  // copy the body
  auto store_postion = page + offset_tmp + sizeof(DmsRecord);
  memcpy(store_postion, (char *)record_pack.data(), record_size);
  out_record = MsgpackToJson(store_postion);

  rid.page_id = page_id;
  rid.slot_id = page_header->num_slots;
  // modify metadata in page
  page_header->num_slots++;
  page_header->slot_offset += sizeof(SlotID);
  page_header->free_offset = offset_tmp;
  // modify database metadata
  update_free_space(page_header,
                    -(record_size + sizeof(SlotID) + sizeof(DmsRecord)),
                    page_id);
  return rc;
}

int DmsFile::remove(DmsRecordID const &rid) {
  auto rc = OK;

  boost::unique_lock<boost::shared_mutex> write_lock(shared_mutex_);
  // find the page in memory
  auto page = pageToOffset(rid.page_id);
  ErrCheck(!page, error, ErrSys, "Failed to find the page");
  // search the given slot
  SlotOff slot = 0;
  rc = search_slot(page, rid, slot);
  ErrCheck(rc, error, ErrSys, "Failed to searc slot");
  ErrCheck((DMS_SLOT_EMPTY == slot), error, ErrSys, "The record is dropped");

  // set page header
  auto page_header = (DmsPageHeader *)page;
  // set slot to empty
  *(SlotID *)(page + sizeof(DmsPageHeader) + rid.slot_id * sizeof(SlotID)) =
      DMS_SLOT_EMPTY;
  // set record header
  auto record_header = (DmsRecord *)(page + slot);
  record_header->flag = DMS_RECORD_FLAG_DROPPED;
  // update database metadata
  update_free_space(page_header, record_header->size, rid.page_id);
  // done:
  return rc;
}

int DmsFile::find(DmsRecordID const &rid, nlohmann::json &result) {
  auto rc = OK;
  // use read lock the database
  boost::shared_lock<boost::shared_mutex> read_lock(shared_mutex_);
  // goto the page and verify the slot is valid
  auto page = pageToOffset(rid.page_id);
  ErrCheck((!page), error, ErrSys, "Failed to find the page");
  // if slot is empty, something big wrong
  SlotOff slot;
  rc = search_slot(page, rid, slot);
  ErrCheck((DMS_SLOT_EMPTY == slot), error, ErrSys, "Failed to find the page");
  // get the record header
  auto record_header = (DmsRecord *)(page + slot);
  // if record_header->flag is dropped, this record is dropped already
  ErrCheck((DMS_RECORD_FLAG_DROPPED == record_header->flag), error, ErrSys,
           "The data is dropped");
  auto data = page + slot + sizeof(DmsRecord);
  result = MsgpackToJson(data);
  return rc;
}

void DmsFile::update_free_space(DmsPageHeader *header, int chang_size,
                                PageID page_id) {
  auto free_space = header->free_space;
  auto ret = free_space_map_.equal_range(free_space);

  for (auto it = ret.first; it != ret.second; ++it) {
    if (it->second == page_id) {
      free_space_map_.erase(it);
      break;
    }
  }
  // increase page free space
  free_space += chang_size;
  header->free_space = free_space;
  free_space_map_.insert(std::pair<uint16_t, PageID>(free_space, page_id));
}

int DmsFile::initialize(const char *file_name) {
  int rc = OK;
  file_name_ = file_name;
  rc = Open(file_name);
  DB_CHECK(rc, error, "Failed to open file");
getfilesize:
  // get file size
  auto offset = file_handle_.getSize();
  // if file size is 0, that means it's newly created file and we should
  // initailize it
  if (!offset) {
    rc = init_new();
    ErrCheck(rc, error, ErrSys, "Failed to initialize file");
    goto getfilesize;
  }
  rc = map_segments();
  ErrCheck(rc, error, ErrSys, "Failed to load data");

  return rc;
}

// extend a new segment
int DmsFile::extend_segment() {
  // extend a new segment
  auto rc = OK;
  auto offset = file_handle_.getSize();

  // first let's get the size of file before extend
  rc = extend_file(DMS_FILE_SEGMENT_SIZE);
  DB_CHECK(rc, error, "Failed to extend segment");

  // map from original end to new end
  char *segment = nullptr;
  rc = Map(offset, DMS_FILE_SEGMENT_SIZE, (void **)&segment);

  // create page header structure and we are going to copy to each page
  DmsPageHeader page_header;
  strcpy(page_header.eye_catcher, DMS_PAGE_EYECATCHER);
  page_header.size = (uint16_t)DMS_PAGE_SIZE;
  page_header.flag = DMS_PAGE_FLAG_NORMAL;
  page_header.num_slots = 0;
  page_header.slot_offset = sizeof(DmsPageHeader);
  page_header.free_space = (DMS_PAGE_SIZE - sizeof(DmsPageHeader));
  page_header.free_offset = DMS_PAGE_SIZE;
  // copy header to each page
  for (size_t i = 0; i < DMS_FILE_SEGMENT_SIZE; i += DMS_PAGE_SIZE) {
    memcpy(segment + i, (char *)&page_header, sizeof(DmsPageHeader));
  }

  // free space handling
  auto free_map_size = free_space_map_.size();
  // insert into free space map
  for (size_t i = 0; i < DMS_PAGES_PER_SEGMENT; ++i) {
    free_space_map_.insert(
        std::make_pair(page_header.free_space, i + free_map_size));
  }
  // push the segment into body list
  segments_.push_back(segment);
  header_->size += DMS_PAGES_PER_SEGMENT;
  return rc;
}

// Create a new header in file
int DmsFile::init_new() {
  // initialize a newly created file, let's append DMS_FILE_HEADER_SIZE bytes
  // and then initialize the header
  auto rc = extend_file(DMS_FILE_HEADER_SIZE);
  ErrCheck(rc, error, ErrSys, "Failed to extend file");

  rc = Map(0, (size_t)DMS_FILE_HEADER_SIZE, (void **)&header_);
  ErrCheck(rc, error, ErrSys, "Failed to map");

  strcpy(header_->eye_catcher, DMS_HEADER_EYECATCHER);
  header_->size = 0;
  header_->flag = DMS_HEADER_FLAG_NORMAL;
  header_->version = DMS_HEADER_VERSION_CURRENT;
  return rc;
}

// find a page in free pages
PageID DmsFile::find_page(size_t require_size) {
  auto iter = free_space_map_.upper_bound(require_size);
  return (iter != free_space_map_.end()) ? iter->second : DMS_INVALID_PAGEID;
}

// extrend a segment size space in file
int DmsFile::extend_file(int const size) {
  auto rc = OK;
  char temp[DMS_EXTEND_SIZE] = {0};
  memset(temp, 0, sizeof(temp));
  ErrCheck((size % DMS_EXTEND_SIZE != 0), error, ErrSys, "Invalid extend size");
  // write file
  for (size_t i = 0; i < (size_t)size; i += DMS_EXTEND_SIZE) {
    file_handle_.seekToEnd();
    auto len = file_handle_.Write(temp, DMS_EXTEND_SIZE);
    ErrCheck(len != DMS_EXTEND_SIZE, error, ErrSys, "Failed to write to file");
  }
  return rc;
}

int DmsFile::search_slot(char *page, DmsRecordID const &rid, SlotOff &slot) {
  auto rc = OK;
  ErrCheck((!page), error, ErrSys, "page is nullptr");
  // let's first verify the rid is valid
  ErrCheck((0 > rid.page_id || 0 > rid.slot_id), error, ErrSys, "Invaild RID");
  auto page_header = (DmsPageHeader *)page;
  ErrCheck((rid.slot_id > page_header->num_slots), error, ErrSys,
           "Slot is out of range, provided");
  slot = *(SlotOff *)(page + sizeof(DmsPageHeader) +
                      rid.slot_id * sizeof(SlotOff));
  return rc;
}
// ERROR!
// load index to bucket manager
int DmsFile::load_data(char *segment) {
  auto rc = OK;
  for (size_t i = 0; i < DMS_PAGES_PER_SEGMENT; ++i) {
    auto page_header = (DmsPageHeader *)(segment + i * DMS_PAGE_SIZE);
    free_space_map_.insert(
        std::pair<unsigned int, PageID>(page_header->free_space, i));
    auto slot_id = (SlotID)page_header->num_slots;
    DmsRecordID record_id;
    record_id.page_id = (PageID)i;
    for (size_t s = 0; s < slot_id; ++s) {
      auto slot_offset =
          *(SlotOff *)(segment + i * DMS_PAGE_SIZE + sizeof(DmsPageHeader) +
                       s * sizeof(SlotID));
      if (DMS_SLOT_EMPTY == slot_offset) {
        continue;
      }
      auto data = segment + i * DMS_PAGE_SIZE + slot_offset + sizeof(DmsRecord);
      auto json_obj = MsgpackToJson(data);
      record_id.slot_id = (SlotID)s;
      rc = bucket_manager_->is_ID_exist(json_obj);
      ErrCheck(rc, info, ErrSys, "Failed to call is_ID_exist");
      rc = bucket_manager_->create_index(json_obj, record_id);
      ErrCheck(rc, info, ErrSys, "Failed to call create index");
    }
  }
  return rc;
}

int DmsFile::map_segments() {
  auto rc = OK;
  if (!header_) {
    rc = Map(0, (size_t)DMS_FILE_HEADER_SIZE, (void **)&header_);
    ErrCheck(rc, error, ErrSys,
             "Failed to load data, partial segment detected");
  }
  auto num_page = header_->size;
  ErrCheck((num_page % DMS_PAGES_PER_SEGMENT), error, ErrSys,
           "Failed to load data, partial segment detected");
  auto num_segments = num_page / DMS_PAGES_PER_SEGMENT;
  if (num_segments > 0) {
    for (size_t i = 0; i < (size_t)num_segments; ++i) {
      // map each segment into memory
      char *segment = nullptr;
      rc = Map(DMS_FILE_HEADER_SIZE + DMS_FILE_SEGMENT_SIZE * i,
               DMS_FILE_SEGMENT_SIZE, (void **)&segment);
      ErrCheck(rc, error, ErrSys, "Failed to map segment");
      segments_.push_back(segment);
      load_data(segment);
    }
  }
  return rc;
}

void DmsFile::recover_space(char *page) {
  auto is_recover = false;
  auto left = page + sizeof(DmsPageHeader);
  auto right = page + DMS_PAGE_SIZE;
  auto page_header = (DmsPageHeader *)page;
  // recover space
  for (size_t i = 0; i < page_header->num_slots; ++i) {
    auto slot = *((SlotOff *)(left + sizeof(SlotOff) * i));
    if (DMS_SLOT_EMPTY != slot) {
      auto record_header = (DmsRecord *)(page + slot);
      auto record_size = record_header->size;
      right -= record_size;
      if (is_recover) {
        memmove(right, page + slot, record_size);
        *((SlotOff *)(left + sizeof(SlotOff) * i)) = (SlotOff)(right - page);
      }
    } else {
      is_recover = false;
    }
  }
  page_header->free_space = right - page;
}
