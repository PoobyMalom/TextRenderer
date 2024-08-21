#include <iostream>
#include "TTFTable.h"
#include "Helpers.h"

using namespace std;

TTFTable::TTFTable(const string& tag, uint32_t checksum, uint32_t offset, uint32_t length)
    : tag(tag), checksum(checksum), offset(offset), length(length) {}

string TTFTable::getTag() const { return tag; }
uint32_t TTFTable::getChecksum() const { return checksum; }
uint32_t TTFTable::getOffset() const { return offset; }
uint32_t TTFTable::getLength() const { return length; }

vector<TTFTable*> TTFTable::parseTableDirectory(const vector<char>& data, uint16_t numTables) {
    vector<TTFTable*> tables;

    int offset = 12;
    for (int i = 0; i < numTables; ++i) {
        string tag(data.begin() + offset, data.begin() + offset + 4);
        offset += 4;

        uint32_t checksum = read4Bytes(data, offset);
        uint32_t Tableoffset = read4Bytes(data, offset);
        uint32_t length = read4Bytes(data, offset);

        tables.push_back(new TTFTable(tag, checksum, Tableoffset, length));
    }
    return tables;
}

uint32_t TTFTable::convertEndian32(uint32_t value) {
    return ((value >> 24) & 0x000000FF) |
           ((value >> 8)  & 0x0000FF00) |
           ((value << 8)  & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
}