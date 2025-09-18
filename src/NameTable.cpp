#include "NameTable.h"
#include "Helpers.h"
#include <vector>
#include <cstdint>
#include <iostream>
#include <fstream>

NameRecord::NameRecord(
  uint16_t platformID,
  uint16_t platformSpecificID,
  uint16_t languageID,
  uint16_t nameID,
  uint16_t length,
  uint16_t offset
) : platformID(platformID),
    platformSpecificID(platformSpecificID),
    languageID(languageID),
    nameID(nameID),
    length(length),
    offset(offset) {}

void NameRecord::printRecord() {
  cout << "----------------------------------------" << endl;
  cout << "Platform ID: " << platformID << endl;
  cout << "Platform Specific ID: " << platformSpecificID << endl;
  cout << "Language ID: " << languageID << endl;
  cout << "Name ID: " << nameID << endl;
  cout << "Name Length: " << length << endl;
  cout << "Name Offset: " << offset << endl;
}

NameTable::NameTable(
  uint16_t format,
  uint16_t count,
  uint16_t stringOffset,
  vector<NameRecord> nameRecords
): format(format),
   count(count),
   stringOffset(stringOffset),
   nameRecords(nameRecords) {}

NameTable NameTable::parseNameDirectory(const vector<char>& data, uint32_t nameTableOffset) {
  int pos = nameTableOffset;
  uint16_t format = read2Bytes(data, pos);
  uint16_t count = read2Bytes(data, pos);
  uint16_t stringOffset = read2Bytes(data, pos);

  vector<NameRecord> records;

  for (uint16_t i = 0; i < count; i++) {
    uint16_t platformID = read2Bytes(data, pos);
    uint16_t platformSpecificID = read2Bytes(data, pos);
    uint16_t languageID = read2Bytes(data, pos);
    uint16_t nameID = read2Bytes(data, pos);
    uint16_t length = read2Bytes(data, pos);
    uint16_t offset = read2Bytes(data, pos);
    records.push_back(NameRecord(platformID,
                                 platformSpecificID,
                                 languageID,
                                 nameID,
                                 length,
                                 offset));
    // records.back().printRecord();
  }

  for (NameRecord record : records) {
    pos = nameTableOffset + stringOffset + record.getOffset();
    string nameText;
    if (record.getPlatformID() == 1) {
        // Platform 1 (Macintosh) - single byte per character
        for (uint16_t i = 0; i < record.getLength(); i++) {
            uint8_t letter = readByte(data, pos);
            nameText += (static_cast<char>(letter));
        }
    } 
    else if (record.getPlatformID() == 0 || record.getPlatformID() == 3) {
        // Platform 0 (Unicode) or 3 (Microsoft) - UTF-16BE, 2 bytes per character
        for (uint16_t i = 0; i < record.getLength(); i += 2) {
            uint16_t letter = read2Bytes(data, pos);
            
            // Simple ASCII conversion - just take the low byte for basic characters
            // if (letter < 128) {
            //     cout << static_cast<char>(letter);
            // } else {
            //     cout << '?'; // Non-ASCII character placeholder
            // }
            nameText += (static_cast<char>(letter));
        }
    }
    record.setNameText(nameText);
    // cout << nameText << endl;
  }
  return NameTable(format, count, stringOffset, records);

}