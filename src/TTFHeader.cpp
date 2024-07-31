#include "TTFHeader.h"
#include <stdexcept>

TTFHeader::TTFHeader(uint32_t scalarType, uint16_t numTables, uint16_t searchRange, uint16_t entrySelector, uint16_t rangeShift)
    : scalarType(scalarType), numTables(numTables), searchRange(searchRange), entrySelector(entrySelector), rangeShift(rangeShift) {}

TTFHeader TTFHeader::parse(const std::vector<char>& data) {
    if (data.size() < 12) {
        throw std::runtime_error("Insufficient data to parse TTF header");
    }

    uint32_t scalarType = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    uint16_t numTables = (data[4] << 8) | data[5];
    uint16_t searchRange = (data[6] << 8) | data[7];
    uint16_t entrySelector = (data[8] << 8) | data[9];
    uint16_t rangeShift = (data[10] << 8) | data[11];

    return TTFHeader(scalarType, numTables, searchRange, entrySelector, rangeShift);
}

uint32_t TTFHeader::getScalarType() const {
    return scalarType;
}

uint16_t TTFHeader::getNumTables() const {
    return numTables;
}

uint16_t TTFHeader::getSearchRange() const {
    return searchRange;
}

uint16_t TTFHeader::getEntrySelector() const {
    return entrySelector;
}

uint16_t TTFHeader::getRangeShift() const {
    return rangeShift;
}
