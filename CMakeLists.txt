cmake_minimum_required(VERSION 3.12)
project(SimpleDB)

set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++14  -Wall -Wextra -v -pthread -lboost_system -lboost_program_options -lboost_log -lboost_log_setup -DBOOST_ALL_DYN_LINK -lboost_filesystem -I/usr/local/include/mongocxx/v_noabi -I/usr/local/include/bsoncxx/v_noabi -L/usr/local/lib -lmongocxx -lbsoncxx")
add_executable(SimpleDB tcplistener.cc core.h filehandle.cc filehandle.h ThreadPool.h logging.cc logging.h)
add_executable(Client shell.cc shell.h command.cc command.h)
add_executable(test test.cc)