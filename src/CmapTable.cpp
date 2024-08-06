#include "CmapTable.h"
#include <vector>
#include <cstdint>
#include <iostream>
#include <fstream>

CmapSubtable::CmapSubtable(uint16_t platformID, uint16_t encodingID, uint16_t format, const std::vector<char>& data, uint32_t offset)
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
    uint32_t pos = offset + 6;
    for (int i = 0; i < 256; ++i) {
        format0Data.glyphIndexArray.push_back(*reinterpret_cast<const uint8_t*>(&data[pos]));
        pos += 1;
    }
}

void CmapSubtable::parseFormat4(const std::vector<char>& data, uint32_t offset) {
    uint32_t pos = offset + 6;
    format4Data.segCountX2 = CmapSubtable::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[pos]));
    pos += 6;
    for (int i = 0; i < format4Data.segCountX2 / 2; ++i) {
        uint16_t endCode = CmapSubtable::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[pos]));
        format4Data.endCodes.push_back(endCode);
        pos += 2;
    }
    pos += 2;
    for (int i = 0; i < format4Data.segCountX2 / 2; ++i) {
        uint16_t startCode = CmapSubtable::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[pos]));
        format4Data.startCodes.push_back(startCode);
        pos += 2;
    }
    for (int i = 0; i < format4Data.segCountX2 / 2; ++i) {
        int16_t idDelta = CmapSubtable::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
        format4Data.idDeltas.push_back(idDelta);
        pos += 2;
    }
    for (int i = 0; i < format4Data.segCountX2 / 2; ++i) {
        uint16_t idRangeOffset = CmapSubtable::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[pos]));
        format4Data.idRangeOffsets.push_back(idRangeOffset);
        pos += 2;
    }
    uint16_t glyphIDArrayLength = 0;
    for (int i = 0; i < format4Data.segCountX2 / 2; ++i) {
        if (format4Data.idRangeOffsets[i] != 0) {
            glyphIDArrayLength += format4Data.endCodes[i] - format4Data.startCodes[i] + 1;
        }
    }
    for (int i = 0; i < glyphIDArrayLength; ++i) {
        uint16_t glyphID = CmapSubtable::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[pos]));
        format4Data.glyphIdArray.push_back(glyphID);
        pos += 2;
    }
}

void CmapSubtable::parseFormat12(const std::vector<char>& data, uint32_t offset) {
    uint32_t pos = offset + 12;
    format12Data.nGroups = CmapSubtable::convertEndian32(*reinterpret_cast<const uint32_t*>(&data[pos]));
    pos += 4;
    format12Data.startCharCodes.resize(format12Data.nGroups);
    format12Data.endCharCodes.resize(format12Data.nGroups);
    format12Data.startGlyphCodes.resize(format12Data.nGroups);

    for (int i = 0; i < format12Data.nGroups; ++i) {
        format12Data.startCharCodes[i] = CmapSubtable::convertEndian32(*reinterpret_cast<const uint32_t*>(&data[pos]));
        pos += 4;
        format12Data.endCharCodes[i] = CmapSubtable::convertEndian32(*reinterpret_cast<const uint32_t*>(&data[pos]));
        pos += 4;
        format12Data.startGlyphCodes[i] = CmapSubtable::convertEndian32(*reinterpret_cast<const uint32_t*>(&data[pos]));
        pos += 4;
    }
}

uint32_t CmapSubtable::getGlyphIndex(uint32_t unicodeValue) const {
    if (format == 4) {
        for (size_t i = 0; i < format4Data.endCodes.size(); ++i) {
            if (unicodeValue <= format4Data.endCodes[i]) {
                if (unicodeValue >= format4Data.startCodes[i]) {
                    uint16_t offset = format4Data.idRangeOffsets[i];
                    if (offset == 0) {
                        return (unicodeValue + format4Data.idDeltas[i]) % 65536;
                    } else {
                        size_t glyphIndexPos = (offset / 2) + (unicodeValue - format4Data.startCodes[i]) - (format4Data.segCountX2 / 2 - i);
                        if (glyphIndexPos < format4Data.glyphIdArray.size()) {
                            return format4Data.glyphIdArray[glyphIndexPos];
                        } else {
                            return 0;
                        }
                    }
                }
                break;
            }
        }
    }
    if (format == 12) {
        for (int i = 0; i < format12Data.nGroups; ++i) {
            if (unicodeValue >= format12Data.startCharCodes[i] && unicodeValue <= format12Data.endCharCodes[i]) {
                return format12Data.startGlyphCodes[i] + (unicodeValue - format12Data.startCharCodes[i]);
            }
        }
    }
    if (format == 0) {
        return format0Data.glyphIndexArray[unicodeValue];
    }
    throw std::runtime_error("unsupported format");
    return 0;
}

CmapTable::CmapTable(const std::vector<char>& data, uint32_t offset){
    uint32_t pos = offset;
    version = CmapTable::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[pos]));
    pos += 2;
    numSubtables = CmapTable::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[pos]));
    pos += 2;
    for (int i = 0; i < numSubtables; ++i) {
        uint16_t platformID = CmapTable::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[pos]));
        pos += 2;
        uint16_t encodingID = CmapTable::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[pos]));
        pos += 2;
        uint32_t subtableOffset = CmapTable::convertEndian32(*reinterpret_cast<const uint32_t*>(&data[pos]));
        pos += 4;
        uint16_t format = CmapTable::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[offset + subtableOffset]));
        subtables.emplace_back(platformID, encodingID, format, data, offset + subtableOffset);
    }

}

CmapTable CmapTable::parse(const std::vector<char>& data, uint32_t offset) {
    return CmapTable(data, offset);
}

uint16_t CmapTable::getGlyphIndex(uint32_t unicodeValue) const {
    // Prioritize Format 12 (UCS-4)
    for (const auto& subtable : subtables) {
        if (subtable.getPlatformID() == 3 && subtable.getEncodingID() == 10) {
            return subtable.getGlyphIndex(unicodeValue);
            break;
        }
    }

    // If no Format 12 found, look for Format 4 (UCS-2)
    for (const auto& subtable : subtables) {
        if (subtable.getPlatformID() == 3 && subtable.getEncodingID() == 1) {
            return subtable.getGlyphIndex(unicodeValue);
            break;
        }
    }

    // If no Format 4 found, look for Format 0 (Macintosh Roman)
    for (const auto& subtable : subtables) {
        if (subtable.getPlatformID() == 1 && subtable.getEncodingID() == 0) {
            return subtable.getGlyphIndex(unicodeValue);
            break;
        }
    }
    std::runtime_error("could not find glyph");
    return 0;
}

uint16_t CmapTable::convertEndian16(uint16_t value) {
    return (value >> 8) | (value << 8);
}

uint32_t CmapTable::convertEndian32(uint32_t value) {
    return ((value >> 24) & 0x000000FF) |
           ((value >> 8)  & 0x0000FF00) |
           ((value << 8)  & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
}

uint16_t CmapSubtable::convertEndian16(uint16_t value) {
    return (value >> 8) | (value << 8);
}

uint32_t CmapSubtable::convertEndian32(uint32_t value) {
    return ((value >> 24) & 0x000000FF) |
           ((value >> 8)  & 0x0000FF00) |
           ((value << 8)  & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
}