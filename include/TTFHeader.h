#pragma once
#include <cstdint>
#include <vector>

class TTFHeader {
public:
    static TTFHeader parse(const std::vector<char>& data);
    
    uint32_t getScalarType() const;
    uint16_t getNumTables() const;
    uint16_t getSearchRange() const;
    uint16_t getEntrySelector() const;
    uint16_t getRangeShift() const;

private:
    uint32_t scalarType;
    uint16_t numTables;
    uint16_t searchRange;
    uint16_t entrySelector;
    uint16_t rangeShift;

    TTFHeader(uint32_t scalarType, uint16_t numTables, uint16_t searchRange, uint16_t entrySelector, uint16_t rangeShift);
};