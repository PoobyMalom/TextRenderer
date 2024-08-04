#include "CmapTable.h"
#include <vector>
#include <cstdint>
#include <iostream>
#include <fstream>

CmapSubtable::CmapSubtable(uint16_t platformID, uint16_t encodingID, uint16_t format, const std::vector<char>& data, uint32_t offset)
    : platformID(platformID), encodingID(encodingID), format(format) {
        switch (format) {
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

void CmapSubtable::parseFormat4(const std::vector<char>& data, uint16_t offset) {
    uint16_t pos = offset;
    format4Data.segCountX2 = (data[pos] << 8) | data[pos + 1];
    pos += 2;
    format4Data.endCodes.resize(format4Data.segCountX2 / 2);
    for (int i = 0; i < format4Data.endCodes.size(); ++i) {
        format4Data.endCodes[i] = (data[pos] << 8) | data[pos + 1];
        pos += 2;
    }
    pos += 2;


}

void CmapSubtable::parseFormat12(const std::vector<char>& data, uint32_t offset) {
    uint32_t pos = offset + 12;
    format12Data.nGroups = CmapSubtable::convertEndian32(*reinterpret_cast<const uint32_t*>(&data[pos]));
    //std::cout << "nGroups: " << format12Data.nGroups << std::endl; 
    pos += 4;
    std::cout << "segCountX2: " << format4Data.segCountX2 << std::endl;
    std::cout << "nGroups: " << format12Data.nGroups << std::endl;
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
    //std::cout << format12Data.startGlyphCodes[3] << std::endl;
}

uint32_t CmapSubtable::getGlyphIndex(uint32_t unicodeValue) const {
    if (format == 4) {
        for (int i = 0; i < format4Data.endCodes.size(); ++i) {
            if (unicodeValue <= format4Data.endCodes[i]) {
                if (unicodeValue >= format4Data.startCodes[i]) {
                    uint16_t offset = format4Data.idRangeOffsets[i];
                    if (offset == 0) {
                        return (unicodeValue + format4Data.idDeltas[i]) % 65536;
                    } else {
                        return 0;
                    }
                    break;
                }
            }
        }
    }
    if (format == 12) {
        for (int i = 0; i < format12Data.nGroups; ++i) {
            if (unicodeValue >= format12Data.startCharCodes[i] && unicodeValue <= format12Data.endCharCodes[i]) {
                //std::cout << "startCharCode " << i << ": " << format12Data.startCharCodes[i] << std::endl;
                //std::cout << "endCharCode " << i << ": " << format12Data.endCharCodes[i] << std::endl;
                //std::cout << "startGlyphCode " << i << ": " << format12Data.startGlyphCodes[i] << std::endl; 
                //std::cout << "Returning: " << format12Data.startGlyphCodes[i] + (unicodeValue - format12Data.startCharCodes[i]) << std::endl;
                return format12Data.startGlyphCodes[i] + (unicodeValue - format12Data.startCharCodes[i]);
            }
        }
    }
    throw std::runtime_error("unsupported format");
    return 0;
}

CmapTable::CmapTable(const std::vector<char>& data, uint32_t offset){
    uint32_t pos = offset;
    version = CmapTable::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[pos]));
    //std::cout << "version confirm: " << version << std::endl;
    pos += 2;
    numSubtables = CmapTable::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[pos]));
    //std::cout << "numSubtable confirm: " << numSubtables << std::endl;
    pos += 2;
    //std::cout << "------------------------------------" << std::endl;

    for (int i = 0; i < numSubtables; ++i) {
        uint16_t platformID = CmapTable::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[pos]));
        //std::cout << "platformID confirm: " << platformID << std::endl;
        pos += 2;
        uint16_t encodingID = CmapTable::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[pos]));
        //std::cout << "encodingID confirm: " << encodingID << std::endl;
        pos += 2;
        uint32_t subtableOffset = CmapTable::convertEndian32(*reinterpret_cast<const uint32_t*>(&data[pos]));
        pos += 4;
        //std::cout << "subtableOffset confirm: " << subtableOffset << std::endl;
        uint16_t format = CmapTable::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[offset + subtableOffset]));
        //std::cout << "format confirm: " << format << std::endl;
        subtables.emplace_back(platformID, encodingID, format, data, offset + subtableOffset);
        //std::cout << "------------------------------------" << std::endl;
    }

}

CmapTable CmapTable::parse(const std::vector<char>& data, uint32_t offset) {
    return CmapTable(data, offset);
}

uint16_t CmapTable::getGlyphIndex(uint32_t unicodeValue, uint16_t platformID, uint16_t encodingID) const {
    for (const auto& subtable : subtables) {
        if (subtable.getPlatformID() == platformID && subtable.getEncodingID() == encodingID) {
            return subtable.getGlyphIndex(unicodeValue);
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