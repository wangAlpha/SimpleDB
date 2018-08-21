cmake_minimum_required(VERSION 3.12)
project(SimpleDB)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++14  -Wall -Wextra -v -pthread -lboost_system -lboost_program_options -lboost_log -lboost_log_setup -DBOOST_ALL_DYN_LINK -lboost_filesystem")
add_executable(SimpleDB tcplistener.cc core.h filehandle.cc filehandle.h ThreadPool.h logging.cc logging.h ICommand.h ICommand.cc)
add_executable(Client shell.cc shell.h command.cc command.h)