#include "HeadTable.h"
#include <cstring>

// Constructor
HeadTable::HeadTable(
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
) : version(version),
    fontRevision(fontRevision),
    checkSumAdjustment(checkSumAdjustment),
    magicNumber(magicNumber),
    flags(flags),
    unitsPerEm(unitsPerEm),
    created(created),
    modified(modified),
    xMin(xMin),
    yMin(yMin),
    xMax(xMax),
    yMax(yMax),
    macStyle(macStyle),
    lowestRecPPEM(lowestRecPPEM),
    fontDirectionHint(fontDirectionHint),
    indexToLocFormat(indexToLocFormat),
    glyphDataFormat(glyphDataFormat) {}

// Getters
uint32_t HeadTable::getVersion() const { return version; }
uint32_t HeadTable::getFontRevision() const { return fontRevision; }
uint32_t HeadTable::getCheckSumAdjustment() const { return checkSumAdjustment; }
uint32_t HeadTable::getMagicNumber() const { return magicNumber; }
uint16_t HeadTable::getFlags() const { return flags; }
uint16_t HeadTable::getUnitsPerEm() const { return unitsPerEm; }
int64_t HeadTable::getCreated() const { return created; }
int64_t HeadTable::getModified() const { return modified; }
int16_t HeadTable::getXMIN() const { return xMin; }
int16_t HeadTable::getYMIN() const { return yMin; }
int16_t HeadTable::getXMAX() const { return xMax; }
int16_t HeadTable::getYMAX() const { return yMax; }
uint16_t HeadTable::getMacStyle() const { return macStyle; }
uint16_t HeadTable::getLowestRecPPEM() const { return lowestRecPPEM; }
int16_t HeadTable::getFontDirectionHint() const { return fontDirectionHint; }
int16_t HeadTable::getIndexToLocFormat() const { return indexToLocFormat; }
int16_t HeadTable::getGlyphDataFormat() const { return glyphDataFormat; }

// Endian conversion functions
uint16_t HeadTable::convertEndian16(uint16_t value) {
    return (value >> 8) | (value << 8);
}

uint32_t HeadTable::convertEndian32(uint32_t value) {
    return ((value >> 24) & 0x000000FF) |
           ((value >> 8)  & 0x0000FF00) |
           ((value << 8)  & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
}

uint64_t HeadTable::convertEndian64(uint64_t value) {
    return ((value >> 56) & 0x00000000000000FF) |
           ((value >> 40) & 0x000000000000FF00) |
           ((value >> 24) & 0x0000000000FF0000) |
           ((value >> 8)  & 0x00000000FF000000) |
           ((value << 8)  & 0x000000FF00000000) |
           ((value << 24) & 0x0000FF0000000000) |
           ((value << 40) & 0x00FF000000000000) |
           ((value << 56) & 0xFF00000000000000);
}

// Parse function
HeadTable HeadTable::parseHeadDirectory(const std::vector<char>& data, uint16_t headTableOffset) {
    uint32_t version = convertEndian32(*reinterpret_cast<const uint32_t*>(&data[headTableOffset]));
    uint32_t fontRevision = convertEndian32(*reinterpret_cast<const uint32_t*>(&data[headTableOffset + 4]));
    uint32_t checkSumAdjustment = convertEndian32(*reinterpret_cast<const uint32_t*>(&data[headTableOffset + 8]));
    uint32_t magicNumber = convertEndian32(*reinterpret_cast<const uint32_t*>(&data[headTableOffset + 12]));
    uint16_t flags = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[headTableOffset + 16]));
    uint16_t unitsPerEm = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[headTableOffset + 18]));
    int64_t created = convertEndian64(*reinterpret_cast<const uint64_t*>(&data[headTableOffset + 20]));
    int64_t modified = convertEndian64(*reinterpret_cast<const uint64_t*>(&data[headTableOffset + 28]));
    int16_t xMin = convertEndian16(*reinterpret_cast<const int16_t*>(&data[headTableOffset + 36]));
    int16_t yMin = convertEndian16(*reinterpret_cast<const int16_t*>(&data[headTableOffset + 38]));
    int16_t xMax = convertEndian16(*reinterpret_cast<const int16_t*>(&data[headTableOffset + 40]));
    int16_t yMax = convertEndian16(*reinterpret_cast<const int16_t*>(&data[headTableOffset + 42]));
    uint16_t macStyle = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[headTableOffset + 44]));
    uint16_t lowestRecPPEM = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[headTableOffset + 46]));
    int16_t fontDirectionHint = convertEndian16(*reinterpret_cast<const int16_t*>(&data[headTableOffset + 48]));
    int16_t indexToLocFormat = convertEndian16(*reinterpret_cast<const int16_t*>(&data[headTableOffset + 50]));
    int16_t glyphDataFormat = convertEndian16(*reinterpret_cast<const int16_t*>(&data[headTableOffset + 52]));

    return HeadTable(
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
    );
}
