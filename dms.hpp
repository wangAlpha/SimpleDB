#pragma once

#include "Record.hpp"
#include "core.hpp"
#include "mmapfile.hpp"

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
  uint16_t flag char data[0];
};

// dms header
#define DMS_HEADER_EYECATCHER "DMSH"
#define DMS_HEADER_EYECATCHER_LEN 4
#define DMS_HEADER_FLAG_NORMAL    0
#define DMS_HEADER_FLAG_DROPPED   1

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
#define DMS_PAGE_FLAG_NORMAL    0
#define DMS_PAGE_FLAG_UNALLOC   1
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
#define DMS_FILE_HEADER_SIZE  65536
#define DMS_PAGES_PER_SEGMENT (DMS_FILE_SEGMENT_SIZE / DMS_PAGE_SIZE)
#define DMS_MAX_SEGMENTS      (DMS_MAX_SEGMENTS / DMS_PAGES_PER_SEGMENT)

class DmsFile : public MmapFile {
 public:
  DmsHeader *header;
  std::vector<char *> body_;
  // free space to page id map
  std::mutex mutex_;
  std::mutex extend_mutex_;
  std::string file_name_;

  DmsFile() {}
  ~DmsFile();

  int initalize(const char *filename);
  int insert(nlohmann::json &record, nlohmann::json &out_record, Record &rid);
  int remove(Record &rid);
  int find(Record &rid, nlohmann::json &result);

  size_t getNumSegments() const { return body_.size(); }
  size_t getNumPages() const { return getNumSegments * DMS_PAGES_PER_SEGMENT; }
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
  int search_slot(char *page, Record &record_id, SlotOff &slot);
  //
  void recover_space(char *page);
  // update free space
  void update_free_space(DmsPageHeader *header, int change_size,
                         PageID page_id);
  // find a page id to insert, return invalid page id if there's no page cache
  // found for required size bytes
  PageID find_page(size_t required_size);
};
