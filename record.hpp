#pragma once

using PageID = unsigned int;
using SlotID = unsigned int;

struct RecordID {
  PageID page_id;
  SlotID slot_id;
};
