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

    struct Format4Data {
        uint16_t segCountX2;
        std::vector<uint16_t> endCodes;
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

private:
    uint16_t version;
    uint16_t numSubtables;
    std::vector<CmapSubtable> subtables;
};