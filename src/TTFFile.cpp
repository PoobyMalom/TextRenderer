#include "TTFFile.h"
#include "Helpers.h"
#include <iostream>

TTFFile::TTFFile(
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
) : header(header),
    tables(tables),
    locas(locas),
    headTable(headTable),
    cmapTable(cmapTable),
    maxpTable(maxpTable),
    cmapOffset(cmapOffset),
    glyfOffset(glyfOffset),
    headOffset(headOffset),
    locaOffset(locaOffset),
    maxpOffset(maxpOffset) {}

TTFHeader TTFFile::getHeader() const {
    return header;
}

std::vector<TTFTable*> TTFFile::getTables() const {
    return tables;
}

std::vector<uint32_t> TTFFile::getLocas() const {
    return locas;
}

HeadTable TTFFile::getHeadTable() const {
    return headTable;
}

CmapTable TTFFile::getCmapTable() const {
    return cmapTable;
}

MaxpTable TTFFile::getMaxpTable() const {
    return maxpTable;
}

uint16_t TTFFile::getCmapOffset() const {
    return cmapOffset;
}

uint16_t TTFFile::getGlyfOffset() const {
    return glyfOffset;
}

uint16_t TTFFile::getHeadOffset() const {
    return headOffset;
}

uint16_t TTFFile::getLocaOffset() const {
    return locaOffset;
}

uint16_t TTFFile::getMaxpOffset() const {
    return maxpOffset;
}

TTFFile TTFFile::parse(const std::vector<char>& data) {
    TTFHeader header = TTFHeader::parse(data);
    std::vector<TTFTable*> tables = TTFTable::parseTableDirectory(data, header.getNumTables());

    uint32_t cmapOffset = 0;
    uint32_t glyfOffset = 0;
    uint32_t headOffset = 0;
    uint32_t locaOffset = 0;
    uint32_t maxpOffset = 0;

    for (int i = 0; i < tables.size(); ++i) {
        TTFTable* tempTable = tables[i];
        if (tempTable->getTag() == "cmap") {
            cmapOffset = tempTable->getOffset();
        }
        if (tempTable->getTag() == "glyf") {
            glyfOffset = tempTable->getOffset();
        }
        if (tempTable->getTag() == "head") {
            headOffset = tempTable->getOffset();
        }
        if (tempTable->getTag() == "loca") {
            locaOffset = tempTable->getOffset();
        }
        if (tempTable->getTag() == "maxp") {
            maxpOffset = tempTable->getOffset();
        }
    }

    HeadTable headTable = HeadTable::parseHeadDirectory(data, headOffset);
    MaxpTable maxpTable = MaxpTable::parseMaxpDirectory(data, maxpOffset);
    bool indexToLocFormat = static_cast<bool>(headTable.getIndexToLocFormat());
    LocaTable locaTable = LocaTable::parse(
        indexToLocFormat, 
        data, 
        locaOffset, 
        maxpTable.getNumGlyphs()
    );

    std::vector<uint32_t> locas;

    if (indexToLocFormat) {
        const std::vector<uint32_t>& locas32 = locaTable.getOffsets32();
        locas.insert(locas.end(), locas32.begin(), locas32.end());
    } else {
        const std::vector<uint16_t>& locas16 = locaTable.getOffsets16();
        locas.insert(locas.end(), locas16.begin(), locas16.end());
    }

    CmapTable cmapTable = CmapTable::parse(data, cmapOffset);
    return TTFFile(header, tables, locas, headTable, cmapTable, maxpTable, cmapOffset, glyfOffset, headOffset, locaOffset, maxpOffset);
}

Glyph TTFFile::parseGlyph(const std::vector<char>& data, uint32_t unicode) {
    if (unicode == 32) {
        return Glyph{0, 0, 0, 0, 0, {}, 0, {}, {}, {}, {}};
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

    return Glyph::parseGlyph(data, glyphOffset);
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
