#pragma once
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include "TTFTable.h"
#include <memory>

using TableVec = std::vector<TTFTable*>;
using TableMap = std::unordered_map<std::string, TTFTable*>;

class TTFHeader {

public:
    static TTFHeader parse(const std::vector<char>& data);
    void parseTables(const std::vector<char>& data);
    
    uint32_t getScalarType() const;
    uint16_t getNumTables() const;
    uint16_t getSearchRange() const;
    uint16_t getEntrySelector() const;
    uint16_t getRangeShift() const;
    TableMap getTables() const;
    
    friend std::ostream& operator<<(std::ostream& os, const TTFHeader& obj);

private:
    uint32_t scalarType;
    uint16_t numTables;
    uint16_t searchRange;
    uint16_t entrySelector;
    uint16_t rangeShift;

    TableMap tables;
    TableVec tablesList;

    TTFHeader(uint32_t scalarType, uint16_t numTables, uint16_t searchRange, uint16_t entrySelector, uint16_t rangeShift);
};