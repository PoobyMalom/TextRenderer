#pragma once
#include "TTFTable.h"
#include <vector>

class Glyph {
public:
    Glyph();
    void parse(const std::vector<char>& data, uint8_t flags);

    int16_t getNumberOfContours() const;
    int16_t getXMin() const;
    int16_t getYMin() const;
    int16_t getXMax() const;
    int16_t getYMax() const;
    const std::vector<uint16_t>& getEndPtsOfContours() const;
    uint16_t getInstructionLength() const;
    const std::vector<uint8_t>& getInstructions() const;
    const std::vector<uint8_t>& getFlags() const;

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
};

/*
uint16	endPtsOfContours[n]	Array of last points of each contour; n is the number of contours; array entries are point indices
uint16	instructionLength	Total number of bytes needed for instructions
uint8	instructions[instructionLength]	Array of instructions for this glyph
uint8	flags[variable]	Array of flags
uint8 or int16	xCoordinates[]	Array of x-coordinates; the first is relative to (0,0), others are relative to previous point
uint8 or int16	yCoordinates[]	Array of y-coordinates; the first is relative to (0,0), others are relative to previous point
*/

class GlyphTable : public TTFTable {
public:
    GlyphTable(const std::string& tag, uint32_t checksum, uint32_t offset, uint32_t length);
    void parse(const std::vector<char>& data, uint32_t offset);
    const std::vector<Glyph>& getGlyphs() const;

private:
    std::vector<Glyph> glyphs;
};