#include "LocaTable.h"
#include "Helpers.h"
#include <cstring>
#include <arpa/inet.h> // For ntohl and ntohs

LocaTable LocaTable::parse(bool is32bit, const std::vector<char>& data, size_t locaOffset, size_t numGlyphs) {
    LocaTable locaTable;
    locaTable.is32bitFormat = is32bit;
    int pos = locaOffset;

    if (is32bit) {
        locaTable.offsets32.resize(numGlyphs + 1);
        for (size_t i = 0; i <= numGlyphs; ++i) {
            locaTable.offsets32[i] = read4Bytes(data, pos);
        }
    } else {
        locaTable.offsets16.resize(numGlyphs + 1);
        for (size_t i = 0; i <= numGlyphs; ++i) {
            locaTable.offsets16[i] = read2Bytes(data, pos);
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
