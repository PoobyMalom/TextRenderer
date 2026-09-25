#pragma once
#include "TTFTable.h"
#include "Helpers.h"
#include "FontTypes.h"
#include <vector>

class Glyph {
public:
    Glyph(
        int16_t numberOfContours,
        FWord xMin,
        FWord yMin,
        FWord xMax,
        FWord yMax,
        std::vector<uint16_t> endPtsOfContours,
        uint16_t instructionLength,
        std::vector<uint8_t> instructions,
        std::vector<uint8_t> flags,
        std::vector<int16_t> xCoordinates,
        std::vector<int16_t> yCoordinates,
        UFWord advanceWidth,
        FWord leftSideBearing
    );

    int16_t getNumberOfContours() const;
    FWord getXMin() const;
    FWord getYMin() const;
    FWord getXMax() const;
    FWord getYMax() const;
    const std::vector<uint16_t>& getEndPtsOfContours() const;
    uint16_t getInstructionLength() const;
    const std::vector<uint8_t>& getInstructions() const;
    const std::vector<uint8_t>& getFlags() const;
    const std::vector<int16_t>& getXCoordinates() const;
    const std::vector<int16_t>& getYCoordinates() const;
    uint16_t getAdvanceWidth() const;
    int16_t getLeftSideBearing() const;
    void setAdvanceWidth(uint16_t width);
    void setLeftSideBearing(int16_t lsb);

    static Glyph parseGlyph(const std::vector<char>& data, const std::vector<uint32_t>& locas, uint32_t glyfTableBase, uint32_t glyphOffset);
    static Glyph parseSimpleGlyph(const std::vector<char>& data, uint32_t offset, int16_t numberOfContours, FWord xMin, FWord yMin, FWord xMax, FWord yMax);
    static Glyph parseCompoundGlyph(const std::vector<char>& data, const std::vector<uint32_t>& locas, uint32_t glyfTableBase, uint32_t componentDataStart, FWord xMin, FWord yMin, FWord xMax, FWord yMax);
    void addPointsBetween();
    void printGlyph();

private:
    int16_t numberOfContours;
    FWord xMin;
    FWord yMin;
    FWord xMax;
    FWord yMax;
    std::vector<uint16_t> endPtsOfContours;
    uint16_t instructionLength;
    std::vector<uint8_t> instructions;
    std::vector<uint8_t> flags;
    std::vector<int16_t> xCoordinates;
    std::vector<int16_t> yCoordinates;
    UFWord advanceWidth;
    FWord leftSideBearing;

    static uint16_t convertEndian16(uint16_t value);
    static uint32_t convertEndian32(uint32_t value);
};