#include <iostream>
#include "TTFTable.h"

TTFTable::TTFTable(const std::string& tag, uint32_t checksum, uint32_t offset, uint32_t length)
    : tag(tag), checksum(checksum), offset(offset), length(length) {}

std::string TTFTable::getTag() const {
    return tag;
}

uint32_t TTFTable::getChecksum() const {
    return checksum;
}

uint32_t TTFTable::getOffset() const {
    return offset;
}

uint32_t TTFTable::getLength() const {
    return length;
}

std::vector<TTFTable*> TTFTable::parseTableDirectory(const std::vector<char>& data, uint16_t numTables) {
    std::vector<TTFTable*> tables;

    size_t offset = 12;
    uint16_t i = 0;
    for (i; i < numTables; ++i) {
        std::string tag(data.begin() + offset, data.begin() + offset + 4);

        uint32_t checksum;
        uint32_t Tableoffset;
        uint32_t length;

        memcpy(&checksum, data.data() + offset + 4, 4);
        memcpy(&Tableoffset, data.data() + offset + 8, 4);
        memcpy(&length, data.data() + offset + 12, 4);

        checksum = convertEndian32(checksum);
        Tableoffset = convertEndian32(Tableoffset);
        length = convertEndian32(length);

        tables.push_back(new TTFTable(tag, checksum, Tableoffset, length));
        offset += 16;
    }
    return tables;
}

uint32_t TTFTable::convertEndian32(uint32_t value) {
    return ((value >> 24) & 0x000000FF) |
           ((value >> 8)  & 0x0000FF00) |
           ((value << 8)  & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
}