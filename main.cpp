/*#include <SDL.h>
#include <iostream>
#include <vector>
#include <fstream>
#include "MovablePoint.h"
#include "Helpers.h"

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("Draggable Point", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
    if (win == nullptr) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == nullptr) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    MovablePoint point1(50, 50);
    MovablePoint point2(100, 100);
    MovablePoint controlPoint(200, 200);

    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else {
                point1.handleEvent(e);
                point2.handleEvent(e);
                controlPoint.handleEvent(e);
            }
        }

        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        SDL_RenderClear(ren);

        // Render the point
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_Rect rect1 = { (int)point1.getX(), (int)point1.getY(), 10, 10 };
        SDL_Rect rect2 = { (int)point2.getX(), (int)point2.getY(), 10, 10 };
        SDL_Rect controlRect = { (int)controlPoint.getX(), (int)controlPoint.getY(), 10, 10 };

        DrawLine(ren, point1, controlPoint);
        DrawLine(ren, controlPoint, point2);

        //MovingPoint(ren, point1, controlPoint, t);
        //MovingPoint(ren, controlPoint, point2, t);

        DrawBezier(ren, point1, controlPoint, point2);

        SDL_RenderFillRect(ren, &rect1);
        SDL_RenderFillRect(ren, &rect2);
        SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
        SDL_RenderFillRect(ren, &controlRect);

        SDL_RenderPresent(ren);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}*/


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
#include "Helpers.h"
#include "TTFHeader.h"
#include "TTFTable.h"

using namespace std;

// Function to convert hex string to ASCII
string hexToAscii(uint32_t value) {
    stringstream ss;
    ss << hex << uppercase << setw(8) << setfill('0') << value;
    string hexString = ss.str();
    string ascii;
    for (size_t i = 0; i < hexString.length(); i += 2) {
        // Convert each pair of hex digits to an integer
        string byteString = hexString.substr(i, 2);
        char byte = static_cast<char>(stoi(byteString, nullptr, 16));
        ascii.push_back(byte);
    }
    return ascii;
}

// Function to convert uint32_t to hex string

// Function to convert endian of 32-bit value
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

uint32_t findUnicodevector(vector<uint32_t> startCharCodes, vector<uint32_t> endCharCodes, vector<uint32_t> startGlyphCodes, uint16_t value) {
    for (int i = 0; i < startCharCodes.size(); ++i) {
        if (value >= startCharCodes[i] && value <= endCharCodes[i]) {
            cout << "startCharCode: " << startCharCodes[i] << " | endCharCode: " << endCharCodes[i] << " | startGlyphCode: " << startGlyphCodes[i] << endl;
            return startGlyphCodes[i] + (value - startCharCodes[i]);
        }
    }
}

// Function to read TTF file (implementation assumed)


