#include <nlohmann/json.hpp>
#include <cstdio>

using json = nlohmann::json;

int main() {
  auto j = R"({"_id": "key", "hello": "world"})"_json;
  auto pack = json::to_msgpack(j);
  for (auto const &p : pack) {
    printf("%x ", p);
  }
  return 0;
}
