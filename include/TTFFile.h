#ifndef TTFFILE_H
#define TTFFILE_H

#include "TTFHeader.h"
#include "TTFTable.h"
#include "HeadTable.h"
#include "CmapTable.h"
#include "MaxpTable.h"
#include "LocaTable.h"
#include "GlyphTable.h"
#include <vector>
#include <string>

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
    Glyph parseGlyph(const std::vector<char>& data, uint32_t unicode);
    std::vector<Glyph> parseGlyphs(const std::vector<char>& data, std::string letters);

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

#endif // TTFFILE_H
