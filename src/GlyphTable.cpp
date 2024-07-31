#include "GlyphTable.h"
#include <vector>
using namespace std;

Glyph::Glyph() {}

int16_t Glyph::getNumberOfContours() const {
    return numberOfContours;
}

int16_t Glyph::getXMin() const {
    return xMin;
}

int16_t Glyph::getYMin() const {
    return yMin;
}

int16_t Glyph::getXMax() const {
    return xMax;
}

int16_t Glyph::getYMax() const {
    return yMax;
}

vector<uint16_t> const& Glyph::getEndPtsOfContours() const {
    return endPtsOfContours;
}

uint16_t Glyph::getInstructionLength() const {
    return instructionLength;
}

vector<uint8_t> const& Glyph::getInstructions() const {
    return instructions;
}

vector<uint8_t> const& Glyph::getFlags() const {
    return flags;
}

void Glyph::parse(const vector<char>& data, uint8_t flags) {

}

GlyphTable::GlyphTable(string const& tag, uint32_t checksum, uint32_t offset, uint32_t length) : TTFTable(tag, checksum, offset, length) {}

vector<Glyph> const& GlyphTable::getGlyphs() const {
    return glyphs;
}

void GlyphTable::parse(const vector<char>& data, uint32_t Tableoffset) {
    vector<Glyph> glyphs;

    
}