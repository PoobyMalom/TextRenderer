#include "TTFFile.h"
#include <vector>
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

    uint32_t cmapOffset;
    uint32_t glyfOffset;
    uint32_t headOffset;
    uint32_t locaOffset;
    uint32_t maxpOffset;

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
        for (uint32_t loca : locas32) {
            locas.push_back(loca);
        }
    }
    else {
        const std::vector<uint16_t>& locas16 = locaTable.getOffsets16();
        for (uint16_t loca : locas16) {
            locas.push_back(loca);
        }
    }

    CmapTable cmapTable = CmapTable::parse(data, cmapOffset);
    return TTFFile(header, tables, locas, headTable, cmapTable, maxpTable, cmapOffset, glyfOffset, headOffset, locaOffset, maxpOffset);
}

Glyph TTFFile::parseGlyph(const std::vector<char>& data, uint16_t platformID, uint16_t endcodingID, char letter) {
    uint32_t unicodeValue = static_cast<uint32_t>(letter);
    //std::cout << letter << " unicode: " << unicodeValue << std::endl;
    uint16_t glyphIndex = cmapTable.getGlyphIndex(unicodeValue, platformID, endcodingID);
    //std::cout << "glyph index: " << glyphIndex << std::endl;
    //std::cout << "loca index: " << locas[glyphIndex] << std::endl;
    return Glyph::parseGlyph(data, glyfOffset + locas[glyphIndex]);
}

std::vector<Glyph> TTFFile::parseGlyphs(const std::vector<char>& data, uint16_t platformID, uint16_t encodingID, std::string letters) {
    int numLetters = letters.size();
    std::vector<Glyph> glyphs;
    for (int i = 0; i < numLetters; ++i) {
        //std::cout << "glyph: " << letters[i] << std::endl;
        glyphs.push_back(parseGlyph(data, platformID, encodingID, letters[i]));
    }
    return glyphs;
}