#pragma once
#include "TTFTable.h"
#include "SDL2/SDL.h"
#include "MovableLine.h"
#include "Helpers.h"
#include <vector>
#include <map>

class GlyphPoint {

public:
    GlyphPoint(
        int16_t x,
        int16_t y,
        uint8_t flag
    );

private:
    int16_t x;
    int16_t y;
    uint8_t flag;
};

class Contour {

public:
    Contour(
        std::vector<GlyphPoint> points,
        int parent,
        std::vector<int> children,
        double signedArea
    );

private:
    std::vector<GlyphPoint> points; // Closed ring (first != last; treat as closed)
    int parent; // -1 = root (no parent)
    std::vector<int> children; // index of childern in contours vector
    double signedArea; // cache (shoelace), for orientation or size
};

class Glyph {

friend class TTFFile;

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
        std::vector<int16_t> yCoordinates,
        std::vector<Contour> contours
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
    uint16_t getGID() const;

    static Glyph parseGlyph(const std::vector<char>& data, uint32_t glyphOffset);
    static Glyph parseSimpleGlyph(const std::vector<char>& data, uint32_t pos, int16_t numberOfContours, int16_t xMin, int16_t yMin, int16_t xMax, int16_t yMax);
    static Glyph parseCompoundGlyph(const std::vector<char>& data, uint32_t pos, int16_t xMin, int16_t yMin, int16_t xMax, int16_t yMax);
    void addPointsBetween();
    static void drawSimpleGlyph(SDL_Renderer* renderer, Glyph glyph, int xOffset, int yOffset, double scalingFactor, int screenHeight, int thickness);
    void printGlyph(bool printEndPoints = true, bool printInstructions = true, bool printCoords = true);
    vector<Line> getGlyphSegments(int xOffset, int yOffset, double scalingFactor, int screenHeight);

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
    std::vector<Contour> contours;
    uint16_t gid;
};

struct GlyphName {
    uint16_t index;
    std::string name;
    
    GlyphName(uint16_t idx, const std::string& n) : index(idx), name(n) {}
};

std::vector<GlyphName> readAdobeGlyphList(const std::string& filename);
void loadStandardGlyphNamesMap(const std::string& filename, std::map<uint16_t, string>& standardGlyphMap);
string getStandardGlyphNameFast(uint16_t index, map<uint16_t, string>& standardGlyphMap);