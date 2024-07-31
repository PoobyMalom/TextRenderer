#include "LocaTable.h"
#include <stdexcept>
#include <cstring>

LocaTable LocaTable::parse(bool is32bit, const std::vector<char>& data, size_t locaOffset, size_t numGlyphs) {
    LocaTable locaTable;
    locaTable.is32bitFormat = is32bit;

    if (is32bit) {
        locaTable.offsets32.resize(numGlyphs + 1);
        for (size_t i = 0; i <= numGlyphs; ++i) {
            uint32_t offset;
            std::memcpy(&offset, &data[locaOffset + i * 4], 4);
            locaTable.offsets32[i] = ntohl(offset); // Assuming big-endian in TTF
        }
    } else {
        locaTable.offsets16.resize(numGlyphs + 1);
        for (size_t i = 0; i <= numGlyphs; ++i) {
            uint16_t offset;
            std::memcpy(&offset, &data[locaOffset + i * 2], 2);
            locaTable.offsets16[i] = ntohs(offset); // Assuming big-endian in TTF
        }
    }

    return locaTable;
}

const std::vector<uint32_t>& LocaTable::getOffsets32() const {
    if (!is32bitFormat) {
        throw std::runtime_error("LocaTable does not contain 32-bit offsets.");
    }
    return offsets32;
}

const std::vector<uint16_t>& LocaTable::getOffsets16() const {
    if (is32bitFormat) {
        throw std::runtime_error("LocaTable does not contain 16-bit offsets.");
    }
    return offsets16;
}
