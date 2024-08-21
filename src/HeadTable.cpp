#include "HeadTable.h"
#include "Helpers.h"
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

// Parse function
HeadTable HeadTable::parseHeadDirectory(const std::vector<char>& data, uint16_t headTableOffset) {
    int offset = headTableOffset;
    uint32_t version = read4Bytes(data, offset);
    uint32_t fontRevision = read4Bytes(data, offset);
    uint32_t checkSumAdjustment = read4Bytes(data, offset);
    uint32_t magicNumber = read4Bytes(data, offset);
    uint16_t flags = read2Bytes(data, offset);
    uint16_t unitsPerEm = read2Bytes(data, offset);
    int64_t created = read8Bytes(data, offset);
    int64_t modified = read8Bytes(data, offset);
    int16_t xMin = read2Bytes(data, offset);
    int16_t yMin = read2Bytes(data, offset);
    int16_t xMax = read2Bytes(data, offset);
    int16_t yMax = read2Bytes(data, offset);
    uint16_t macStyle = read2Bytes(data, offset);
    uint16_t lowestRecPPEM = read2Bytes(data, offset);
    int16_t fontDirectionHint = read2Bytes(data, offset);
    int16_t indexToLocFormat = read2Bytes(data, offset);
    int16_t glyphDataFormat = read2Bytes(data, offset);
    
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
