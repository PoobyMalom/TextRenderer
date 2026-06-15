#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

class TTFTable {
    public:
    TTFTable(std::string tag, uint32_t checksum, uint32_t offset, uint32_t length);
    
    const std::string& getTag() const;
    uint32_t getChecksum() const;
    uint32_t getOffset() const;
    uint32_t getLength() const;
    void printTable() const;

    static std::vector<TTFTable> parseTableDirectory(const std::vector<char>& data, uint16_t numTables);

private:
    std::string tag;
    uint32_t checksum;
    uint32_t offset;
    uint32_t length;


};
