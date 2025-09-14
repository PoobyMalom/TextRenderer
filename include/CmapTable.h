#pragma once
#include <vector>
#include <cstdint>

class CmapSubtable {
public: 

    CmapSubtable(uint16_t platformID, uint16_t encodingID, uint16_t format, const std::vector<char>& data, uint32_t offset);

    uint32_t getGlyphIndex(uint32_t unicodeValue) const;
    uint16_t getPlatformID() const { return platformID; }
    uint16_t getEncodingID() const { return encodingID; }
    uint16_t getFormat() const { return format; }
    void printFormat4();

private:

    uint16_t platformID;
    uint16_t encodingID;
    uint16_t format;

    void parseFormat0(const std::vector<char>& data, uint32_t offset);
    void parseFormat4(const std::vector<char>& data, uint32_t offset);
    void parseFormat12(const std::vector<char>& data, uint32_t offset);

    struct Format0Data {
        std::vector<uint8_t> glyphIndexArray;
    };
    Format0Data format0Data;

    /*
    
    UInt16	format	Format number is set to 4
    UInt16	length	Length of subtable in bytes
    UInt16	language	Language code (see above)
    UInt16	segCountX2	2 * segCount
    UInt16	searchRange	2 * (2**FLOOR(log2(segCount)))
    UInt16	entrySelector	log2(searchRange/2)
    UInt16	rangeShift	(2 * segCount) - searchRange
    UInt16	endCode[segCount]	Ending character code for each segment, last = 0xFFFF.
    UInt16	reservedPad	This value should be zero
    UInt16	startCode[segCount]	Starting character code for each segment
    UInt16	idDelta[segCount]	Delta for all character codes in segment
    UInt16	idRangeOffset[segCount]	Offset in bytes to glyph indexArray, or 0
    UInt16	glyphIndexArray[variable]	Glyph index array
    
    */

    struct Format4Data {
        uint16_t format;
        uint16_t length;
        uint16_t language;
        uint16_t segCountX2;
        uint16_t searchRange;
        uint16_t entrySelector;
        uint16_t rangeShift;
        std::vector<uint16_t> endCodes;
        uint16_t reservedPad;
        std::vector<uint16_t> startCodes;
        std::vector<int16_t> idDeltas;
        std::vector<uint16_t> idRangeOffsets;
        std::vector<uint16_t> glyphIdArray;
    };
    Format4Data format4Data;

    struct Format12Data {
        uint32_t nGroups;
        std::vector<uint32_t> startCharCodes;
        std::vector<uint32_t> endCharCodes;
        std::vector<uint32_t> startGlyphCodes;
    };
    Format12Data format12Data;
};

class CmapTable {
public:
    CmapTable(const std::vector<char>& data, uint32_t offset);

    static CmapTable parse(const std::vector<char>& data, uint32_t offset);
    uint16_t getGlyphIndex(uint32_t unicodeValue) const;

    uint16_t getVersion() const { return version; }
    uint16_t getNumSubtables() const { return numSubtables; }
    std::vector<CmapSubtable> getSubtables() const { return subtables; }

private:
    uint16_t version;
    uint16_t numSubtables;
    std::vector<CmapSubtable> subtables;
};