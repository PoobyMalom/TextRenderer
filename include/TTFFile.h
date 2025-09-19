#ifndef TTFFILE_H
#define TTFFILE_H

#include "TTFHeader.h"
#include "TTFTable.h"
#include "HeadTable.h"
#include "CmapTable.h"
#include "MaxpTable.h"
#include "LocaTable.h"
#include "GlyphTable.h"
#include "HheaTable.h"
#include "NameTable.h"
#include "PostTable.h"
#include <vector>
#include <string>

class TTFFile {
public:
    TTFFile(
        TTFHeader header,
        std::vector<TTFTable*> tables,
        std::vector<uint32_t> locas,
        LocaTable locaTable,
        HeadTable headTable,
        CmapTable cmapTable,
        MaxpTable maxpTable,
        HheaTable hheaTable,
        uint32_t cmapOffset,
        uint32_t glyfOffset,
        uint32_t headOffset,
        uint32_t locaOffset,
        uint32_t maxpOffset
    );

    TTFHeader getHeader() const;
    std::vector<TTFTable*> getTables() const;
    std::vector<uint32_t> getLocas() const;
    LocaTable getLocaTable() const;
    HeadTable getHeadTable() const;
    CmapTable getCmapTable() const;
    MaxpTable getMaxpTable() const;
    HheaTable getHheaTable() const;
    uint32_t getCmapOffset() const;
    uint32_t getGlyfOffset() const;
    uint32_t getHeadOffset() const;
    uint32_t getLocaOffset() const;
    uint32_t getMaxpOffset() const;

    static TTFFile parse(const std::vector<char>& data);
    Glyph parseGlyph(const std::vector<char>& data, uint32_t unicode);
    std::vector<Glyph> parseGlyphs(const std::vector<char>& data, std::string letters);

private:
    TTFHeader header;
    std::vector<TTFTable*> tables;
    std::vector<uint32_t> locas;
    LocaTable locaTable;
    HeadTable headTable;
    CmapTable cmapTable;
    MaxpTable maxpTable;
    HheaTable hheaTable;
    uint32_t cmapOffset;
    uint32_t glyfOffset;
    uint32_t headOffset;
    uint32_t locaOffset;
    uint32_t maxpOffset;
};

#endif // TTFFILE_H
