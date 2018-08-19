#pragma once

#include "core.h"
// 无效文件
const int INVALID_FD_HANDLE = -1;

using handleType = int;
using offsetType = off_t;

class FileHandleOp {
 public:
  explicit FileHandleOp() : file_handle_(INVALID_FD_HANDLE) {}
  ~FileHandleOp();
  int Open(const char *path);
  int Read(void *const buf, const size_t size);
  int Write(const void *buf, size_t len);

  handleType getCurrentOffset() const {
    return lseek(file_handle_, 0, SEEK_CUR);
  };
  void seekToOffset(offsetType offset);
  void seekToEnd(void) { lseek(file_handle_, 0, SEEK_END); }
  offsetType getSize(void);
  void setFileHandleOp(handleType handle);
  handleType handle(void) const { return file_handle_; }
  bool isValid() const { return file_handle_ != INVALID_FD_HANDLE; }

 private:
  handleType file_handle_;
  std::string file_name_;
  // DISALLOW_COPY_AND_ASSIGN(FileHandleOp);
  const int kFileFlag = O_RDWR | O_CREAT;
  const int kFileMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
};

FileHandleOp::~FileHandleOp() {
  close(file_handle_);
  file_handle_ = INVALID_FD_HANDLE;
}

int FileHandleOp::Open(const char *path) {
  do {
    // 读写模式
    file_handle_ = open(path, kFileFlag, kFileMode);
  } while (-1 == file_handle_ && errno == EINTR);
  return file_handle_;
}

// 读取文件
int FileHandleOp::Read(void *const buf, const size_t size) {
  int bytesRead = 0, length = 0;
  if (isValid()) {
    do {
      bytesRead = read(file_handle_, buf, size);
      if (bytesRead > 0) {
        length += bytesRead;
      }
    } while ((-1 == bytesRead) && (EINTR == errno));
    return bytesRead;
  } else {
    BOOST_LOG_TRIVIAL(error) << "Failed to read file";
  }
  return INVALID_FD_HANDLE;
}

// 写入文件
int FileHandleOp::Write(const void *buffer, size_t size) {
  int rc = 0;
  size_t current_size = 0;
  if (0 == size) {
    size = strlen((char *)buffer);
  }
  if (isValid()) {
    do {
      rc = write(file_handle_, &((char *)buffer)[current_size],
                 size - current_size);
      if (rc >= 0) {
        current_size += rc;
      }
    } while ((-1 == rc && EINTR == errno) ||
             ((-1 != rc && current_size < size)));
    if (-1 == rc) {
      rc = errno;
      BOOST_LOG_TRIVIAL(error) << "Failed to write file";
    }
  } else {
    BOOST_LOG_TRIVIAL(error) << "Failed to write file, file is invalid";
    return -1;
  }
  return current_size;
}

// 设置文件句柄
void FileHandleOp::setFileHandleOp(handleType filehandle) {
  if (isValid()) {
    close(file_handle_);
  }
  file_handle_ = filehandle;
}

void FileHandleOp::seekToOffset(offsetType offset) {
  if ((offsetType)-1 != offset) {
    lseek(file_handle_, offset, SEEK_SET);
  }
}

offsetType FileHandleOp::getSize(void) {
  struct stat fd_info;
  if (COMMON_ERROR == fstat(file_handle_, &fd_info)) {
    BOOST_LOG_TRIVIAL(error) << "Failed to get file size";
    return COMMON_ERROR;
  }
  return fd_info.st_size;
}
