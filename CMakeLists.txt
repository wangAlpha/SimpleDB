cmake_minimum_required(VERSION 3.0)
project(SimpleDB)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++17  -Wall -Wextra -v -g -lboost_system -lboost_program_options -lboost_log -lboost_log_setup -DBOOST_ALL_DYN_LINK -DBOOST_LOG_DYN_LIN -lboost_filesystem -pthread -lboost_thread")
add_executable(SimpleDB tcplistener.cc core.hpp filehandle.hpp threadpool.hpp logging.hpp icommand.hpp cmdline.hpp options.hpp dms.hpp mmapfile.hpp record.hpp bucket.hpp rtn.hpp pmd.hpp hash_code.hpp handle_message.hpp monitor.hpp)
add_executable(Client shell.cc shell.hpp command.hpp)
