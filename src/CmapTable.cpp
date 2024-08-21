#include "CmapTable.h"
#include "Helpers.h"
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
    int pos = offset + 6;
    for (int i = 0; i < 256; ++i) {
        format0Data.glyphIndexArray.push_back(readByte(data, pos));
    }
}
void CmapSubtable::parseFormat4(const std::vector<char>& data, uint32_t offset) {
    int pos = offset + 6;
    format4Data.segCountX2 = read2Bytes(data, pos);
    for (int i = 0; i < format4Data.segCountX2 / 2; ++i) { format4Data.endCodes.push_back(read2Bytes(data, pos)); }
    pos += 2;
    for (int i = 0; i < format4Data.segCountX2 / 2; ++i) { format4Data.startCodes.push_back(read2Bytes(data, pos)); }
    for (int i = 0; i < format4Data.segCountX2 / 2; ++i) { format4Data.idDeltas.push_back(read2Bytes(data, pos)); }
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
    int pos = offset + 12;
    format12Data.nGroups = read4Bytes(data, pos);
    pos += 4;
    format12Data.startCharCodes.resize(format12Data.nGroups);
    format12Data.endCharCodes.resize(format12Data.nGroups);
    format12Data.startGlyphCodes.resize(format12Data.nGroups);

    for (int i = 0; i < format12Data.nGroups; ++i) {
        format12Data.startCharCodes[i] = read4Bytes(data, pos);
        format12Data.endCharCodes[i] = read4Bytes(data, pos);
        format12Data.startGlyphCodes[i] = read4Bytes(data, pos);

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
    int pos = offset;
    cout << "cmap constructor offset: " << pos << endl;
    version = read2Bytes(data, pos);
    cout << "cmap version: " << version << endl;
    numSubtables = read2Bytes(data, pos);
    cout << "cmap subtable count: " << numSubtables << endl;
    for (int i = 0; i < numSubtables; ++i) {
        uint16_t platformID = read2Bytes(data, pos);
        uint16_t encodingID = read2Bytes(data, pos);
        uint32_t subtableOffset = read4Bytes(data, pos);
        uint16_t format = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[offset + subtableOffset]));
        cout << "cmap table " << i << " platformID: " << platformID << " | encodingID: " << encodingID << " | subtableOffset: " << subtableOffset << "| format: " << format << endl;
        subtables.emplace_back(platformID, encodingID, format, data, offset + subtableOffset);
    }

}

CmapTable CmapTable::parse(const std::vector<char>& data, uint32_t offset) { return CmapTable(data, offset); }

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