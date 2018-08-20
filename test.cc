#include <iostream>

#include <bsonobjbuilder.h>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
int main(int, char**) {
  auto builder = bsoncxx::builder::stream::document{};
  bsoncxx::document::value doc_value = builder << "name"
                                               << "MongoDB"
                                               << "type"
                                               << "database"
                                               << "count" << 1 << "versions";
  BSONObjBuilder b;
  b << "name"
    << "Joe"
    << "age" << 33;
  BSONObj p = b.obj();
  return 0;
}