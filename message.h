#pragma once

struct MessageHeader {
  int len;
  int typeCode;
  int data[0];
};
