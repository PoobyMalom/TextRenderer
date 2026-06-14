#pragma once
#include <cstdint>
#include <vector>

struct HeadTable {
    uint32_t version;
    uint32_t fontRevision;
    uint32_t checkSumAdjustment;
    uint32_t magicNumber;
    uint16_t flags;
    uint16_t unitsPerEm;
    int64_t created;
    int64_t modified;
    int16_t xMin;
    int16_t yMin;
    int16_t xMax;
    int16_t yMax;
    uint16_t macStyle;
    uint16_t lowestRecPPEM;
    int16_t fontDirectionHint;
    int16_t indexToLocFormat;
    int16_t glyphDataFormat;

    static HeadTable parseHeadDirectory(const std::vector<char>& data, uint32_t headTableOffset);
};
