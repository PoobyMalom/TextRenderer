#include "TTFFile.h"
#include "Helpers.h"
#include <iostream>

using namespace std;

TTFFile::TTFFile(
    TTFHeader header,
    vector<uint32_t> locas,
    const HeadTable& headTable,
    CmapTable cmapTable,
    const MaxpTable& maxpTable
) : header(std::move(header)),
    locas(std::move(locas)),
    headTable(headTable),
    cmapTable(std::move(cmapTable)),
    maxpTable(maxpTable) {}

const TTFHeader& TTFFile::getHeader() const { return header; }
const vector<uint32_t>& TTFFile::getLocas() const { return locas; }
const HeadTable& TTFFile::getHeadTable() const { return headTable; }
const CmapTable& TTFFile::getCmapTable() const { return cmapTable; }
const MaxpTable& TTFFile::getMaxpTable() const { return maxpTable; }

TTFFile TTFFile::parse(const std::vector<char>& data) {
    TTFHeader header = TTFHeader::parse(data);
    header.parseTables(data);
    TableMap tableMap = header.getTables();

    HeadTable headTable = HeadTable::parseHeadDirectory(data, tableMap.at("head").getOffset());
    MaxpTable maxpTable = MaxpTable::parseMaxpDirectory(data, tableMap.at("maxp").getOffset());
    bool indexToLocFormat = static_cast<bool>(headTable.indexToLocFormat);
    LocaTable locaTable = LocaTable::parse(indexToLocFormat, data, tableMap.at("loca").getOffset(), maxpTable.numGlyphs);

    vector<uint32_t> locas;

    if (indexToLocFormat) {
        const std::vector<uint32_t>& locas32 = locaTable.getOffsets32();
        locas.insert(locas.end(), locas32.begin(), locas32.end());
    } else {
        const std::vector<uint16_t>& locas16 = locaTable.getOffsets16();
        locas.insert(locas.end(), locas16.begin(), locas16.end());
    }

    CmapTable cmapTable = CmapTable::parse(data, tableMap.at("cmap").getOffset());
    return {header, locas, headTable, cmapTable, maxpTable};
}

Glyph TTFFile::parseGlyph(const std::vector<char>& data, uint32_t unicode) {
    if (unicode == 32) {
        return Glyph{0, 0, 0, 0, 0, {}, 0, {}, {}, {}, {}};
    }

    uint16_t glyphIndex = cmapTable.getGlyphIndex(unicode);

    if (glyphIndex >= locas.size()) {
        std::cerr << "Invalid glyph index: " << glyphIndex << '\n';
        throw std::out_of_range("Glyph index out of range");
    }

    uint32_t locaOffset = locas[glyphIndex];

    if (locaOffset >= data.size()) {
        std::cerr << "Invalid loca offset: " << locaOffset << '\n';
        throw std::out_of_range("Loca offset out of range");
    }

    uint32_t glyfOffset = header.getTables().at("glyf").getOffset();
    uint32_t glyphOffset = glyfOffset + locaOffset;

    if (glyphOffset >= data.size()) {
        std::cerr << "Invalid glyph offset: " << glyphOffset << '\n';
        throw std::out_of_range("Glyph offset out of range");
    }
    Glyph parsedGlyph = Glyph::parseGlyph(data, locas, glyfOffset, glyphOffset);
    parsedGlyph.addPointsBetween();
    return parsedGlyph;
}

std::vector<Glyph> TTFFile::parseGlyphs(const std::vector<char>& data, const std::string& letters) {
    std::vector<uint32_t> chars = stringToUnicode(letters);
    std::vector<Glyph> glyphs;
    glyphs.reserve(chars.size());
    for (uint32_t charCode : chars) {
        glyphs.push_back(parseGlyph(data, charCode));
    }
    return glyphs;
}
