#include "CmapTable.h"
#include "Helpers.h"
#include <vector>
#include <cstdint>
#include <iostream>
#include <fstream>

using namespace std;

CmapSubtable::CmapSubtable(uint16_t platformID, uint16_t encodingID, uint16_t format, const std::vector<char>& data, uint32_t offset) // NOLINT(bugprone-easily-swappable-parameters)
    : platformID(platformID), encodingID(encodingID), format(format) {
        switch (format) {
            case 0:
                parseFormat0(data, offset);
                break;
            case 4:
                parseFormat4(data, offset);
                break;
            case 12:
                parseFormat12(data, offset);
                break;
            default:
                throw std::runtime_error("Unsupported cmap subtable format");
        }
    }

void CmapSubtable::parseFormat0(const std::vector<char>& data, uint32_t offset) {
    int pos = static_cast<int>(offset + 6);
    for (int i = 0; i < 256; ++i) {
        format0Data.glyphIndexArray.push_back(readByte(data, pos));
    }
}

void CmapSubtable::parseFormat4(const std::vector<char>& data, uint32_t offset) {
    int pos = static_cast<int>(offset + 6);
    format4Data.segCountX2 = read2Bytes(data, pos);
    pos += 6; // Consume searchRange, entrySelector, rangeShift for now
    for (int i = 0; i < format4Data.segCountX2 / 2; ++i) { format4Data.endCodes.push_back(read2Bytes(data, pos)); }
    pos += 2; // Consume reservedPad
    for (int i = 0; i < format4Data.segCountX2 / 2; ++i) { format4Data.startCodes.push_back(read2Bytes(data, pos)); }
    for (int i = 0; i < format4Data.segCountX2 / 2; ++i) { format4Data.idDeltas.push_back(static_cast<int16_t>(read2Bytes(data, pos))); }
    for (int i = 0; i < format4Data.segCountX2 / 2; ++i) { format4Data.idRangeOffsets.push_back(read2Bytes(data, pos)); }
    uint16_t glyphIDArrayLength = 0;
    for (int i = 0; i < format4Data.segCountX2 / 2; ++i) {
        if (format4Data.idRangeOffsets[i] != 0) {
            glyphIDArrayLength += format4Data.endCodes[i] - format4Data.startCodes[i] + 1;
        }
    }
    for (int i = 0; i < glyphIDArrayLength; ++i) { format4Data.glyphIdArray.push_back(read2Bytes(data, pos)); }
}

void CmapSubtable::parseFormat12(const std::vector<char>& data, uint32_t offset) {
    int pos = static_cast<int>(offset + 12);
    format12Data.nGroups = read4Bytes(data, pos);
    format12Data.startCharCodes.resize(format12Data.nGroups);
    format12Data.endCharCodes.resize(format12Data.nGroups);
    format12Data.startGlyphCodes.resize(format12Data.nGroups);

    for (uint32_t i = 0; i < format12Data.nGroups; ++i) {
        format12Data.startCharCodes[i] = read4Bytes(data, pos);
        format12Data.endCharCodes[i] = read4Bytes(data, pos);
        format12Data.startGlyphCodes[i] = read4Bytes(data, pos);

    }
}

uint32_t CmapSubtable::getGlyphIndex(uint32_t unicodeValue) const { // NOLINT(readability-function-cognitive-complexity)
    if (format == 4) {
        for (size_t i = 0; i < format4Data.endCodes.size(); ++i) {
            if (unicodeValue <= format4Data.endCodes[i]) {
                if (unicodeValue >= format4Data.startCodes[i]) {
                    uint16_t offset = format4Data.idRangeOffsets[i];
                    if (offset == 0) {
                        return (unicodeValue + format4Data.idDeltas[i]) % 65536;
                    }
                    size_t glyphIndexPos = (offset / 2) + (unicodeValue - format4Data.startCodes[i]) - (format4Data.segCountX2 / 2 - i);
                    if (glyphIndexPos < format4Data.glyphIdArray.size()) {
                        return format4Data.glyphIdArray[glyphIndexPos];
                    }
                    return 0;
                }
                break;
            }
        }
    }
    if (format == 12) {
        for (uint32_t i = 0; i < format12Data.nGroups; ++i) {
            if (unicodeValue >= format12Data.startCharCodes[i] && unicodeValue <= format12Data.endCharCodes[i]) {
                return format12Data.startGlyphCodes[i] + (unicodeValue - format12Data.startCharCodes[i]);
            }
        }
    }
    if (format == 0) {
        return format0Data.glyphIndexArray[unicodeValue];
    }
    throw std::runtime_error("unsupported format");
}

CmapTable::CmapTable(const std::vector<char>& data, uint32_t offset){
    int pos = static_cast<int>(offset);
    version = read2Bytes(data, pos);
    numSubtables = read2Bytes(data, pos);
    subtables.reserve(numSubtables);
    for (int i = 0; i < numSubtables; ++i) {
        uint16_t platformID = read2Bytes(data, pos);
        uint16_t encodingID = read2Bytes(data, pos);
        uint32_t subtableOffset = read4Bytes(data, pos);
        uint16_t format = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[offset + subtableOffset]));
        subtables.emplace_back(platformID, encodingID, format, data, offset + subtableOffset);
    }

    for (size_t i = 0; i < subtables.size(); ++i) {
        if (subtables[i].getPlatformID() == 3 && subtables[i].getEncodingID() == 10) { activeSubtableIndex = static_cast<int>(i); break; }
    }
    if (activeSubtableIndex == -1) {
        for (size_t i = 0; i < subtables.size(); ++i) {
            if (subtables[i].getPlatformID() == 3 && subtables[i].getEncodingID() == 1) { activeSubtableIndex = static_cast<int>(i); break; }
        }
    }
    if (activeSubtableIndex == -1) {
        for (size_t i = 0; i < subtables.size(); ++i) {
            if (subtables[i].getPlatformID() == 1 && subtables[i].getEncodingID() == 0) { activeSubtableIndex = static_cast<int>(i); break; }
        }
    }
}

CmapTable CmapTable::parse(const std::vector<char>& data, uint32_t offset) { return {data, offset}; }

uint16_t CmapTable::getGlyphIndex(uint32_t unicodeValue) const {
    if (activeSubtableIndex == -1) {
        throw std::runtime_error("no supported cmap subtable found");
    }
    const CmapSubtable& active = subtables[activeSubtableIndex];
    return active.getGlyphIndex(unicodeValue);
}