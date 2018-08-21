#include <bson.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  bson_t *b;
  bson_error_t error;

  b = bson_new_from_json("{\"a\":1}", -1, &error);

  if (!b) {
    printf("Error: %s\n", error.message);
  } else {
    bson_destroy(b);
  }

  return 0;
}
