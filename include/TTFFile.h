#pragma once
#include <vector>
#include <string>
#include <map>
#include "TTFTable.h"
#include "TTFHeader.h"
#include "GlyphTable.h"

class TTFFile {
public:
    TTFFile(const std::string& filePath);
    void parse();
    TTFHeader getHeader() const;
    TTFTable* getTable(const std::string& tag) const;
    GlyphTable* getGlyphTable() const;

private:
    std::string filePath;
    TTFHeader header;
    std::map<std::string, TTFTable*> tables;
    GlyphTable* glyphTable;

    void parseHeader();
    void parseTables();
};