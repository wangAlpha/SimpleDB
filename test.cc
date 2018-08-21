#include <iostream>
#include <nlohmann/json.hpp>
#include <vector>
#include "bson.h"
#include "bson/src/util/json.h"
// #include "json.h"
using json = nlohmann::json;
int main(int argc, char const* argv[]) {
  json j;
  // bool parse_error(std::size_t position, const std::string& last_token,
  //                  const detail::exception& ex);

  // .parse(true, j_nothrow));
  // // CHECK(j_nothrow == j);
  // std::string str;
  // std::cin >> str;
  // try {
  //   auto j = json::parse(str);
  //   auto v = json::to_msgpack(j);
  //   std::cout << j << j.size() << std::endl;
  //   std::cout << v.size() << std::endl;

  // } catch (const json::exception& e) {
  //   std::cerr << e.what() << '\n';
  //   std::cerr << "Json parse failed!" << std::endl;
  // }
  // // std::string str;
  // // std::cin >> str;
  // // try {
  // //   auto obj = bson::BSONObj("{\"_id\": 1, \"hello\":\"world\"}");
  // // } catch (bson::exception const& e) {
  // //   std::cerr << e.what() << std::endl;
  // }
  try {
    std::string str;
    bson::BSONObj data = bson::fromjson(str);
  } catch (std::exception const& e) {
    std::cout << e.what() << std::endl;
  }
  return 0;
}
