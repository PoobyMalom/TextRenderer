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

    

    file.close();

    return 0;
    
    file.close();
}


