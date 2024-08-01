#pragma once
#include <vector>
#include <string>
#include <map>
#include "TTFTable.h"
#include "TTFHeader.h"
#include "GlyphTable.h"
#include "LocaTable.h"
#include "HeadTable.h"
#include "CmapTable.h"
#include "MaxpTable.h"

class TTFFile {
public:
    TTFFile(
        TTFHeader header,
        std::vector<TTFTable*> tables,
        std::vector<uint32_t> locas,
        HeadTable headTable,
        CmapTable cmapTable,
        MaxpTable maxpTable,
        uint16_t cmapOffset,
        uint16_t glyfOffset,
        uint16_t headOffset,
        uint16_t locaOffset,
        uint16_t maxpOffset
    );
    
    TTFHeader getHeader() const;
    std::vector<TTFTable*> getTables() const;
    std::vector<uint32_t> getLocas() const;
    HeadTable getHeadTable() const;
    CmapTable getCmapTable() const;
    MaxpTable getMaxpTable() const;
    uint16_t getCmapOffset() const;
    uint16_t getGlyfOffset() const;
    uint16_t getHeadOffset() const;
    uint16_t getLocaOffset() const;
    uint16_t getMaxpOffset() const;

    static TTFFile parse(const std::vector<char>& data);
    Glyph parseGlyph(const std::vector<char>& data, uint16_t platformID, uint16_t endcodingID, char letter);
    std::vector<Glyph> parseGlyphs(const std::vector<char>& data, uint16_t platformID, uint16_t endcodingID, std::string letters);

private:
    TTFHeader header;
    std::vector<TTFTable*> tables;
    std::vector<uint32_t> locas;
    HeadTable headTable;
    CmapTable cmapTable;
    MaxpTable maxpTable;
    uint16_t cmapOffset;
    uint16_t glyfOffset;
    uint16_t headOffset;
    uint16_t locaOffset;
    uint16_t maxpOffset;
};