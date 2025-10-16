#include "TTFFile.h"
#include "Helpers.h"
#include <iostream>

using namespace std;

TTFFile::TTFFile(
    TTFHeader header,
    vector<TTFTable*> tables,
    vector<uint32_t> locas,
    LocaTable locaTable,
    HeadTable headTable,
    CmapTable cmapTable,
    MaxpTable maxpTable,
    HheaTable hheaTable,
    HmtxTable hmtxTable,
    uint32_t cmapOffset,
    uint32_t glyfOffset,
    uint32_t headOffset,
    uint32_t locaOffset,
    uint32_t maxpOffset
) : header(header),
    tables(tables),
    locas(locas),
    locaTable(locaTable),
    headTable(headTable),
    cmapTable(cmapTable),
    maxpTable(maxpTable),
    hheaTable(hheaTable),
    hmtxTable(hmtxTable),
    cmapOffset(cmapOffset),
    glyfOffset(glyfOffset),
    headOffset(headOffset),
    locaOffset(locaOffset),
    maxpOffset(maxpOffset) {}

TTFHeader TTFFile::getHeader() const { return header; }
vector<TTFTable*> TTFFile::getTables() const { return tables; }
vector<uint32_t> TTFFile::getLocas() const { return locas; }
LocaTable TTFFile::getLocaTable() const { return locaTable; }
HeadTable TTFFile::getHeadTable() const { return headTable; }
CmapTable TTFFile::getCmapTable() const { return cmapTable; }
MaxpTable TTFFile::getMaxpTable() const { return maxpTable; }
HheaTable TTFFile::getHheaTable() const { return hheaTable; }
HmtxTable TTFFile::getHmtxTable() const { return hmtxTable; }
uint32_t TTFFile::getCmapOffset() const { return cmapOffset; }
uint32_t TTFFile::getGlyfOffset() const { return glyfOffset; }
uint32_t TTFFile::getHeadOffset() const { return headOffset; }
uint32_t TTFFile::getLocaOffset() const { return locaOffset; }
uint32_t TTFFile::getMaxpOffset() const { return maxpOffset; }


