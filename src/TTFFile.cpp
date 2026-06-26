#include "TTFFile.h"
#include "Helpers.h"
#include <iostream>

using namespace std;

TTFFile::TTFFile(
    TTFHeader header,
    vector<uint32_t> locas,
    const HeadTable& headTable,
    CmapTable cmapTable,
    const MaxpTable& maxpTable,
    const Metrics& metricsTable
) : header(move(header)),
    locas(move(locas)),
    headTable(headTable),
    cmapTable(move(cmapTable)),
    maxpTable(maxpTable),
    metricsTable(metricsTable) {}

const TTFHeader& TTFFile::getHeader() const { return header; }
const vector<uint32_t>& TTFFile::getLocas() const { return locas; }
const HeadTable& TTFFile::getHeadTable() const { return headTable; }
const CmapTable& TTFFile::getCmapTable() const { return cmapTable; }
const MaxpTable& TTFFile::getMaxpTable() const { return maxpTable; }
const Metrics&   TTFFile::getMetricsTable() const { return metricsTable; }

TTFFile TTFFile::parse(const vector<char>& data) {
    TTFHeader header = TTFHeader::parse(data);
    header.parseTables(data);
    TableMap tableMap = header.getTables();

    HeadTable headTable = HeadTable::parseHeadDirectory(data, tableMap.at("head").getOffset());
    MaxpTable maxpTable = MaxpTable::parseMaxpDirectory(data, tableMap.at("maxp").getOffset());
    Metrics   metricsTable = {};
    metricsTable.getMetrics(data, tableMap.at("hhea").getOffset(), tableMap.at("hmtx").getOffset(), maxpTable.numGlyphs);
    bool indexToLocFormat = static_cast<bool>(headTable.indexToLocFormat);
    LocaTable locaTable = LocaTable::parse(indexToLocFormat, data, tableMap.at("loca").getOffset(), maxpTable.numGlyphs);

    const vector<uint32_t>& locas = locaTable.getOffsets();

    CmapTable cmapTable = CmapTable::parse(data, tableMap.at("cmap").getOffset());
    return {header, locas, headTable, cmapTable, maxpTable, metricsTable};
}

Glyph TTFFile::parseGlyph(const vector<char>& data, uint32_t unicode) {
    uint16_t glyphIndex = cmapTable.getGlyphIndex(unicode);

    if (unicode == 32) {
        Glyph space = {0, 0, 0, 0, 0, {}, 0, {}, {}, {}, {}, 0, 0};
        space.setAdvanceWidth(metricsTable.getLongHorMetrics()[glyphIndex].advanceWidth);
        return space;
    }


    if (glyphIndex >= locas.size()) {
        cerr << "Invalid glyph index: " << glyphIndex << '\n';
        throw out_of_range("Glyph index out of range");
    }

    uint32_t locaOffset = locas[glyphIndex];

    if (locaOffset >= data.size()) {
        cerr << "Invalid loca offset: " << locaOffset << '\n';
        throw out_of_range("Loca offset out of range");
    }

    uint32_t glyfOffset = header.getTables().at("glyf").getOffset();
    uint32_t glyphOffset = glyfOffset + locaOffset;

    if (glyphOffset >= data.size()) {
        cerr << "Invalid glyph offset: " << glyphOffset << '\n';
        throw out_of_range("Glyph offset out of range");
    }
    Glyph parsedGlyph = Glyph::parseGlyph(data, locas, glyfOffset, glyphOffset);
    parsedGlyph.setAdvanceWidth(metricsTable.getLongHorMetrics()[glyphIndex].advanceWidth);
    parsedGlyph.setLeftSideBearing(metricsTable.getLongHorMetrics()[glyphIndex].leftSideBearing);
    parsedGlyph.addPointsBetween();
    return parsedGlyph;
}

vector<Glyph> TTFFile::parseGlyphs(const vector<char>& data, const string& letters) {
    vector<uint32_t> chars = stringToUnicode(letters);
    vector<Glyph> glyphs;
    glyphs.reserve(chars.size());
    for (uint32_t charCode : chars) {
        glyphs.push_back(parseGlyph(data, charCode));
    }
    return glyphs;
}
