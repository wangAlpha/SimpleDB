cd build
cmake ..
make
add_executable(SimpleDB tcplistener.cc core.h filehandle.cc filehandle.h ThreadPool.h logging.cc logging.h ${BSON_SOURCES})
add_executable(Client shell.cc shell.h command.cc command.h)