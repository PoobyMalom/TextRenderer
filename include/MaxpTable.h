#pragma once
#include <cstdint>
#include <vector>

/*
Fixed	version	0x00010000 (1.0)
uint16	numGlyphs	the number of glyphs in the font
uint16	maxPoints	points in non-compound glyph
uint16	maxContours	contours in non-compound glyph
uint16	maxComponentPoints	points in compound glyph
uint16	maxComponentContours	contours in compound glyph
uint16	maxZones	set to 2
uint16	maxTwilightPoints	points used in Twilight Zone (Z0)
uint16	maxStorage	number of Storage Area locations
uint16	maxFunctionDefs	number of FDEFs
uint16	maxInstructionDefs	number of IDEFs
uint16	maxStackElements	maximum stack depth
uint16	maxSizeOfInstructions	byte count for glyph instructions
uint16	maxComponentElements	number of glyphs referenced at top level
uint16	maxComponentDepth	levels of recursion, set to 0 if font has only simple glyphs
*/

class MaxpTable {
public:
    MaxpTable(
        uint32_t version,
        uint16_t numGlyphs,
        uint16_t maxPoints,
        uint16_t maxContours,
        uint16_t maxComponentPoints,
        uint16_t maxComponentContours,
        uint16_t maxZones,
        uint16_t maxTwighlightPoints,
        uint16_t maxStorage,
        uint16_t maxFunctionDefs,
        uint16_t maxInstructionDefs,
        uint16_t maxStackElements,
        uint16_t maxComponentElements,
        uint16_t maxSizeOfInstructions,
        uint16_t maxComponentDepth
    );

    uint32_t getVersion() const;
    uint16_t getNumGlyphs() const;
    uint16_t getMaxPoints() const;
    uint16_t getMaxContours() const;
    uint16_t getMaxComponentPoints() const;
    uint16_t getMaxComponentContours() const;
    uint16_t getMaxZones() const;
    uint16_t getMaxTwighlightPoints() const;
    uint16_t getMaxStorage() const;
    uint16_t getMaxFunctionDefs() const;
    uint16_t getMaxInstructionDefs() const;
    uint16_t getMaxStackElements() const;
    uint16_t getMaxComponentElements() const;
    uint16_t getMaxSizeOfInstructions() const;
    uint16_t getMaxComponentDepth() const;    

    static MaxpTable parseMaxpDirectory(const std::vector<char>& data, uint16_t maxpTableOffset);
private:
    uint32_t version;
    uint16_t numGlyphs;
    uint16_t maxPoints;
    uint16_t maxContours;
    uint16_t maxComponentPoints;
    uint16_t maxComponentContours;
    uint16_t maxZones;
    uint16_t maxTwighlightPoints;
    uint16_t maxStorage;
    uint16_t maxFunctionDefs;
    uint16_t maxInstructionDefs;
    uint16_t maxStackElements;
    uint16_t maxComponentElements;
    uint16_t maxSizeOfInstructions;
    uint16_t maxComponentDepth;

    static uint16_t convertEndian16(uint16_t value);
    static uint32_t convertEndian32(uint32_t value);
};