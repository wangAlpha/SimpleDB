#pragma once

#include "core.hpp"
#include "logging.hpp"
#include "mmapfile.hpp"
#include "record.hpp"

#define DMS_EXTEND_SIZE 65536
// 4MB for page size
#define DMS_PAGE_SIZE 1024 * 4096
#define DMS_MAX_RECORD \
  (DMS_PAGE_SIZE - sizeof(Header) - sizeof(Record) - sizeof(SlotOff))
#define DMS_MAX_PAGES 1024 * 256
using SlotOff = unsigned int;
#define DMS_INVALID_SLOTID 0xFFFFFFFF
#define DMS_INVALID_PAGEID 0xFFFFFFFF

#define DMS_KEY_FIELDNAME "_id"

// each record has the following header, include 4 bytes size and 4 bytes flag
#define DMS_RECORD_FLAG_NORMAL 0
#define DMS_RECORD_FLAG_DROPPED 1

struct DmsRecord {
  uint16_t size;
  uint16_t flag;
  char data[0];
};

// dms header
#define DMS_HEADER_EYECATCHER "DMSH"
#define DMS_HEADER_EYECATCHER_LEN 4
#define DMS_HEADER_FLAG_NORMAL 0
#define DMS_HEADER_FLAG_DROPPED 1

#define DMS_HEADER_VERSION 0
#define DMS_HEADER_VERSION_CURRENT DMS_HEADER_VERSION

struct DmsHeader {
  char eye_catcher[DMS_HEADER_EYECATCHER_LEN];
  uint16_t size;
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
#define DMS_PAGE_EYECATCHER "PAGH"
#define DMS_PAGE_EYECATCHER_LEN 4
#define DMS_PAGE_FLAG_NORMAL 0
#define DMS_PAGE_FLAG_UNALLOC 1
#define DMS_SLOT_EMPTY 0xFFFFFFFF

struct DmsPageHeader {
  char eye_catcher[DMS_PAGE_EYECATCHER_LEN];
  uint16_t size;
  uint16_t flag;
  uint16_t num_slots;
  uint16_t slot_offset;
  uint16_t free_space;
  uint16_t free_offset;
  char data[0];
};

#define DMS_FILE_SEGMENT_SIZE 65536 * 2048
#define DMS_FILE_HEADER_SIZE 65536
#define DMS_PAGES_PER_SEGMENT (DMS_FILE_SEGMENT_SIZE / DMS_PAGE_SIZE)
#define DMS_MAX_SEGMENTS (DMS_MAX_SEGMENTS / DMS_PAGES_PER_SEGMENT)

class DmsFile : public MmapFile {
 public:
  DmsHeader *header_;
  std::vector<char *> body_;
  // free space to page id map
  std::multimap<uint16_t, PageID> free_space_map_;
  std::mutex mutex_;
  std::mutex extend_mutex_;
  std::string file_name_;

  explicit DmsFile() : header_(nullptr) {}
  ~DmsFile() { Close(); }

  int initalize(const char *filename);
  int insert(nlohmann::json &record, nlohmann::json &out_record,
             DmsRecordID &rid);
  int remove(DmsRecordID &rid);
  int find(DmsRecordID &rid, nlohmann::json &result);

