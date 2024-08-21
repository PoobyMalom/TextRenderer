#include "TTFHeader.h"
#include "Helpers.h"
#include <stdexcept>
#include <iostream>

TTFHeader::TTFHeader(uint32_t scalarType, uint16_t numTables, uint16_t searchRange, uint16_t entrySelector, uint16_t rangeShift)
    : scalarType(scalarType), numTables(numTables), searchRange(searchRange), entrySelector(entrySelector), rangeShift(rangeShift) {}

TTFHeader TTFHeader::parse(const std::vector<char>& data) {
    int offset = 0;
    uint32_t scalarType = read4Bytes(data, offset); 
    uint16_t numTables = read2Bytes(data, offset); 
    uint16_t searchRange = read2Bytes(data, offset);
    uint16_t entrySelector = read2Bytes(data, offset);
    uint16_t rangeShift = read2Bytes(data, offset);

    return TTFHeader(scalarType, numTables, searchRange, entrySelector, rangeShift);
}

uint32_t TTFHeader::getScalarType() const { return scalarType; }
uint16_t TTFHeader::getNumTables() const { return numTables; }
uint16_t TTFHeader::getSearchRange() const { return searchRange; }
uint16_t TTFHeader::getEntrySelector() const { return entrySelector; }
uint16_t TTFHeader::getRangeShift() const { return rangeShift; }