int main() {
    // Open the file in binary mode
    ifstream file("src/fonts/JetBrainsMono-Bold.ttf", ios::binary);
    file.seekg(0, ios::end);
    cout << "File size: " << file.tellg() << " bytes" << endl;
    file.seekg(0, ios::beg);

    if (sizeof(file) < sizeof(uint32_t)) {
        cerr << "File is too small to read a uint32_t value." << endl;
        return 1;
    }

    // Read the first 4 bytes as uint32_t
    uint32_t scalar_type;
    file.read(reinterpret_cast<char*>(&scalar_type), sizeof(uint32_t));

    // Convert endian if necessary (depends on TTF format and system endianness)
    uint32_t scalar_type_swapped = convertEndian32(scalar_type);

    // Convert to hexadecimal string
    string scalar_type_ascii = hexToAscii(scalar_type_swapped);

    // Convert hexadecimal string to ASCII
    // Note: Make sure hexToAscii is used correctly for specific needs

    // Print results
    bitset<32> binary(scalar_type);
    //cout << "Binary representation: " << binary << endl;
    //cout << "The value in hexadecimal is: 0x" << scalar_type_hex << endl;

    file.seekg(4, ios::beg);

    uint16_t numTables;
    file.read(reinterpret_cast<char*>(&numTables), sizeof(uint16_t));

    uint16_t numTables_swapped = convertEndian16(numTables);
    uint32_t tags[numTables_swapped];
    uint32_t checksums[numTables_swapped];
    uint32_t offsets[numTables_swapped];
    uint32_t lengths[numTables_swapped];
    cout << "Number of tables: " << numTables_swapped << endl;

    file.seekg(6, ios::cur);

    for (int i = 0; i < numTables_swapped; i++) {
        uint32_t tag;
        uint32_t checksum;
        uint32_t offset;
        uint32_t length;
        char buffer[16];
        file.read(buffer, sizeof(buffer));
        memcpy(&tag, buffer, sizeof(tag));
        memcpy(&checksum, buffer + 4, sizeof(checksum));
        memcpy(&offset, buffer + 8, sizeof(offset));
        memcpy(&length, buffer + 12, sizeof(length));

        // Convert endian if necessary (depends on TTF format and system endianness)
        tag = convertEndian32(tag);
        checksum = convertEndian32(checksum);
        offset = convertEndian32(offset);
        length = convertEndian32(length);

        string tag_ascii = hexToAscii(tag);

        streampos current_position;
        current_position = file.tellg();

        // Print results
        cout << dec;
        cout << "Table " << i + 1 << ":" << endl;
        cout << "-------------------------------" << endl;
        cout << "Tag: " << hex << tag_ascii << endl;
        cout << "Checksum: 0x" << hex << checksum << endl;
        cout << "Offset: 0x" << hex << offset << endl;
        cout << dec;
        cout << "Length: " << length << endl;
        cout << "-------------------------------" << endl;
        cout << "Current Byte: " << current_position << endl;

        tags[i] = tag;
        checksums[i] = checksum;
        offsets[i] = offset;
        lengths[i] = length;

    }

    

    uint32_t glyf = 0x8a6c;
    file.seekg(glyf, ios::beg);

    streampos current_position;
    current_position = file.tellg();
    cout << "Current Byte: " << current_position << endl;

    uint16_t num_contours;
    uint16_t xMin;
    uint16_t yMin;
    uint16_t xMax;
    uint16_t yMax;
    
    file.read(reinterpret_cast<char*>(&num_contours), sizeof(uint16_t));
    file.read(reinterpret_cast<char*>(&xMin), sizeof(uint16_t));
    file.read(reinterpret_cast<char*>(&yMin), sizeof(uint16_t));
    file.read(reinterpret_cast<char*>(&xMax), sizeof(uint16_t));
    file.read(reinterpret_cast<char*>(&yMax), sizeof(uint16_t));

    uint16_t num_contours_endian = convertEndian16(num_contours);
    uint16_t xMin_endian = convertEndian16(xMin);
    uint16_t yMin_endian = convertEndian16(yMin);
    uint16_t xMax_endian = convertEndian16(xMax);
    uint16_t yMax_endian = convertEndian16(yMax);

    cout << "Number of contours: " << num_contours_endian << endl;
    cout << "xMin: " << xMin_endian << endl;
    cout << "yMin: " << yMin_endian << endl;
    cout << "xMax: " << xMax_endian << endl;
    cout << "yMax: " << yMax_endian << endl;

    uint16_t endpoints[num_contours_endian];
    file.read(reinterpret_cast<char*>(endpoints), num_contours_endian * sizeof(uint16_t));
    uint16_t endpoints_swapped[num_contours_endian];
    for (int i = 0; i < num_contours_endian; i++) {
        endpoints_swapped[i] = convertEndian16(endpoints[i]);
    }

    for (int i = 0; i < num_contours_endian; i++) {
        cout << "End point " << i << ": " << endpoints_swapped[i] << endl;
    }

    uint16_t instruction_length;
    file.read(reinterpret_cast<char*>(&instruction_length), sizeof(uint16_t));
    uint16_t instruction_length_swapped = convertEndian16(instruction_length);
    cout << "Instruction length: " << instruction_length_swapped << endl;

    uint8_t instructions[instruction_length_swapped];
    file.read(reinterpret_cast<char*>(instructions), instruction_length_swapped * sizeof(uint8_t));
    for (int i = 0; i < instruction_length_swapped; i++) {
        cout << "Instruction " << i << ": " << static_cast<int>(instructions[i]) << endl;
    }

    uint8_t flags[instruction_length_swapped];
    file.read(reinterpret_cast<char*>(flags), instruction_length_swapped * sizeof(uint8_t));
    for (int i = 0; i < instruction_length_swapped; i++) {
        cout << "Flag " << i << ": " << bitset<8>(flags[i]) << endl;
    }

    //---------------------------------------------------------------------------------

    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    TTFHeader header = TTFHeader::parse(buffer);
    std::cout << "Scaler Type: " << header.getScalarType() << std::endl;
    std::cout << "Number of Tables: " << header.getNumTables() << std::endl;
    std::cout << "Search Range: " << header.getSearchRange() << std::endl;
    std::cout << "Entry Selector: " << header.getEntrySelector() << std::endl;
    std::cout << "Range Shift: " << header.getRangeShift() << std::endl;

    file.seekg(0, ios::beg);

    file.read(buffer.data(), fileSize);
    vector<TTFTable*> tables = TTFTable::parseTableDirectory(buffer, header.getNumTables());
    
    std::cout << "Number of tables: " << tables.size() << std::endl;    

    for (uint16_t i = 0; i < tables.size(); ++i) {
        TTFTable* table = tables[i];
        cout << "Tag: " << table->getTag() << endl;
        cout << "Checksum: " << table->getChecksum() << endl;
        cout << "Offset: " << table->getOffset() << endl;
        cout << "Length: " << table->getLength() << endl;
    }


    uint16_t cmap_offset = 0x1d34;
    uint16_t cmap_0_offset = 0x400;
    file.seekg(cmap_offset, ios::beg);

    current_position = file.tellg();
    cout << "Current Byte: " << current_position << endl;

    uint16_t format;
    file.read(reinterpret_cast<char*>(&format), sizeof(uint16_t));
    format = convertEndian16(format);
    cout << "format: " << format << endl;
    uint16_t length;
    file.read(reinterpret_cast<char*>(&length), sizeof(uint16_t));
    length = convertEndian16(length);
    cout << "length: " << length << endl;
    for (int i = 0; i < 4; ++i) {
        cout << "-----------------------------" << endl;
        uint16_t platformId;
        file.read(reinterpret_cast<char*>(&platformId), sizeof(uint16_t));
        platformId = convertEndian16(platformId);
        cout << "platform id: " << platformId << endl;

        uint16_t platformSpecificId;
        file.read(reinterpret_cast<char*>(&platformSpecificId), sizeof(uint16_t));
        platformSpecificId = convertEndian16(platformSpecificId);
        cout << "platform specific id: " << platformSpecificId << endl;

        uint32_t offset;
        file.read(reinterpret_cast<char*>(&offset), sizeof(uint32_t));
        offset = convertEndian32(offset);
        cout << "offset: " << offset << endl;
    }

    file.read(reinterpret_cast<char*>(&format), sizeof(uint16_t));
    format = convertEndian16(format);
    cout << "format: " << format << endl;
    file.read(reinterpret_cast<char*>(&length), sizeof(uint16_t));
    length = convertEndian16(length);
    cout << "length: " << length << endl;

    file.seekg(length-4, ios::cur);

    file.read(reinterpret_cast<char*>(&format), sizeof(uint16_t));
    format = convertEndian16(format);
    cout << "format: " << format << endl;
    file.seekg(2, ios::cur);
    uint32_t length32;
    file.read(reinterpret_cast<char*>(&length32), sizeof(uint32_t));
    length32 = convertEndian32(length32);
    cout << "length: " << length32 << endl;
    file.seekg(4, ios::cur);
    uint32_t nGroups;
    file.read(reinterpret_cast<char*>(&nGroups), sizeof(uint32_t));
    nGroups = convertEndian32(nGroups);
    cout << "n groups: " << nGroups << endl;

    vector<uint32_t> startCharCodes;
    vector<uint32_t> endCharCodes;
    vector<uint32_t> startGlyphCodes;

    for (int i = 0; i < nGroups; ++i) {
        uint32_t startCharCode;
        file.read(reinterpret_cast<char*>(&startCharCode), sizeof(uint32_t));
        startCharCode = convertEndian32(startCharCode);
        //cout << "startCodeChar: " << startCharCode << endl;

        uint32_t endCharCode;
        file.read(reinterpret_cast<char*>(&endCharCode), sizeof(uint32_t));
        endCharCode = convertEndian32(endCharCode);
        //cout << "endCodeChar: " << endCharCode << endl;

        uint32_t startGlyphCode;
        file.read(reinterpret_cast<char*>(&startGlyphCode), sizeof(uint32_t));
        startGlyphCode = convertEndian32(startGlyphCode);
        //cout << "startGlyphCode: " << startGlyphCode << endl;

        startCharCodes.push_back(startCharCode);
        endCharCodes.push_back(endCharCode);
        startGlyphCodes.push_back(startGlyphCode);
    }

    uint32_t idontknow;
    idontknow = findUnicodevector(startCharCodes, endCharCodes, startGlyphCodes, 0x42);
    cout << "please work: " << idontknow << endl;

    uint32_t headOffset = 0x11c;
    uint32_t headskip = 0x32;
    /*
    Bytes offsets:
    Fixed version = 4 bytes
    Fixed fontRevision = 4 bytes
    uint32 checkSumAdjustment = 4 bytes
    uint32 magicNumber = 4 bytes
    uint16 flags = 2 bytes
    uint16 unitsPerEm = 2 bytes
    longDateTime created = 8 bytes
    longDateTime modified = 8 bytes
    Fword xMin = 2 bytes
    Fword yMin = 2 bytes
    Fword xMax = 2 bytes
    Fword yMax = 2 bytes
    uint16 macStyle = 2 bytes
    uint16 lowerRecPPEM = 2 bytes
    int16 fontDirectionHint = 2 bytes
    total 52 bytes
    */
    file.seekg(headOffset+headskip, ios::beg);
    int16_t indexToLocFormat;
    file.read(reinterpret_cast<char*>(&indexToLocFormat), sizeof(int16_t));
    indexToLocFormat = convertEndian16(indexToLocFormat);
    cout << "indexToLocFormat: " << indexToLocFormat << endl;

    uint32_t maxpOffset = 0x17c;
    file.seekg(maxpOffset, ios::beg);
    uint16_t numGlyphs;
    file.read(reinterpret_cast<char*>(&numGlyphs), sizeof(uint16_t));
    numGlyphs = convertEndian16(numGlyphs);
    cout << "numGlyphs: " << numGlyphs << endl;

    uint32_t locaOffset = 0x6f2c;
    file.seekg(locaOffset, ios::beg);

    uint32_t locaOffsets[numGlyphs];
    for (int i = 0; i < numGlyphs + 1; ++i) {
        uint32_t locaGlyphOffset;
        file.read(reinterpret_cast<char*>(&locaGlyphOffset), sizeof(uint32_t));
        locaGlyphOffset = convertEndian32(locaGlyphOffset);
        //cout << hex << locaGlyphOffset << endl;
        locaOffsets[i] = locaGlyphOffset;
    }
    cout << "loca offset of B: " << hex << locaOffsets[idontknow] << endl;

    cout << glyf << endl;
    file.seekg(glyf, ios::beg);
    file.seekg(locaOffsets[idontknow], ios::cur);
    cout << file.tellg() << endl;
    cout << "locaOffsets[0]: " << locaOffsets[0];

    uint16_t numberOfContours;
    file.read(reinterpret_cast<char*>(&numberOfContours), sizeof(uint16_t));
    numberOfContours = convertEndian16(numberOfContours);
    cout << "numberOfContours: " << numberOfContours << endl;
    uint16_t xMin_here;
    file.read(reinterpret_cast<char*>(&xMin_here), sizeof(uint16_t));
    xMin_here = convertEndian16(xMin_here);
    cout << "xMin: " << dec << xMin_here << endl;


/*
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

    uint8_t xCoordinates[instruction_length_swapped];
        uint8_t yCoordinates[instruction_length_swapped];
        file.read(reinterpret_cast<char*>(xCoordinates), instruction_length_swapped * sizeof(uint8_t));
        file.read(reinterpret_cast<char*>(yCoordinates), instruction_length_swapped * sizeof(uint8_t));

    SDL_Event e;
    while (!quit) {
        // Handle events on the queue
        while (SDL_PollEvent(&e) != 0) {
            // User requests quit
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        SDL_SetRenderDrawColor(ren, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(ren);
        SDL_SetRenderDrawColor(ren, 0xFF, 0x00, 0x00, 0xFF);
        // Render a red filled quad
        for (int i = 0; i < instruction_length_swapped; i++) {
            SDL_RenderDrawPoint(ren, xCoordinates[i], yCoordinates[i]);
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
    */
    file.close();
}


