#include "simpledb.hpp"

int main() {
  SimpleDB connect("localhost:1090");
  connect.insert("{\"_id\":\"key\"}");
  auto value = connect.query("{\"_id\"}");
  return 0;
}