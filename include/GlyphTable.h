#pragma once
#include "TTFTable.h"
#include "Helpers.h"
#include "FontTypes.h"
#include "Metrics.h"
#include "cdt/draw.h"
#include <vector>

class Glyph {
public:
    Glyph(
        int16_t numberOfContours,
        std::vector<int> windingDirections,
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
    const std::vector<int>& getWindingDirections() const;
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

    // Triangulates in glyph-local pixel space (origin at the glyph's own
    // baseline/left edge, not a pen position in some rendered string --
    // parsing has no notion of where the glyph ends up on screen). Callers
    // translate the cached result by their own (xOffset, yOffset) at draw
    // time; translation doesn't change triangle connectivity, so this only
    // needs to run once per glyph instead of once per frame.
    void triangulateGlyph(const FontTransform& ftrans);
    const std::vector<Triangle>& getTriangles() const;
    const std::vector<int>& getTrianglePositions() const;
    const std::vector<Vertex>& getCurveControlPoints() const;


private:
    int16_t numberOfContours;
    std::vector<int> windingDirections;
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

    std::vector<Triangle> triangles;
    // Triangles position 0 for inside the shape, 1 for exterior, 2 for interior
    std::vector<int> trianglePositions;
    // For curve triangles (position 1 or 2), which of the triangle's own
    // vertices is actually the off-curve control point -- CDT's mesh
    // triangle can store v0/v1/v2 in any rotation, not necessarily the
    // (onCurve, control, onCurve) order they were constructed in, so this
    // can't be assumed to always be v1. Unused (default-constructed) for
    // position-0 triangles.
    std::vector<Vertex> curveControlPoints;

    static uint16_t convertEndian16(uint16_t value);
    static uint32_t convertEndian32(uint32_t value);
};