TTFFile TTFFile::parse(const std::vector<char>& data) {
    // Parse header + table directory
    TTFHeader header = TTFHeader::parse(data);
    header.parseTables(data);
    std::vector<TTFTable*> tables = TTFTable::parseTableDirectory(data, header.getNumTables());
    TableMap tableMap = header.getTables();

    // (Optional) tag dump
    // for (const auto& [tag, ptr] : tableMap) {
    //     std::cout << "TAG: " << tag << std::endl;
    // }

    // Convenience accessor for required tables (will throw if missing)
    // Grab required tables (still fine to use at() here if you want hard failures)
    TTFTable& headTbl = *tableMap.at("head");
    TTFTable& maxpTbl = *tableMap.at("maxp");
    TTFTable& locaTbl = *tableMap.at("loca");
    TTFTable& glyfTbl = *tableMap.at("glyf");
    TTFTable& cmapTbl = *tableMap.at("cmap");
    TTFTable& hheaTbl = *tableMap.at("hhea");
    TTFTable& nameTbl = *tableMap.at("name");
    TTFTable& postTbl = *tableMap.at("post");
    TTFTable& hmtxTbl = *tableMap.at("hmtx");

    // Optional tables: prefer GPOS; fall back to kern
    TTFTable* gposTbl = nullptr;
    TTFTable* kernTbl = nullptr;

    if (auto it = tableMap.find("GPOS"); it != tableMap.end()) gposTbl = it->second;
    if (auto it = tableMap.find("kern"); it != tableMap.end())  kernTbl = it->second;

    // Parse required tables...
    HeadTable headTable = HeadTable::parseHeadDirectory(data, headTbl.getOffset());
    MaxpTable maxpTable = MaxpTable::parseMaxpDirectory(data, maxpTbl.getOffset());
    HheaTable hheaTable = HheaTable::parseHheaDirectory(data, hheaTbl.getOffset());
    NameTable nameTable = NameTable::parseNameDirectory(data, nameTbl.getOffset());
    PostTable postTable = PostTable::parsePostDirectory(data, postTbl.getOffset());
    HmtxTable hmtxTable = HmtxTable::parseHmtxDirectory(
        data, hmtxTbl.getOffset(), hheaTable.getNumOfLongHorMetrics(), maxpTable.getNumGlyphs());

    // Optional shaping/kerning
    if (gposTbl) {
        GposTable::parseGposDirectory(data, gposTbl->getOffset());
    } else if (kernTbl) {
        KernTable::parseKernDirectory(data, kernTbl->getOffset());
    }
    // else: neither present — that’s valid in some fonts

    // indexToLocFormat: 0=short(half offsets), 1=long(byte offsets)
    const bool isLongLoca = (headTable.getIndexToLocFormat() != 0);

    // Parse loca and normalize to BYTE OFFSETS (done inside LocaTable::parse)
    LocaTable locaTable = LocaTable::parse(
        isLongLoca,
        data,
        locaTbl.getOffset(),
        maxpTable.getNumGlyphs()
    );

    // Get normalized byte offsets (locas.size() == numGlyphs + 1)
    const std::vector<uint32_t>& locasRef = locaTable.getOffsets32();
    std::vector<uint32_t> locas(locasRef.begin(), locasRef.end()); // own a copy if TTFFile stores by value

    // --- Sanity checks on loca ---
    if (locas.size() != static_cast<size_t>(maxpTable.getNumGlyphs()) + 1) {
        throw std::runtime_error("loca size mismatch (expected numGlyphs + 1).");
    }
    for (size_t i = 1; i < locas.size(); ++i) {
        if (locas[i] < locas[i - 1]) {
            throw std::runtime_error("loca entries must be non-decreasing.");
        }
    }
    // (Optional) If your TTFTable exposes length, you can ensure last offset <= glyf length.
    // Example (pseudo): if (locas.back() > glyfTbl.getLength()) throw ...

    // Parse cmap
    CmapTable cmapTable = CmapTable::parse(data, cmapTbl.getOffset());

    // Return assembled file object
    return TTFFile(
        header,
        tables,
        locas,          // normalized byte offsets
        locaTable,
        headTable,
        cmapTable,
        maxpTable,
        hheaTable,
        hmtxTable,
        /* cmap  */ cmapTbl.getOffset(),
        /* glyf  */ glyfTbl.getOffset(),
        /* head  */ headTbl.getOffset(),
        /* loca  */ locaTbl.getOffset(),
        /* maxp  */ maxpTbl.getOffset()
    );
}


Glyph TTFFile::parseGlyph(const std::vector<char>& data, uint32_t unicode) {
    if (unicode == 32) {
        return Glyph{0, 0, 0, 0, 0, {}, 0, {}, {}, {}, {}, {}};
    }

    uint16_t glyphIndex = cmapTable.getGlyphIndex(unicode);    

    if (glyphIndex >= locas.size()) {
        std::cerr << "Invalid glyph index: " << glyphIndex << std::endl;
        throw std::out_of_range("Glyph index out of range");
    }

    uint32_t locaOffset = locas[glyphIndex];

    if (locaOffset >= data.size()) {
        std::cerr << "Invalid loca offset: " << locaOffset << std::endl;
        throw std::out_of_range("Loca offset out of range");
    }

    uint32_t glyphOffset = glyfOffset + locaOffset;

    if (glyphOffset >= data.size()) {
        std::cerr << "Invalid glyph offset: " << glyphOffset << std::endl;
        throw std::out_of_range("Glyph offset out of range");
    }
    Glyph parsedGlyph = Glyph::parseGlyph(data, glyphOffset);
    parsedGlyph.addPointsBetween();
    parsedGlyph.gid = glyphIndex;
    return parsedGlyph;
}

std::vector<Glyph> TTFFile::parseGlyphs(const std::vector<char>& data, std::string letters) {
    std::vector<uint32_t> chars = stringToUnicode(letters);
    std::vector<Glyph> glyphs;
    glyphs.reserve(chars.size());
    for (uint32_t charCode : chars) {
        glyphs.push_back(parseGlyph(data, charCode));
    }
    return glyphs;
}
