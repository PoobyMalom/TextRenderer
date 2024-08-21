#pragma once
#include <cstdint>
#include <vector>

class HeadTable {
public:
    HeadTable(
        uint32_t version,
        uint32_t fontRevision,
        uint32_t checkSumAdjustment,
        uint32_t magicNumber,
        uint16_t flags,
        uint16_t unitsPerEm,
        int64_t created,
        int64_t modified,
        int16_t xMin,
        int16_t yMin,
        int16_t xMax,
        int16_t yMax,
        uint16_t macStyle,
        uint16_t lowestRecPPEM,
        int16_t fontDirectionHint,
        int16_t indexToLocFormat,
        int16_t glyphDataFormat
    );

    uint32_t getVersion() const;
    uint32_t getFontRevision() const;
    uint32_t getCheckSumAdjustment() const;
    uint32_t getMagicNumber() const;
    uint16_t getFlags() const;
    uint16_t getUnitsPerEm() const;
    int64_t getCreated() const;
    int64_t getModified() const;
    int16_t getXMIN() const;
    int16_t getYMIN() const;
    int16_t getXMAX() const;
    int16_t getYMAX() const;
    uint16_t getMacStyle() const;
    uint16_t getLowestRecPPEM() const;
    int16_t getFontDirectionHint() const;
    int16_t getIndexToLocFormat() const;
    int16_t getGlyphDataFormat() const;

    static HeadTable parseHeadDirectory(const std::vector<char>& data, uint16_t headTableOffset);

private:
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
};