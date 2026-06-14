#include "MaxpTable.h"
#include "Helpers.h"
#include <cstring>

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

    return {
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
    };
}
