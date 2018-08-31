#pragma once

#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <atomic>
#include <boost/algorithm/string.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <cctype>
#include <condition_variable>
#include <cstdio>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

constexpr int OK = 0;
constexpr int COMMON_OK = 0;
constexpr int COMMON_ERROR = -1;
constexpr int ErrConnected = -2;
constexpr int ErrCommand = -3;
constexpr int ErrSubCommand = -4;
constexpr int ErrSend = -5;
constexpr int Err_Recv_Length = -6;
constexpr int ErrInvaildArg = -7;
constexpr int ErrPackParse = -8;
constexpr int ErrSockNotConnect = -9;
constexpr int ErrRecvCode = -10;
constexpr int ErrRecvLen = -11;
constexpr int ErrIDExist = -12;
constexpr int ErrIDNotExist = -13;
constexpr int ErrSys = -14;

constexpr size_t SEND_BUFFER_SIZE = 4096;
constexpr size_t RECV_BUFFER_SIZE = 4096;
constexpr uint8_t MagicNumber = 0x20;
