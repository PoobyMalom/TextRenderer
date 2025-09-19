#pragma once
#include "TTFTable.h"
#include "SDL2/SDL.h"
#include "MovableLine.h"
#include "Helpers.h"
#include <vector>
#include <map>

class Glyph {
public:
    Glyph(
        int16_t numberOfContours,
        int16_t xMin,
        int16_t yMin,
        int16_t xMax,
        int16_t yMax,
        std::vector<uint16_t> endPtsOfContours,
        uint16_t instructionLength,
        std::vector<uint8_t> instructions,
        std::vector<uint8_t> flags,
        std::vector<int16_t> xCoordinates,
        std::vector<int16_t> yCoordinates
    );

    int16_t getNumberOfContours() const;
    int16_t getXMin() const;
    int16_t getYMin() const;
    int16_t getXMax() const;
    int16_t getYMax() const;
    std::vector<uint16_t> getEndPtsOfContours() const;
    uint16_t getInstructionLength() const;
    std::vector<uint8_t> getInstructions() const;
    std::vector<uint8_t> getFlags() const;
    std::vector<int16_t> getXCoordinates() const;
    std::vector<int16_t> getYCoordinates() const;

    static Glyph parseGlyph(const std::vector<char>& data, uint32_t glyphOffset);
    static Glyph parseSimpleGlyph(const std::vector<char>& data, uint32_t pos, int16_t numberOfContours, int16_t xMin, int16_t yMin, int16_t xMax, int16_t yMax);
    static Glyph parseCompoundGlyph(const std::vector<char>& data, uint32_t pos, int16_t xMin, int16_t yMin, int16_t xMax, int16_t yMax);
    void addPointsBetween();
    static void drawSimpleGlyph(SDL_Renderer* renderer, Glyph glyph, int xOffset, int yOffset, double scalingFactor, int screenHeight, int thickness);
    void printGlyph(bool printEndPoints = true, bool printInstructions = true, bool printCoords = true);

private:
    int16_t numberOfContours;
    int16_t xMin;
    int16_t yMin;
    int16_t xMax;
    int16_t yMax;
    std::vector<uint16_t> endPtsOfContours;
    uint16_t instructionLength;
    std::vector<uint8_t> instructions;
    std::vector<uint8_t> flags;
    std::vector<int16_t> xCoordinates;
    std::vector<int16_t> yCoordinates;
};

struct GlyphName {
    uint16_t index;
    std::string name;
    
    GlyphName(uint16_t idx, const std::string& n) : index(idx), name(n) {}
};

std::vector<GlyphName> readAdobeGlyphList(const std::string& filename);
void loadStandardGlyphNamesMap(const std::string& filename, std::map<uint16_t, string>& standardGlyphMap);
string getStandardGlyphNameFast(uint16_t index, map<uint16_t, string>& standardGlyphMap);