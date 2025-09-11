#include <iostream>
#include <SDL2/SDL.h>
#include <fstream>
#include <istream>
#include <vector>
#include <string>
#include <cstdint>
#include <tuple>
#include <bitset>
#include <cstdint>
#include <array>
#include <cstring> // For memcpy
#include "include/Helpers.h"
#include "include/TTFHeader.h"
#include "include/TTFTable.h"
#include "include/CmapTable.h"
#include "include/TTFFile.h"

using namespace std;

int main() {
    // Open the file in binary mode
    ifstream file("src/fonts/JetBrainsMono-Bold.ttf", ios::binary);
    //file.seekg(0, ios::end);
    //cout << "File size: " << file.tellg() << " bytes" << endl;

    // Start reading at the beginning of the file
    file.seekg(0, ios::beg);

    // Check if the file is too small to possibly be read
    if (sizeof(file) < sizeof(uint32_t)) {
        cerr << "File is too small to read a uint32_t value." << endl;
        return 1;
    }

    // set a streampos object to keep track of current reading position
    streampos current_position;
    current_position = file.tellg();

    //---------------------------------------------------------------------------------

    // Read the size of the file
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Create a buffer of the data in the file
    std::vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);

    // Parse the header subtable
    TTFHeader header = TTFHeader::parse(buffer);
    header.parseTables(buffer);

    // Create a map of all tables in the ttf file (tag -> table)
    TableMap tableMap = header.getTables();

    // Return to the beginning of the file
    //file.seekg(0, ios::beg);

    uint16_t cmap_offset = tableMap.at("cmap")->getOffset();
    file.seekg(cmap_offset, ios::beg);

    // std::vector<uint8_t> tableData(cmap->getLength());
    // file.read(reinterpret_cast<char*>(tableData.data()), cmap->getLength());

    CmapTable cmapTable = CmapTable::parse(buffer, reinterpret_cast<uint16_t>(cmap_offset));

    uint32_t character = stringToUnicode("a")[0];

    TTFFile ttfData = TTFFile::parse(buffer);
    Glyph glyph = ttfData.parseGlyph(buffer, character);
    glyph.printGlyph();
    // uint16_t format;
    // file.read(reinterpret_cast<char*>(&format), sizeof(uint16_t));
    // format = convertEndian16(format);
    // cout << "format: " << format << endl;
    // uint16_t length;
    // file.read(reinterpret_cast<char*>(&length), sizeof(uint16_t));
    // length = convertEndian16(length);
    // cout << "length: " << length << endl;
    // for (int i = 0; i < 4; ++i) {
    //     cout << "-----------------------------" << endl;
    //     uint16_t platformId;
    //     file.read(reinterpret_cast<char*>(&platformId), sizeof(uint16_t));
    //     platformId = convertEndian16(platformId);
    //     cout << "platform id: " << platformId << endl;

    //     uint16_t platformSpecificId;
    //     file.read(reinterpret_cast<char*>(&platformSpecificId), sizeof(uint16_t));
    //     platformSpecificId = convertEndian16(platformSpecificId);
    //     cout << "platform specific id: " << platformSpecificId << endl;

    //     uint32_t offset;
    //     file.read(reinterpret_cast<char*>(&offset), sizeof(uint32_t));
    //     offset = convertEndian32(offset);
    //     cout << "offset: " << offset << endl;
    // }

    // file.read(reinterpret_cast<char*>(&format), sizeof(uint16_t));
    // format = convertEndian16(format);
    // cout << "format: " << format << endl;
    // file.read(reinterpret_cast<char*>(&length), sizeof(uint16_t));
    // length = convertEndian16(length);
    // cout << "length: " << length << endl;

    // file.seekg(length-4, ios::cur);

    // file.read(reinterpret_cast<char*>(&format), sizeof(uint16_t));
    // format = convertEndian16(format);
    // cout << "format: " << format << endl;
    // file.seekg(2, ios::cur);
    // uint32_t length32;
    // file.read(reinterpret_cast<char*>(&length32), sizeof(uint32_t));
    // length32 = convertEndian32(length32);
    // cout << "length: " << length32 << endl;
    // file.seekg(4, ios::cur);
    // uint32_t nGroups;
    // file.read(reinterpret_cast<char*>(&nGroups), sizeof(uint32_t));
    // nGroups = convertEndian32(nGroups);
    // cout << "n groups: " << nGroups << endl;

    // vector<uint32_t> startCharCodes;
    // vector<uint32_t> endCharCodes;
    // vector<uint32_t> startGlyphCodes;

    // for (int i = 0; i < static_cast<int>(nGroups); ++i) {
    //     uint32_t startCharCode;
    //     file.read(reinterpret_cast<char*>(&startCharCode), sizeof(uint32_t));
    //     startCharCode = convertEndian32(startCharCode);
    //     //cout << "startCodeChar: " << startCharCode << endl;

    //     uint32_t endCharCode;
    //     file.read(reinterpret_cast<char*>(&endCharCode), sizeof(uint32_t));
    //     endCharCode = convertEndian32(endCharCode);
    //     //cout << "endCodeChar: " << endCharCode << endl;

    //     uint32_t startGlyphCode;
    //     file.read(reinterpret_cast<char*>(&startGlyphCode), sizeof(uint32_t));
    //     startGlyphCode = convertEndian32(startGlyphCode);
    //     //cout << "startGlyphCode: " << startGlyphCode << endl;

    //     startCharCodes.push_back(startCharCode);
    //     endCharCodes.push_back(endCharCode);
    //     startGlyphCodes.push_back(startGlyphCode);
    // }

    // uint32_t idontknow;
    // idontknow = findUnicodevector(startCharCodes, endCharCodes, startGlyphCodes, 0x42);
    // cout << "please work: " << idontknow << endl;

    // uint32_t headOffset = 0x11c;
    // uint32_t headskip = 0x32;
    // /*
    // Bytes offsets:
    // Fixed version = 4 bytes
    // Fixed fontRevision = 4 bytes
    // uint32 checkSumAdjustment = 4 bytes
    // uint32 magicNumber = 4 bytes
    // uint16 flags = 2 bytes
    // uint16 unitsPerEm = 2 bytes
    // longDateTime created = 8 bytes
    // longDateTime modified = 8 bytes
    // Fword xMin = 2 bytes
    // Fword yMin = 2 bytes
    // Fword xMax = 2 bytes
    // Fword yMax = 2 bytes
    // uint16 macStyle = 2 bytes
    // uint16 lowerRecPPEM = 2 bytes
    // int16 fontDirectionHint = 2 bytes
    // total 52 bytes
    // */
    // file.seekg(headOffset+headskip, ios::beg);
    // int16_t indexToLocFormat;
    // file.read(reinterpret_cast<char*>(&indexToLocFormat), sizeof(int16_t));
    // indexToLocFormat = convertEndian16(indexToLocFormat);
    // cout << "indexToLocFormat: " << indexToLocFormat << endl;

    // uint32_t maxpOffset = 0x17c;
    // file.seekg(maxpOffset, ios::beg);
    // uint16_t numGlyphs;
    // file.read(reinterpret_cast<char*>(&numGlyphs), sizeof(uint16_t));
    // numGlyphs = convertEndian16(numGlyphs);
    // cout << "numGlyphs: " << numGlyphs << endl;

    // uint32_t locaOffset = 0x6f2c;
    // file.seekg(locaOffset, ios::beg);

    // uint32_t locaOffsets[numGlyphs];
    // for (int i = 0; i < numGlyphs + 1; ++i) {
    //     uint32_t locaGlyphOffset;
    //     file.read(reinterpret_cast<char*>(&locaGlyphOffset), sizeof(uint32_t));
    //     locaGlyphOffset = convertEndian32(locaGlyphOffset);
    //     //cout << hex << locaGlyphOffset << endl;
    //     locaOffsets[i] = locaGlyphOffset;
    // }
    // cout << "loca offset of B: " << hex << locaOffsets[idontknow] << endl;

    // uint16_t numberOfContours;
    // file.read(reinterpret_cast<char*>(&numberOfContours), sizeof(uint16_t));
    // numberOfContours = convertEndian16(numberOfContours);
    // cout << "numberOfContours: " << numberOfContours << endl;
    // uint16_t xMin_here;
    // file.read(reinterpret_cast<char*>(&xMin_here), sizeof(uint16_t));
    // xMin_here = convertEndian16(xMin_here);
    // cout << "xMin: " << dec << xMin_here << endl;
    
    file.close();

    return 0;
    
    file.close();
}


