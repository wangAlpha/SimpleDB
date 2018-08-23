cmake_minimum_required(VERSION 3.12)
project(SimpleDB)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++14  -Wall -Wextra -v -lboost_system -lboost_program_options -lboost_log -lboost_log_setup -DBOOST_ALL_DYN_LINK -lboost_filesystem")
add_executable(SimpleDB tcplistener.cc core.h filehandle.h threadpool.h logging.h icommand.h cmdline.h options.h)
add_executable(Client shell.cc shell.h command.h)