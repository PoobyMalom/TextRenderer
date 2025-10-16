#include "GposTable.h"
#include <iostream>
#include <fstream>

using Offset16 = uint16_t;
using Offset32 = uint32_t;

void GposTable::parseGposDirectory(const vector<char>& data, uint32_t offset) {
  int pos = offset;
  uint16_t majorVersion = read2Bytes(data, pos);
  uint16_t minorVersion = read2Bytes(data, pos);
  //cout << "GPOS Table Data, Major Verison: " << majorVersion << ", Minor Version: " << minorVersion << endl;
  // TODO Figure out how to do version 1 vs 0
  Offset16 scriptListOffset = read2Bytes(data, pos);
  Offset16 featureListOffset = read2Bytes(data, pos);
  Offset16 lookupListOffset = read2Bytes(data, pos);
  Offset32 featureVariationsOffset;
  if (minorVersion == 1) {
    featureVariationsOffset = read4Bytes(data, pos); 
  } else {
    featureVariationsOffset = NULL;
  }
  // cout << "Script List Offset: " << scriptListOffset << endl;
  // cout << "Feature List Offset: " << featureListOffset << endl;
  // cout << "Lookup List Offset: " << lookupListOffset << endl;
  // cout << "Feature Variations Offset: " << featureVariationsOffset << endl;

  int scriptListPos = offset + scriptListOffset;
  uint16_t scriptCount = read2Bytes(data, scriptListPos);
  cout << "Script Count: " << scriptCount << endl;
  for (uint16_t i = 0; i < scriptCount; i++) {
    uint32_t scriptTag = read4Bytes(data, scriptListPos);
    Offset16 scriptOffset = read2Bytes(data, scriptListPos);

    uint8_t tag[4];

    tag[3] = (scriptTag & 0x000000ff);
    tag[2] = (scriptTag & 0x0000ff00) >> 8;
    tag[1] = (scriptTag & 0x00ff0000) >> 16;
    tag[0] = (scriptTag & 0xff000000) >> 24;

   // cout << "Script Record " << i << ", Tag: " << tag << ", Offset: " << scriptOffset << endl;
  }
}