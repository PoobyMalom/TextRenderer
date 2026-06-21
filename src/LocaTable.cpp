#include "LocaTable.h"
#include "Helpers.h"
#include <cstring>

LocaTable LocaTable::parse(bool is32bit, const std::vector<char>& data, size_t locaOffset, size_t numGlyphs) { // NOLINT(bugprone-easily-swappable-parameters)
    LocaTable locaTable;
    int pos = static_cast<int>(locaOffset);

    if (is32bit) {
        locaTable.offsets.resize(numGlyphs + 1);
        for (size_t i = 0; i <= numGlyphs; ++i) {
            locaTable.offsets[i] = read4Bytes(data, pos);
        }
    } else {
        locaTable.offsets.resize(numGlyphs + 1);
        for (size_t i = 0; i <= numGlyphs; ++i) {
            locaTable.offsets[i] = read2Bytes(data, pos) * 2u;
        }
    }

    return locaTable;
}

const std::vector<uint32_t>& LocaTable::getOffsets() const {
    return offsets;
}
