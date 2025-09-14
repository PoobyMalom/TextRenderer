#include "LocaTable.h"
#include "Helpers.h"

#include <cstring>
#include <cassert>
#include <iostream>
// <arpa/inet.h> not needed here; Helpers handles endianness

LocaTable LocaTable::parse(bool is32bit,
                           const std::vector<char>& data,
                           size_t locaOffset,
                           size_t numGlyphs) {
    LocaTable locaTable;
    locaTable.is32bitFormat = is32bit;

    // NOTE: Keep pos as int only if your read helpers take int&.
    int pos = static_cast<int>(locaOffset);

    // Normalize to BYTE OFFSETS regardless of format.
    locaTable.offsets32.resize(numGlyphs + 1);

    if (is32bit) {
        // indexToLocFormat == 1 → uint32 byte offsets
        for (size_t i = 0; i <= numGlyphs; ++i) {
            uint32_t offBytes = read4Bytes(data, pos);   // big-endian → host
            locaTable.offsets32[i] = offBytes;
        }
    } else {
        // indexToLocFormat == 0 → uint16 half-offsets (offset/2). Convert to bytes.
        for (size_t i = 0; i <= numGlyphs; ++i) {
            uint16_t half = read2Bytes(data, pos);       // big-endian → host
            locaTable.offsets32[i] = static_cast<uint32_t>(half) * 2U;
        }
    }

#ifndef NDEBUG
    // Sanity checks (debug only)
    assert(locaTable.offsets32.size() == numGlyphs + 1);
    for (size_t i = 1; i < locaTable.offsets32.size(); ++i) {
        assert(locaTable.offsets32[i] >= locaTable.offsets32[i - 1]); // non-decreasing
    }
#endif

    return locaTable;
}

const std::vector<uint32_t>& LocaTable::getOffsets32() const {
    // Now always the normalized BYTE offsets, regardless of original format.
    return offsets32;
}

const std::vector<uint16_t>& LocaTable::getOffsets16() const {
    // Prevent accidental use of raw half-offsets.
    throw std::runtime_error("LocaTable no longer exposes raw 16-bit half-offsets. Use getOffsets32().");
}

void LocaTable::printLocaTable() {
    std::cout << "indexToLocFormat==1 (long offsets)? " << (is32bitFormat ? "true" : "false") << std::endl;
    for (std::size_t i = 0; i < offsets32.size(); ++i) {
        std::cout << "Loca byte offset [" << i << "] = " << offsets32[i] << '\n';
    }
}
