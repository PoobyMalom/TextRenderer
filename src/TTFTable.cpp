#include <iostream>
#include <unordered_map>
#include <cstdint>
#include <vector>
#include "TTFTable.h"
#include "Helpers.h"

using namespace std;

TTFTable::TTFTable(string tag, uint32_t checksum, uint32_t offset, uint32_t length) // NOLINT(bugprone-easily-swappable-parameters)
    : tag(std::move(tag)), checksum(checksum), offset(offset), length(length) {}

const string& TTFTable::getTag() const { return tag; }
uint32_t TTFTable::getChecksum() const { return checksum; }
uint32_t TTFTable::getOffset() const { return offset; }
uint32_t TTFTable::getLength() const { return length; }
void TTFTable::printTable() const { 
    cout << "Tag: " << tag << ", Checksum: " << checksum << ", Offset: " << offset << ", Length: " << length << '\n'; 
}

vector<TTFTable> TTFTable::parseTableDirectory(const vector<char>& data, uint16_t numTables) {
    vector<TTFTable> tables;

    int offset = 12;

    

    for (int i = 0; i < numTables; ++i) {
        string tag(data.begin() + offset, data.begin() + offset + 4);
        offset += 4;

        uint32_t checksum = read4Bytes(data, offset);
        uint32_t tableOffset = read4Bytes(data, offset);
        uint32_t length = read4Bytes(data, offset);

        uint32_t calculatedCheckSum;

        if (tag == "head") {
            calculatedCheckSum = calculateHeadChecksum(data, tableOffset, length);
        } else {
            calculatedCheckSum = CalcTableChecksum(data, tableOffset, length);
        }

        if (calculatedCheckSum != checksum) {
            fprintf(stderr, "Table: %s checksum does not match calculated checksum\n", tag.c_str());
        } else {
            printf("Table: %s checksum matchs calculated checksum\n", tag.c_str());
        }

        tables.emplace_back(TTFTable(tag, checksum, tableOffset, length));
    }
    return tables;
}