  size_t getNumSegments() const { return body_.size(); }
  size_t getNumPages() const {
    return getNumSegments() * DMS_PAGES_PER_SEGMENT;
  }
  char *pageToOffset(PageID page_id) {
    if (page_id >= getNumPages()) {
      return nullptr;
    } else {
      return body_[page_id / DMS_PAGES_PER_SEGMENT] +
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
  // create a new segment for the current file
  int extend_segment();
  // init from empty file, creating header only
  int init_new();
  // load data from beginning
  int load_data();
  // search slot
  int search_slot(char *page, DmsRecordID &record_id, SlotOff &slot);
  //
  void recover_space(char *page);
  // update free space
  void update_free_space(DmsPageHeader *header, int change_size,
                         PageID page_id);
  // find a page id to insert, return invalid page id if there's no page cache
  // found for required size bytes
  PageID find_page(size_t required_size);
};

int DmsFile::insert(nlohmann::json &record, nlohmann::json &out_record,
                    DmsRecordID &rid) {
  auto rc = OK;
  PageID page_id = 0;
  char *page = nullptr;
  int record_size = 0;
  SlotOff offset_tmp = 0;
  DmsRecord record_header;

  try {
    auto record_buffer = nlohmann::json::to_msgpack(record);
    record_size = record_buffer.size();
  } catch (nlohmann::json::exception const &e) {
    DB_LOG(error, e.what());
    rc = ErrPackParse;
    // goto done;
    return rc;
  }

  // make sure _id exists
  if (record.find(DMS_KEY_FIELDNAME) == record.end()) {
    rc = ErrInvaildArg;
    DB_LOG(error, "record must be with id");
    // goto done;
    return rc;
  }
retry:
  // lock the database
  mutex_.lock();
  // and then we should get the required record size
  page_id = find_page(record_size + sizeof(DmsRecord));
  // if there's not enough space in any existing page, let's release db lock
  // and try to allocate a new segment by calling extend_segment
  if (DMS_INVALID_PAGEID == page_id) {
    mutex_.unlock();
    // release db lock
    if (extend_mutex_.try_lock()) {
      rc = extend_segment();
      if (rc) {
        DB_LOG(error, "Failed to extend segment");
        extend_mutex_.unlock();
        // goto done;
        return rc;
      }
    } else {
      // if we cannot get the extendmutex, that means someone else is trying to
      // extend so let's wait until getting the mutex, and release it and try
      // again
      extend_mutex_.lock();
    }
    extend_mutex_.unlock();
    goto retry;
  }
  // find the in-memeory offset for the page
  page = pageToOffset(page_id);
  // if something wrong, let's return error
  if (!page) {
    DB_LOG(error, "Failed to find the page");
    rc = ErrSys;
    mutex_.unlock();
    return rc;
    // goto error_release_mutex;
  }
  // set page header
  auto page_header = (DmsPageHeader *)(page);
  if (memcmp(reinterpret_cast<void *>(page_header), DMS_PAGE_EYECATCHER,
             DMS_PAGE_EYECATCHER_LEN) != 0) {
    DB_LOG(error, "Invalid page header");
    rc = ErrSys;
    // goto error_release_mutex;
    mutex_.unlock();
    return rc;
  }
  // slots offset is the lase bytes of slots;
  // free offset is the first byte of data
  // so freeOffset - slotOffset is the actual free space excluding holes
  if ((page_header->free_space <
       record_size + sizeof(DmsHeader) + sizeof(SlotID)) ||
      (page_header->free_offset - page_header->slot_offset <
       record_size + sizeof(DmsRecord) + sizeof(SlotID))) {
    DB_LOG(error, "Something big wrong!!");
    rc = ErrSys;
    mutex_;
    return rc;
    // goto error_release_mutex;
  }
  offset_tmp = page_header->free_offset - record_size - sizeof(DmsRecord);
  record_header.size = record_size + sizeof(DmsRecord);
  record_header.flag = DMS_RECORD_FLAG_NORMAL;
  // copy the slot
  *(SlotOff *)(page + sizeof(DmsPageHeader) +
               page_header->num_slots * sizeof(SlotOff)) = offset_tmp;
  // copy the header
  auto pack_buffer = nlohmann::json::to_msgpack(record);
  memcpy(page + offset_tmp + sizeof(DmsRecord), pack_buffer.data(),
         record_size);
  out_record =
      nlohmann::json::from_msgpack(page + offset_tmp + sizeof(DmsRecord));
  rid.page_id = page_id;
  rid.slot_id = page_header->num_slots;
  // modiy metadata in page
  page_header->num_slots++;
  page_header->slot_offset += sizeof(SlotID);
  page_header->free_offset = offset_tmp;
  // modify database metadata
  update_free_space(page_header,
                    -(record_size + sizeof(SlotID) + sizeof(DmsRecord)),
                    page_id);
  // release lock for database
  mutex_.unlock();
done:
  return rc;
error_release_mutex:
  mutex_.unlock();
  goto done;
}

int DmsFile::remove(DmsRecordID &rid) {
  auto rc = OK;

  std::lock_guard<std::mutex> lock(mutex_);
  // find the page in memory
  auto page = pageToOffset(rid.page_id);
  if (!page) {
    DB_LOG(error, "Failed to find the page");
    rc = ErrSys;
    goto done;
  }
  // search the given slot
  SlotOff slot = 0;
  rc = search_slot(page, rid, slot);
  if (rc) {
    DB_LOG(error, "Failed to search slot");
    rc = ErrSys;
    goto done;
  }
  if (DMS_SLOT_EMPTY == slot) {
    DB_LOG(error, "The record is dropped");
    rc = ErrSys;
    goto done;
  }
  // set page header
  auto page_header = reinterpret_cast<DmsPageHeader *>(page);
  // set slot to empty
  *(SlotID *)(page + sizeof(DmsPageHeader) + rid.slot_id * sizeof(SlotID)) =
      DMS_SLOT_EMPTY;
  // set record header
  auto record_header = reinterpret_cast<DmsRecord *>(page + slot);
  record_header->flag = DMS_RECORD_FLAG_DROPPED;
  // update database metadata
  update_free_space(page_header, record_header->size, rid.page_id);
done:
  return rc;
}

int DmsFile::find(DmsRecordID &rid, nlhomann::json &result) {
  auto rc = OK;
  // use read lock the database
  std::lock<std::mutex> lock(mutex_);
  // goto the page and verify the slot is valid
  auto page = pageToOffset(rid.slot_id);
  if (!page) {
    DB_LOG(error, "Failed to find the page");
    rc = ErrSys;
    goto done;
  }
  // if slot is empty, something big wrong
  rc = search_slot(page, rid, slot);
  if (DMS_SLOT_EMPTY == slot) {
    DB_LOG(error, "Failed to find the page");
    rc = ErrSys;
    goto done;
  }
  // get the record header
  auto record_header = reinterpret_cast<DmsRecord *>(page + slot);
  // if record_header->flag is dropped, this record is dropped already
  if (DMS_RECORD_FLAG_DROPPED == record_header->flag) {
    DB_LOG(error, "The data is dropped");
    rc = ErrSys;
    goto done;
  }
  auto result = nlohmann::json::from_msgpack(page + slot + sizeof(DmsRecord));
done:
  return rc;
}

void DmsFile::updateFreeSpace(DmsPageHeader *header, int chang_size,
                              PageID page_id) {
  auto free_space = header->free_space;
  auto ret = free_space_map_.equal_range(free_space);

  for (auto &it : ret) {
    if (it->second == page_id) {
      free_space_map_.erase(it);
      break;
    }
  }
  // increase page free space
  free_space += chang_size;
  header->free_space = free_space;
  free_space_map_.insert(std::pair<uint16_t PageID>(freeSpace, page_id));
}

int DmsFile::initialize(const char *file_name) {
  offsetType offset = 0;
  int rc = OK;
  file_name_ = file_name;
  rc = Open(file_name);
  DB_CHECK(rc, error, "Failed to open file");
getfilesize:
  // get file size
  rc = file_handle_.getfilesize(&offset);
  // if file size is 0, that means it's newly created file and we should
  // initailize it
  if (!offset) {
    rc = init_new();
    DB_CHECK(rc, error, "Failed to initialize file");
    goto getfilesize;
  }
  // load data
  rc = load_data();
  DB_CHECK(rc, error, "Failed to load data");
done:
  return rc;
}

// caller must hold extend latch
int DmsFile::extend_segments() {
  // extend a new segment
  offsetType offset = 0;
  auto rc = file_handle_.getSize();
  DB_CHECK(rc, error "Failed to get file size");

  // first let's get the size of file before extend
  rc = extend_file(DMS_FILE_SEGMENT_SIZE);
  DB_CHECK(rc, error, "Failed to extend segment");

  // map from original end to new end
  char *data = nullptr;
  rc = Map(offset, DMS_FILE_SEGMENT_SIZE, reinterpret_cast<void **> & data);

  // create page header structure and we are going to copy to each page
  DmsPageHeader page_header;
  strcpy(page_header.eye_catcher, DMS_PAGE_EYECATCHER);
  page_header.size = DMS_PAGE_SIZE;
  page_header.flag = DMS_PAGE_FLAG_NORMAL;
  page_header.num_slots = 0;
  page_header.slot_offset = sizeof(DmsPageHeader);
  page_header.free_space = DMS_PAGE_SIZE - sizeof(DmsPageHeader);
  page_header.free_offset = DMS_PAGE_SIZE;
  // copy header to each page
  for (size_t i = 0; i < DMS_FILE_SEGMENT_SIZE; i += DMS_PAGE_SIZE) {
    memcpy(data + i, (char *)&page_header, sizeof(DmsPageHeader));
  }

  // free space handling
  auto free_map_size = free_space_map_.size();
  // insert into free space map
  for (size_t i = 0; i < DMS_PAGES_PER_SEGMENT; ++i) {
    free_space_map_.insert(
        std::pair<uint16_t, PageID>(page_header.free_space, i + free_map_size));
  }
  // push the segment into body list
  body_.push_back(data);
  header_->size += DMS_PAGES_PER_SEGMENT;
  return rc;
}

int DmsFile::init_new() {
  // initialize a newly created file, let's append DMS_FILE_HEADER_SIZE bytes
  // and then initialize the header
  auto rc = extend_file(DMS_FILE_HEADER_SIZE);
  DB_CHECK(rc, error, "Failed to extend file");

  rc = Map(0, DMS_FILE_HEADER_SIZE, (void **)&header_);
  DB_CHECK(rc, error, "Failed to map");

  strcpy(header_->eye_catcher, DMS_HEADER_EYECATCHER);
  header_->size = 0;
  header_->flag = DMS_HEADER_FLAG_NORMAL;
  header_->version = DMS_HEADER_VERSION_CURRENT;
  return rc;
}

PageID DmsFile::find_page(size_t require_size) {
  auto iter = free_space_map.update_bound(require_size);
  return (iter != free_space_map_.end()) ? iter->second : DMS_INVALID_PAGEID;
}

int DmsFile::extend_file(int size) {
  auto rc = OK;
  char temp[DMS_EXTEND_SIZE] = {0};
  memset(temp, 0, sizeof(temp));
  if (size % DMS_EXTEND_SIZE != 0) {
    rc = ErrSys;
    DB_LOG(error, "Invalid extend size");
    goto done;
  }
  // write file
  for (size_t i = 0; i < size; i += DMS_EXTEND_SIZE) {
    file_handle_.seekToEnd();
    rc = file_handle_.Write(temp, DMS_EXTEND_SIZE);
    DB_CHECK(rc, error, "Failed to write to file");
  }
done:
  return rc;
}

int DmsFile::search_slot(char *page, DMsDmsRecordID &rid, SlotOff &slot) {
  auto rc = OK;
  if (!page) {
    DB_LOG(error, "page is nullptr");
    rc = ErrSys;
    goto done;
  }
  // let's first verify the rid is valid
  if (0 > rid.page_id || 0 > rid.slot_id) {
    DB_LOG(error, "Invaild RID");
    rc = ErrSys;
    goto done;
  }
  auto page_header = (DmsPageHeader *)page;
  if (rid.slot_id > page_header->num_slots) {
    DB_LOG(error, "Slot is out of range, provided");
    rc = ErrSys;
    goto done;
  }
  slot =
      *(SlotOff *)(page + sizeof(DmsPageHeader) + rid.slotID * sizeof(SlotOff));
done:
  return rc;
}
int DmsFile::load_data() {
  auto rc = OK;
  if (!header_) {
    rc = Map(0, DMS_FILE_HEADER_SIZE, (void **)&header_);
    DB_CHECK(rc, error, "Failed to load data, partial segment detected");
    goto done;
  }
  auto num_page = header_->size;
  if (num_page % DMS_PAGES_PER_SEGMENT) {
    DB_LOG(error, "Failed to load data, partial segment detected");
    rc = ErrSys;
    goto done;
  }
  auto num_segments = num_page / DMS_PAGES_PER_SEGMENT;
  // get the segments number
  char *data = nullptr;
  if (num_segments > 0) {
    for (size_t i = 0; i < num_segments; ++i) {
      // map each segment into memory
      rc = Map(DMS_FILE_HEADER_SIZE + DMS_FILE_SEGMENT_SIZE * i,
               DMS_FILE_SEGMENT_SIZE, (void **)&data);
      DB_CHECK(rc, error, "Failed to map segment");
      body_.push_back(data);
      // initialize each page into free_space_map
      for (size_t k = 0; k < DMS_PAGES_PER_SEGMENT; ++k) {
        auto page_header = (DmsPageHeader *)(data + k * DMS_PAGE_SIZE);
        free_space_map_.insert(
            std::pair<unsigned int, PageID>(page_header->free_space, k));
        auto slot_id = (SlotID)page_header->num_slots;
        record_id.page_id = (PageID)k;
        for (size_t s = 0; s < slot_id; ++s) {
        }
      }
    }
  }
done:
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
  page_header->freespace = right - page;
}