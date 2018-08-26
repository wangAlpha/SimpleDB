#pragma once

#include "core.hpp"
#include "filehandle.hpp"
// Mapping files to memory
class MmapFile {
 public:
  explicit MmapFile() : opened_(false) {}
  ~MmapFile() { Close(); }

  int Open(const char *path);
  int Close();
  int Map(offsetType offset, size_t length, void **address);
  bool is_open(void) const { return opened_; }

 private:
  struct MmapSegment {
    void *ptr;
    size_t length;
    size_t offset;
    MmapSegment(void *p = nullptr, size_t const len = 0, size_t const off = 0)
        : ptr(p), length(len), offset(off) {}
  };

 protected:
  bool opened_;
  FileHandleOp file_handle_;
  std::string file_name_;
  std::vector<MmapSegment> segments_;
  std::mutex mutex_;
};

int MmapFile::Open(char const *path) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (file_handle_.Open(path) == INVALID_FD_HANDLE) {
    BOOST_LOG_TRIVIAL(error) << "Failed to open mmappfile " << path;
    return COMMON_ERROR;
  }
  file_name_ = path;
  opened_ = true;
  return OK;
}

int MmapFile::Map(offsetType offset, const size_t length, void **address) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto file_size = file_handle_.getSize();
  if (file_size < (offsetType)length + offset) {
    // 需要确定是否减1
    // auto current_offset = file_handle_.getCurrentOffset();
    printf("current offset %d\n", file_handle_.getCurrentOffset());
    auto len = length + offset - file_handle_.getSize();
    auto tmp = malloc(len);
    file_handle_.Write(tmp, len);
    free(tmp);
    printf("current offset %d\n", file_handle_.getCurrentOffset());
  }

  auto result = mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED,
                     file_handle_.handle(), offset);
  if (result == MAP_FAILED) {
    BOOST_LOG_TRIVIAL(error) << "Failed to mmap file " << file_name_;
    return COMMON_ERROR;
  }
  *address = result;
  return OK;
}

int MmapFile::Close(void) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (opened_) {
    std::for_each(segments_.begin(), segments_.end(), [](auto &segment) {
      if (segment.ptr != nullptr) {
        munmap(segment.ptr, segment.length);
      }
    });
  }
  segments_.clear();
  file_name_.clear();
  opened_ = false;
  return OK;
}
