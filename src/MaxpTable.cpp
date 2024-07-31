#include "MaxpTable.h"
#include <cstring>

MaxpTable::MaxpTable(
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

uint32_t MaxpTable::getVersion() const { return version; }
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

uint16_t MaxpTable::convertEndian16(uint16_t value) {
    return (value >> 8) | (value << 8);
}

uint32_t MaxpTable::convertEndian32(uint32_t value) {
    return ((value >> 24) & 0x000000FF) |
           ((value >> 8)  & 0x0000FF00) |
           ((value << 8)  & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
}

MaxpTable MaxpTable::parseMaxpDirectory(const std::vector<char>& data, uint16_t maxpTableOffset) {
    uint32_t version = convertEndian32(*reinterpret_cast<const uint32_t*>(&data[maxpTableOffset]));
    uint16_t numGlyphs = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[maxpTableOffset + 4]));
    uint16_t maxPoints = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[maxpTableOffset + 6]));
    uint16_t maxContours = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[maxpTableOffset + 8]));
    uint16_t maxComponentPoints = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[maxpTableOffset + 10]));
    uint16_t maxComponentContours = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[maxpTableOffset + 12]));
    uint16_t maxZones = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[maxpTableOffset + 14]));
    uint16_t maxTwighlightPoints = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[maxpTableOffset + 16]));
    uint16_t maxStorage = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[maxpTableOffset + 18]));
    uint16_t maxFunctionDefs = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[maxpTableOffset + 20]));
    uint16_t maxInstructionDefs = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[maxpTableOffset + 22]));
    uint16_t maxStackElements = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[maxpTableOffset + 24]));
    uint16_t maxComponentElements = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[maxpTableOffset + 26]));
    uint16_t maxSizeOfInstructions = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[maxpTableOffset + 28]));
    uint16_t maxComponentDepth = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[maxpTableOffset + 30]));

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

