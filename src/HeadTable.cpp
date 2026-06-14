#include "HeadTable.h"
#include "Helpers.h"
#include <cstring>

HeadTable HeadTable::parseHeadDirectory(const std::vector<char>& data, uint32_t headTableOffset) {
    int offset = static_cast<int>(headTableOffset);
    uint32_t version = read4Bytes(data, offset);
    uint32_t fontRevision = read4Bytes(data, offset);
    uint32_t checkSumAdjustment = read4Bytes(data, offset);
    uint32_t magicNumber = read4Bytes(data, offset);
    uint16_t flags = read2Bytes(data, offset);
    uint16_t unitsPerEm = read2Bytes(data, offset);
    auto created = static_cast<int64_t>(read8Bytes(data, offset));
    auto modified = static_cast<int64_t>(read8Bytes(data, offset));
    auto xMin = static_cast<int16_t>(read2Bytes(data, offset));
    auto yMin = static_cast<int16_t>(read2Bytes(data, offset));
    auto xMax = static_cast<int16_t>(read2Bytes(data, offset));
    auto yMax = static_cast<int16_t>(read2Bytes(data, offset));
    uint16_t macStyle = read2Bytes(data, offset);
    uint16_t lowestRecPPEM = read2Bytes(data, offset);
    auto fontDirectionHint = static_cast<int16_t>(read2Bytes(data, offset));
    auto indexToLocFormat = static_cast<int16_t>(read2Bytes(data, offset));
    auto glyphDataFormat = static_cast<int16_t>(read2Bytes(data, offset));

    return {
        version,
        fontRevision,
        checkSumAdjustment,
        magicNumber,
        flags,
        unitsPerEm,
        created,
        modified,
        xMin,
        yMin,
        xMax,
        yMax,
        macStyle,
        lowestRecPPEM,
        fontDirectionHint,
        indexToLocFormat,
        glyphDataFormat
    };
}
