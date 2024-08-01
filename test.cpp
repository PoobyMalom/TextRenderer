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
#include "GlyphTable.h"
using namespace std;

//thanks chatGPT
void drawCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius) {
    for (int w = -radius; w <= radius; w++) {
        for (int h = -radius; h <= radius; h++) {
            if (w*w + h*h <= radius*radius) {
                SDL_RenderDrawPoint(renderer, centerX + w, centerY + h);
            }
        }
    }
}

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

    /*for (uint16_t loca : locas) {
        cout << "loca: " << loca << endl;
    }*/

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

    cout << "glyphIndex: " << glyphIndex << endl;
    cout << "B glyf offset: " << locas[glyphIndex] << endl;

    Glyph glyph = Glyph::parseGlyph(buffer, glyfOffset + locas[glyphIndex]);

    cout << "number of contours: " << glyph.getNumberOfContours() << endl;
    cout << "xMin: " << dec << glyph.getXMin() << endl;
    cout << "yMin: " << glyph.getYMin() << endl;
    cout << "xMax: " << glyph.getXMax() << endl;
    cout << "yMax: " << glyph.getYMax() << endl;

    vector<uint8_t> flags = glyph.getFlags();
    for (uint8_t flag : flags) {
        bitset<8> bits(flag);
        cout << bits << endl;
    }
    cout << "length of flags: " << flags.size() << endl;

    vector<uint16_t> xCoordinates = glyph.getXCoordinates();
    vector<uint16_t> yCoordinates = glyph.getYCoordinates();
    for (int i = 0; i < flags.size(); ++i) {
        cout << "x: " << xCoordinates[i] << " | y: " << yCoordinates[i] << endl;
    }

    /*
    uint16_t numberOfContours;
    file.read(reinterpret_cast<char*>(&numberOfContours), sizeof(uint16_t));
    numberOfContours = convertEndian16(numberOfContours);
    cout << "numberOfContours: " << numberOfContours << endl;

    int16_t xMin;
    int16_t yMin;
    int16_t xMax;
    int16_t yMax;

    file.read(reinterpret_cast<char*>(&xMin), sizeof(int16_t));
    file.read(reinterpret_cast<char*>(&yMin), sizeof(int16_t));
    file.read(reinterpret_cast<char*>(&xMax), sizeof(int16_t));
    file.read(reinterpret_cast<char*>(&yMax), sizeof(int16_t));

    xMin = convertEndian16(xMin);
    yMin = convertEndian16(yMin);
    xMax = convertEndian16(xMax);
    yMax = convertEndian16(yMax);

    cout << "xMin: " << dec << xMin << endl;
    cout << "yMin: " << yMin << endl;
    cout << "xMax: " << xMax << endl;
    cout << "yMax: " << yMax << endl;
    */

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        cerr << "SDL initialization failed: " << SDL_GetError() << endl;
        return 1;
    }

    // Create SDL window and renderer
    const int winWidth = 1000;
    const int winHeight = 1000;
    SDL_Window *win = SDL_CreateWindow("SDL Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winWidth, winHeight, 0);
    if (win == nullptr) {
        cerr << "Failed to create SDL window: " << SDL_GetError() << endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == nullptr) {
        cerr << "Failed to create SDL renderer: " << SDL_GetError() << endl;
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    bool quit = false;

    

    SDL_Event e;
    while (!quit) {
        // Handle events on the queue
        while (SDL_PollEvent(&e) != 0) {
            // User requests quit
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);

        SDL_RenderClear(ren);
        // Render a red filled quad

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);

        for (int i = 0; i < xCoordinates.size(); ++i) {
            int radius = 4;
            //cout << "Drawing circle at (" << xCoordinates[i] << ", " << yCoordinates[i] << ") with radius " << radius << endl;
            drawCircle(ren, xCoordinates[i] + 10, yCoordinates[i] + 10, radius);
        }

        // Update screen
        SDL_RenderPresent(ren);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);

    // Quit SDL subsystems
    SDL_Quit();

    
    file.close();

    return 0;
}