#pragma once
#include <string>
#include <vector>
#include <cstdint>

class TTFTable {
public:
    TTFTable(const std::string& tag, uint32_t checksum, uint32_t offset, uint32_t length);
    
    std::string getTag() const;
    uint32_t getChecksum() const;
    uint32_t getOffset() const;
    uint32_t getLength() const;

    static std::vector<TTFTable*> parseTableDirectory(const std::vector<char>& data, uint16_t numTables);

private:
    std::string tag;
    uint32_t checksum;
    uint32_t offset;
    uint32_t length;

    static uint32_t convertEndian32(uint32_t value);
};