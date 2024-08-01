#include <iostream>
#include <SDL.h>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <bitset>
#include <cstring> // For memcpy

#include "TTFTable.h"
#include "TTFHeader.h"
#include "HeadTable.h"
#include "MaxpTable.h"
#include "CmapTable.h"
#include "LocaTable.h"
using namespace std;


uint32_t convertEndian32(uint32_t value) {
    return ((value >> 24) & 0x000000FF) |
           ((value >> 8)  & 0x0000FF00) |
           ((value << 8)  & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
}

uint16_t convertEndian16(uint16_t value) {
    return ((value >> 8) & 0x00FF) |
           ((value << 8)  & 0xFF00);
}

int main() {
    // Open the file in binary mode
    ifstream file("src/fonts/JetBrainsMono-Bold.ttf", ios::binary);
    file.seekg(0, ios::end);
    streampos fileSize = file.tellg();
    cout << "File size: " << fileSize << " bytes" << endl;
    file.seekg(0, ios::beg);

    if (sizeof(file) < sizeof(uint32_t)) {
        cerr << "File is too small to read a uint32_t value." << endl;
        return 1;
    }

    vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    TTFHeader header = TTFHeader::parse(buffer);

    uint16_t numTables = header.getNumTables();

    cout << "Number of Tables: " << numTables << endl;

    file.seekg(0, ios::beg);
    
    vector<TTFTable*> tables = TTFTable::parseTableDirectory(buffer, numTables);

    uint16_t cmapOffset;
    uint16_t glyfOffset;
    uint16_t headOffset;
    uint16_t locaOffset;
    uint16_t maxpOffset;

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

    cout << "---------------------------------" << endl;
    cout << "cmap offset: " << cmapOffset << endl;
    cout << "glyf offset: " << glyfOffset << endl;
    cout << "head offset: " << headOffset << endl;
    cout << "loca offset: " << locaOffset << endl;
    cout << "maxp offset: " << maxpOffset << endl;
    cout << "---------------------------------" << endl;

    HeadTable headTable = HeadTable::parseHeadDirectory(buffer, headOffset);
    bool indexToLocFormat = static_cast<bool>(headTable.getIndexToLocFormat());
    cout << "indexToLocFormat: " << indexToLocFormat << endl;

    MaxpTable maxpTable = MaxpTable::parseMaxpDirectory(buffer, maxpOffset);
    uint16_t numGlyphs = maxpTable.getNumGlyphs();
    cout << "Number of Glyphs: " << numGlyphs << endl; 

    LocaTable locaTable = LocaTable::parse(indexToLocFormat, buffer, locaOffset, numGlyphs);
    
    vector<uint32_t> locas;

    if (indexToLocFormat) {
        const vector<uint32_t>& locas32 = locaTable.getOffsets32();
        for (uint32_t loca : locas32) {
            locas.push_back(loca);
        }
    }
    else {
        const vector<uint16_t>& locas16 = locaTable.getOffsets16();
        for (uint16_t loca : locas16) {
            locas.push_back(loca);
        }
    }

    cout << "Loca Table Offset 0: " << locas[0] << endl;
    cout << "Loca Table Offset 1: " << locas[1] << endl;
    cout << "Loca Table Offset 2: " << locas[2] << endl;
    cout << "Loca Table Offset 3: " << locas[3] << endl;
    cout << "Loca Table Offset 4: " << locas[4] << endl;
    cout << "Loca Table Offset 5: " << locas[5] << endl;

    CmapTable cmapTable = CmapTable::parse(buffer, cmapOffset);

    char letter = 'B';
    uint32_t unicodeValue = static_cast<uint32_t>(letter);
    cout << "unicodeValue: 0x" << hex << unicodeValue << endl; 
    uint16_t platformID = 0;
    uint16_t encodingID = 4;
    uint16_t glyphIndex = cmapTable.getGlyphIndex(unicodeValue, platformID, encodingID);

    cout << "Glyph index for 'B': " << glyphIndex << endl;

    file.seekg(glyfOffset, ios::beg);
    file.seekg(locas[glyphIndex], ios::cur);

}