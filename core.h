#pragma once

#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <condition_variable>
#include <cstdio>
#include <functional>
#include <future>
#include <iostream>
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
