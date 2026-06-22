#pragma once

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
        std::vector<uint32_t> locas,
        const HeadTable& headTable,
        CmapTable cmapTable,
        const MaxpTable& maxpTable
    );

    const TTFHeader& getHeader() const;
    const std::vector<uint32_t>& getLocas() const;
    const HeadTable& getHeadTable() const;
    const CmapTable& getCmapTable() const;
    const MaxpTable& getMaxpTable() const;

    static TTFFile parse(const std::vector<char>& data);
    Glyph parseGlyph(const std::vector<char>& data, uint32_t unicode);
    std::vector<Glyph> parseGlyphs(const std::vector<char>& data, const std::string& letters);

private:
    TTFHeader header;
    std::vector<uint32_t> locas;
    HeadTable headTable;
    CmapTable cmapTable;
    MaxpTable maxpTable;
};

