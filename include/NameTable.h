#pragma once
#include <vector>
#include <cstdint>
#include "Helpers.h"

using namespace std;

class NameRecord {
  public:
    NameRecord(
      uint16_t platformID,
      uint16_t platformSpecificID,
      uint16_t languageID,
      uint16_t nameID,
      uint16_t length,
      uint16_t offset
    );

    uint16_t getPlatformID() const { return platformID; };
    uint16_t getPlatformSpecificID() const { return platformSpecificID; };
    uint16_t getLanguageID() const { return languageID; };
    uint16_t getNameID() const { return nameID; };
    uint16_t getLength() const { return length; };
    uint16_t getOffset() const { return offset; };
    string getNameText() const { return nameText; };
    void setNameText(string text) { nameText = text; };

    void printRecord();

  private:
    uint16_t platformID;
    uint16_t platformSpecificID;
    uint16_t languageID;
    uint16_t nameID;
    uint16_t length;
    uint16_t offset;
    string nameText;
};

class NameTable {
  public:
    NameTable(
      uint16_t format,
      uint16_t count,
      uint16_t stringOffset,
      vector<NameRecord> nameRecords
    );

    uint16_t getFormat() const { return format; };
    uint16_t getCount() const { return count; };
    uint16_t getStringOffset() const { return stringOffset; };
    vector<NameRecord> getNameRecords() const { return nameRecords; };

    static NameTable parseNameDirectory(const vector<char>& data, uint32_t nameTableOffset);
    void printNameTable();

  private:
    uint16_t format;
    uint16_t count;
    uint16_t stringOffset;
    vector<NameRecord> nameRecords;
    // include name later when you understand wtf is going on
};