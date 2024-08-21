#include "MaxpTable.h"
#include "Helpers.h"
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

MaxpTable MaxpTable::parseMaxpDirectory(const std::vector<char>& data, uint16_t maxpTableOffset) {
    int pos = maxpTableOffset;
    uint32_t version = read4Bytes(data, pos);
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

