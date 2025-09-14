#include "MaxpTable.h"
#include "Helpers.h"
#include <cstring>

MaxpTable::MaxpTable(
    double version,
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
) : version(version),
    numGlyphs(numGlyphs),
    maxPoints(maxPoints),
    maxContours(maxContours),
    maxComponentPoints(maxComponentPoints),
    maxComponentContours(maxComponentContours),
    maxZones(maxZones),
    maxTwighlightPoints(maxTwighlightPoints),
    maxStorage(maxStorage),
    maxFunctionDefs(maxFunctionDefs),
    maxInstructionDefs(maxInstructionDefs),
    maxStackElements(maxStackElements),
    maxComponentElements(maxComponentElements),
    maxSizeOfInstructions(maxSizeOfInstructions),
    maxComponentDepth(maxComponentDepth) {}

double MaxpTable::getVersion() const { return version; }
uint16_t MaxpTable::getNumGlyphs() const { return numGlyphs; }
uint16_t MaxpTable::getMaxPoints() const { return maxPoints; }
uint16_t MaxpTable::getMaxContours() const { return maxContours; }
uint16_t MaxpTable::getMaxComponentPoints() const { return maxComponentPoints; }
uint16_t MaxpTable::getMaxComponentContours() const { return maxComponentContours; }
uint16_t MaxpTable::getMaxZones() const { return maxZones; }
uint16_t MaxpTable::getMaxTwighlightPoints() const { return maxTwighlightPoints; }
uint16_t MaxpTable::getMaxStorage() const { return maxStorage; }
uint16_t MaxpTable::getMaxFunctionDefs() const { return maxFunctionDefs; }
uint16_t MaxpTable::getMaxInstructionDefs() const { return maxInstructionDefs; }
uint16_t MaxpTable::getMaxStackElements() const { return maxStackElements; }
uint16_t MaxpTable::getMaxComponentElements() const { return maxComponentElements; }
uint16_t MaxpTable::getMaxSizeOfInstructions() const { return maxSizeOfInstructions; }
uint16_t MaxpTable::getMaxComponentDepth() const { return maxComponentDepth; }

MaxpTable MaxpTable::parseMaxpDirectory(const std::vector<char>& data, uint16_t maxpTableOffset) {
    int pos = maxpTableOffset;
    uint32_t version_u = read4Bytes(data, pos);
    int32_t version_raw = static_cast<int32_t>(version_u);
    double version = version_raw / 65536.0;
    uint16_t numGlyphs = read2Bytes(data, pos);
    uint16_t maxPoints = read2Bytes(data, pos);
    uint16_t maxContours = read2Bytes(data, pos);
    uint16_t maxComponentPoints = read2Bytes(data, pos);
    uint16_t maxComponentContours = read2Bytes(data, pos);
    uint16_t maxZones = read2Bytes(data, pos);
    uint16_t maxTwighlightPoints = read2Bytes(data, pos);
    uint16_t maxStorage = read2Bytes(data, pos);
    uint16_t maxFunctionDefs = read2Bytes(data, pos);
    uint16_t maxInstructionDefs = read2Bytes(data, pos);
    uint16_t maxStackElements = read2Bytes(data, pos);
    uint16_t maxComponentElements = read2Bytes(data, pos);
    uint16_t maxSizeOfInstructions = read2Bytes(data, pos);
    uint16_t maxComponentDepth = read2Bytes(data, pos);

    return MaxpTable(
        version,
        numGlyphs,
        maxPoints,
        maxContours,
        maxComponentPoints,
        maxComponentContours,
        maxZones,
        maxTwighlightPoints,
        maxStorage,
        maxFunctionDefs,
        maxInstructionDefs,
        maxStackElements,
        maxComponentElements,
        maxSizeOfInstructions,
        maxComponentDepth
    );
}

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

void MaxpTable::printMaxpTable() {
    puts("-------------------------------------------------");
    puts("Maxp Table Information");
    printf("Version: %f\n", version);
    printf("Number of Glyphs: %u\n", numGlyphs);
    printf("Max Points (Non Compound): %u\n", maxPoints);
    printf("Max Contours (Non Compound): %u\n", maxContours);
    printf("Max Component Points (Compound): %u\n", maxComponentPoints);
    printf("Max Component Contours (Compound): %u\n", maxComponentContours);
    printf("Max Zones: %u\n", maxZones);
    printf("Max Twilight Points: %u\n", maxTwighlightPoints);
    printf("Max Storage: %u\n", maxStorage);
    printf("Max Function Definitions: %u\n", maxFunctionDefs);
    printf("Max Instruction Definitions: %u\n", maxInstructionDefs);
    printf("Max Stack Elements: %u\n", maxStackElements);
    printf("Max Size of Instructions: %u\n", maxSizeOfInstructions);
    printf("Max Component Elements: %u\n", maxComponentElements);
    printf("Max Component Depth: %u\n", maxComponentDepth);
    puts("-------------------------------------------------");
}

