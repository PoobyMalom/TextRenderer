#pragma once
#include "Helpers.h"
#include <cstdint>

using Tag = uint8_t[4];
using Offset16 = uint16_t; 

struct ScriptRecord {
  Tag scriptTable;
  Offset16 scriptOffset;
};

struct ScriptList {
  uint16_t scriptCount;
  vector<ScriptRecord> scriptRecords;
};

struct LangSysRecord {
  Tag langSysTag;
  Offset16 langSysOffset;
};

struct ScriptTable {
  Offset16 deafaultLangSysOffset;
  uint16_t langSysCount;
  vector<LangSysRecord> langSysRecords;
};