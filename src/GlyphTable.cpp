#include "GlyphTable.h"
#include "Helpers.h"
#include "TTFHeader.h"
#include "cdt/draw.h"
#include "cdt/contour.h"
#include "cdt/constrain.h"
#include <vector>
#include <tuple>
#include <iostream>
#include <bitset>
#include <cmath>
using namespace std;

Glyph::Glyph(
    int16_t numberOfContours, // NOLINT(bugprone-easily-swappable-parameters)
    vector<int> windingDirections,
    FWord xMin,
    FWord yMin, // NOLINT(bugprone-easily-swappable-parameters)
    FWord xMax,
    FWord yMax,
    vector<uint16_t> endPtsOfContours,
    uint16_t instructionLength,
    vector<uint8_t> instructions,
    vector<uint8_t> flags,
    vector<int16_t> xCoordinates,
    vector<int16_t> yCoordinates,
    UFWord advanceWidth,
    FWord leftSideBearing
) : numberOfContours(numberOfContours),
    windingDirections(windingDirections),
    xMin(xMin),
    yMin(yMin),
    xMax(xMax),
    yMax(yMax),
    endPtsOfContours(std::move(endPtsOfContours)),
    instructionLength(instructionLength),
    instructions(std::move(instructions)),
    flags(std::move(flags)),
    xCoordinates(std::move(xCoordinates)),
    yCoordinates(std::move(yCoordinates)),
    advanceWidth(advanceWidth),
    leftSideBearing(leftSideBearing) {}

int16_t Glyph::getNumberOfContours() const { return numberOfContours; }
const vector<int>& Glyph::getWindingDirections() const { return windingDirections; }
FWord Glyph::getXMin() const { return xMin; }
FWord Glyph::getYMin() const { return yMin; }
FWord Glyph::getXMax() const { return xMax; }
FWord Glyph::getYMax() const { return yMax; }
const vector<uint16_t>& Glyph::getEndPtsOfContours() const { return endPtsOfContours; }
uint16_t Glyph::getInstructionLength() const { return instructionLength; }
const vector<uint8_t>& Glyph::getInstructions() const { return instructions; }
const vector<uint8_t>& Glyph::getFlags() const { return flags; }
const vector<int16_t>& Glyph::getXCoordinates() const { return xCoordinates; }
const vector<int16_t>& Glyph::getYCoordinates() const { return yCoordinates; }
uint16_t Glyph::getAdvanceWidth() const { return advanceWidth; }
int16_t Glyph::getLeftSideBearing() const { return leftSideBearing; }

void Glyph::setAdvanceWidth(uint16_t width) {
    advanceWidth = width;
}

void Glyph::setLeftSideBearing(int16_t lsb) {
    leftSideBearing = lsb;
}

Glyph Glyph::parseSimpleGlyph(const vector<char>& data, uint32_t offset, int16_t numberOfContours, int16_t xMin, int16_t yMin, int16_t xMax, int16_t yMax) { // NOLINT(bugprone-easily-swappable-parameters)
    int pos = static_cast<int>(offset);

    vector<uint16_t> endPtsOfContours;
    endPtsOfContours.reserve(numberOfContours);
    for (int i = 0; i < numberOfContours; ++i) { endPtsOfContours.push_back(read2Bytes(data, pos)); }

    uint16_t instructionLength = read2Bytes(data, pos);
    vector<uint8_t> instructions;
    instructions.reserve(instructionLength);
    for (int i = 0; i < instructionLength; ++i) { instructions.push_back(readByte(data, pos)); }

    vector<uint8_t> flags;
    uint16_t totalPoints = endPtsOfContours.back() + 1;
    for (int i = 0; i < totalPoints; ++i) {
        uint8_t flag = data[pos++];
        flags.push_back(flag);
        if ((flag & 8) != 0) { // Repeat flag
            uint8_t repeatCount = data[pos++];
            for (int j = 0; j < repeatCount; ++j) {
                flags.push_back(flag);
                ++i;
            }
        }
    }

    vector<int16_t> xCoordinates;
    int16_t currentX = 0;
    for (int i = 0; i < totalPoints; ++i) {
        if ((flags[i] & 2) != 0) { // Short vector
            auto delta = static_cast<uint8_t>(data[pos++]);
            if (!((flags[i] & 16) != 0)) { // Short vector with negative values
                currentX = static_cast<int16_t>(currentX - delta);
            } else { // Short vector with positive values
                currentX = static_cast<int16_t>(currentX + delta);
            }
        } else if (!((flags[i] & 16) != 0)) { // Long vector
            currentX = static_cast<int16_t>(currentX + readS16(data, pos));
        }
        xCoordinates.push_back(currentX);
    }

    vector<int16_t> yCoordinates;
    int16_t currentY = 0;
    for (int i = 0; i < totalPoints; ++i) {
        if ((flags[i] & 4) != 0) { // Short vector
            auto delta = static_cast<uint8_t>(data[pos++]);
            if ((flags[i] & 32) != 0) { // Short vector with positive values
                currentY = static_cast<int16_t>(currentY + delta);
            } else { // Short vector with negative values
                currentY = static_cast<int16_t>(currentY - delta);
            }
        } else if (!((flags[i] & 32) != 0)) { // Long vector
            currentY = static_cast<int16_t>(currentY + readS16(data, pos));
        }
        yCoordinates.push_back(currentY);
    }

    vector<int> windingDirections = calculateWindingDirections(endPtsOfContours, xCoordinates, yCoordinates);

    return {numberOfContours, windingDirections, xMin, yMin, xMax, yMax, endPtsOfContours, instructionLength, instructions, flags, xCoordinates, yCoordinates, 0, 0};
}

Glyph Glyph::parseCompoundGlyph(const vector<char>& data, const vector<uint32_t>& locas, uint32_t glyfTableBase, uint32_t componentDataStart, int16_t xMin, int16_t yMin, int16_t xMax, int16_t yMax) { // NOLINT(bugprone-easily-swappable-parameters, readability-function-cognitive-complexity)
    struct ComponentData { // NOLINT(readability-identifier-length)
        uint16_t glyphIndex;
        float a, b, c, d;
        int16_t dx, dy;
    };

    int pos = static_cast<int>(componentDataStart);
    bool keepGoing = true;
    vector<ComponentData> components;

    while (keepGoing) {
        uint16_t flag = read2Bytes(data, pos);
        bool isWord = (flag & 1) != 0;
        bool isXY = (flag >> 1 & 1) != 0;
        bool moreGlyphs = (flag >> 5 & 1) != 0;
        if (!moreGlyphs) {
            keepGoing = false;
        }
        bool weHaveScale = (flag >> 3 & 1) != 0;
        bool weHaveXYScale = (flag >> 6 & 1) != 0;
        bool weHaveTwoByTwo = (flag >> 7 & 1) != 0;

        ComponentData comp{};
        comp.glyphIndex = read2Bytes(data, pos);

        if (isWord) {
            if (isXY) {
                // Signed 16-bit xy offsets
                comp.dx = readS16(data, pos);
                comp.dy = readS16(data, pos);
            } else {
                // Unsigned 16-bit point indices — anchor matching not implemented; consume and zero
                read2Bytes(data, pos);
                read2Bytes(data, pos);
                comp.dx = 0;
                comp.dy = 0;
            }
        } else {
            if (isXY) {
                // Signed 8-bit xy offsets — cast via int8_t to sign-extend
                comp.dx = static_cast<int16_t>(static_cast<int8_t>(readByte(data, pos)));
                comp.dy = static_cast<int16_t>(static_cast<int8_t>(readByte(data, pos)));
            } else {
                // Unsigned 8-bit point indices — anchor matching not implemented; consume and zero
                readByte(data, pos);
                readByte(data, pos);
                comp.dx = 0;
                comp.dy = 0;
            }
        }

        comp.a = 1.0F;
        comp.b = 0.0F;
        comp.c = 0.0F;
        comp.d = 1.0F;

        if (weHaveScale) {
            auto scale = static_cast<float>(static_cast<int16_t>(read2Bytes(data, pos))) / 16384.0F;
            comp.a = scale;
            comp.d = scale;
        } else if (weHaveXYScale) {
            comp.a = static_cast<float>(static_cast<int16_t>(read2Bytes(data, pos))) / 16384.0F;
            comp.d = static_cast<float>(static_cast<int16_t>(read2Bytes(data, pos))) / 16384.0F;
        } else if (weHaveTwoByTwo) {
            comp.a = static_cast<float>(static_cast<int16_t>(read2Bytes(data, pos))) / 16384.0F;
            comp.b = static_cast<float>(static_cast<int16_t>(read2Bytes(data, pos))) / 16384.0F;
            comp.c = static_cast<float>(static_cast<int16_t>(read2Bytes(data, pos))) / 16384.0F;
            comp.d = static_cast<float>(static_cast<int16_t>(read2Bytes(data, pos))) / 16384.0F;
        }

        components.push_back(comp);
    }

    int16_t numberOfContours = 0;
    vector<uint16_t> endPtsOfContours;
    uint16_t instructionLength = 0;
    vector<uint8_t> instructions;
    vector<uint8_t> flags;
    vector<int16_t> xCoordinatesPush;
    vector<int16_t> yCoordinatesPush;
    for (size_t i = 0; i < components.size(); ++i) {
        const ComponentData& comp = components[i];
        size_t pointOffset = xCoordinatesPush.size();
        Glyph glyph = Glyph::parseGlyph(data, locas, glyfTableBase, glyfTableBase + locas[comp.glyphIndex]);
        numberOfContours = static_cast<int16_t>(glyph.getNumberOfContours() + numberOfContours);
        for (uint16_t endPt : glyph.getEndPtsOfContours()) {
            endPtsOfContours.push_back(static_cast<uint16_t>(endPt + pointOffset));
        }
        for (uint8_t f : glyph.getFlags()) {
            flags.push_back(f);
        }
        instructionLength += glyph.getInstructionLength();
        for (uint8_t instr : glyph.getInstructions()) {
            instructions.push_back(instr);
        }
        const vector<int16_t>& xCoords = glyph.getXCoordinates();
        const vector<int16_t>& yCoords = glyph.getYCoordinates();
        for (size_t j = 0; j < xCoords.size(); ++j) {
            float xPrime = (comp.a * static_cast<float>(xCoords[j])) + (comp.c * static_cast<float>(yCoords[j])) + static_cast<float>(comp.dx);
            float yPrime = (comp.b * static_cast<float>(xCoords[j])) + (comp.d * static_cast<float>(yCoords[j])) + static_cast<float>(comp.dy);
            xCoordinatesPush.push_back(static_cast<int16_t>(std::round(xPrime)));
            yCoordinatesPush.push_back(static_cast<int16_t>(std::round(yPrime)));
        }
    }

    vector<int> windingDirections = calculateWindingDirections(endPtsOfContours, xCoordinatesPush, yCoordinatesPush);

    return {numberOfContours, windingDirections, xMin, yMin, xMax, yMax, endPtsOfContours, instructionLength, instructions, flags, xCoordinatesPush, yCoordinatesPush, 0, 0};
}


Glyph Glyph::parseGlyph(const vector<char>& data, const vector<uint32_t>& locas, uint32_t glyfTableBase, uint32_t offset) { // NOLINT(bugprone-easily-swappable-parameters)
    int pos = static_cast<int>(offset);
    auto numberOfContours = static_cast<int16_t>(read2Bytes(data, pos));
    auto xMin = static_cast<int16_t>(read2Bytes(data, pos));
    auto yMin = static_cast<int16_t>(read2Bytes(data, pos));
    auto xMax = static_cast<int16_t>(read2Bytes(data, pos));
    auto yMax = static_cast<int16_t>(read2Bytes(data, pos));

    if (numberOfContours >= 0) {
        return Glyph::parseSimpleGlyph(data, pos, numberOfContours, xMin, yMin, xMax, yMax);
    }

    return Glyph::parseCompoundGlyph(data, locas, glyfTableBase, pos, xMin, yMin, xMax, yMax);
}

void Glyph::addPointsBetween() {
    vector<int16_t> newXCoordinates;
    vector<int16_t> newYCoordinates;
    vector<uint8_t> newFlags;
    vector<uint16_t> newEndPtsOfContours;

    size_t n = xCoordinates.size(); // NOLINT(readability-identifier-length)
    size_t contourIndex = 0;
    size_t contourStartIndex = 0;

    for (size_t i = 0; i < n; ++i) {
        // Add the current point
        newXCoordinates.push_back(xCoordinates[i]);
        newYCoordinates.push_back(yCoordinates[i]);
        newFlags.push_back(flags[i]);

        // Check if the next point is part of the same contour or a new contour
        if (contourIndex < endPtsOfContours.size() && i == endPtsOfContours[contourIndex]) {
            // Handle wrap-around case for the end of the contour
            size_t firstPointIndex = contourStartIndex;
            size_t lastPointIndex = i;

            bool isLastPointOnCurve = (flags[lastPointIndex] & 1) != 0;
            bool isFirstPointOnCurve = (flags[firstPointIndex] & 1) != 0;

            if (!isLastPointOnCurve && !isFirstPointOnCurve) {
                // Add an on-curve point between two off-curve points
                auto midX = static_cast<int16_t>((xCoordinates[lastPointIndex] + xCoordinates[firstPointIndex]) / 2);
                auto midY = static_cast<int16_t>((yCoordinates[lastPointIndex] + yCoordinates[firstPointIndex]) / 2);

                newXCoordinates.push_back(midX);
                newYCoordinates.push_back(midY);
                newFlags.push_back(1); // On-curve point
            }

            // Move to the next contour
            newEndPtsOfContours.push_back(newXCoordinates.size() - 1);
            contourIndex++;
            contourStartIndex = i + 1;
        } else {
            // Determine the index of the next point (wrap around within the same contour)
            size_t nextIndex = (i + 1) % n;

            if (contourIndex < endPtsOfContours.size() && nextIndex > endPtsOfContours[contourIndex]) {
                nextIndex = contourStartIndex;
            }

            // Determine if current and next points are on-curve or off-curve
            bool isCurrentOnCurve = (flags[i] & 1) != 0;
            bool isNextOnCurve = (flags[nextIndex] & 1) != 0;

            // Add points between current and next points
            if (!isCurrentOnCurve && !isNextOnCurve) {
                // Add an on-curve point between two off-curve points
                auto midX = static_cast<int16_t>((xCoordinates[i] + xCoordinates[nextIndex]) / 2);
                auto midY = static_cast<int16_t>((yCoordinates[i] + yCoordinates[nextIndex]) / 2);

                newXCoordinates.push_back(midX);
                newYCoordinates.push_back(midY);
                newFlags.push_back(1); // On-curve point
            }
        }
    }

    // Set updated values
    xCoordinates = newXCoordinates;
    yCoordinates = newYCoordinates;
    flags = newFlags;
    endPtsOfContours = newEndPtsOfContours;
}

void Glyph::printGlyph() {
    cout << "----------------------------------------------------------------------" << '\n';
    cout << "Number of contours: " << numberOfContours << '\n';
    cout << "xMin: " << xMin << ", yMin: " << yMin << ", xMax: " << xMax << "yMax: " << yMax << '\n';
    cout << "Instruction Length: " << instructionLength << '\n';

    cout << "Endpoints of contours: ";
    for (uint16_t endpt : endPtsOfContours) {
        cout << endpt << ", ";
    } cout << '\n';

    cout << "Endpoints of contours: ";
    for (uint8_t inst : instructions) {
        cout << static_cast<int>(inst) << ", ";
    } cout << '\n';

    for (size_t i = 0; i < xCoordinates.size(); i++) {
        cout << "Point " << i << " X: " << xCoordinates[i] << ", Y: " << yCoordinates[i] << ", Flags: " << bitset<8>(flags[i]) << '\n';
    }
}

const vector<Triangle>& Glyph::getTriangles() const { return triangles; }


void Glyph::triangulateGlyph(const FontTransform& ftrans) {
    // Local pixel space: no xOffset/yOffset -- those depend on where this
    // glyph lands in a rendered string, which parsing doesn't know. The
    // caller translates the cached result at draw time instead.
    auto toVertex = [&](size_t idx) -> Vertex {
        int px = static_cast<int>(ftrans.toPixels(xCoordinates[idx]));
        int py = ftrans.toScreenY(yCoordinates[idx], 0);
        return {px, py};
    };

    // Shoelace winding sign, usable on a contour (N points) or a single
    // triangle (3 points) alike -- as long as both callers pass points in
    // the same coordinate space, the results are directly comparable. Using
    // one function for both matters: comparing a contour winding computed in
    // font-unit (y-up) space against a triangle winding computed in screen
    // (y-down) space would have opposite sign conventions for the same
    // real-world shape.
    auto windingSign = [](const std::vector<Vertex>& pts) -> int {
        long long area2 = 0;
        for (size_t i = 0; i < pts.size(); ++i) {
            const Vertex& a = pts[i];
            const Vertex& b = pts[(i + 1) % pts.size()];
            area2 += static_cast<long long>(a.x) * b.y - static_cast<long long>(b.x) * a.y;
        }
        return area2 < 0 ? 1 : 0; // 1 = clockwise, 0 = counter-clockwise (screen space)
    };

    vector<Contour> contours;
    contours.reserve(endPtsOfContours.size());
    vector<Vertex> extraVertices;        // off-curve control points, inserted alongside the contours
    vector<Edge> extraConstraints;       // each curve's two legs: on-curve -> control -> on-curve
    vector<Triangle> expectedCurveTriangles; // the exact (on, control, on) triangle for each curve segment

    int contourStartIndex = 0;
    for (uint16_t endpoint : endPtsOfContours) {
        auto len = static_cast<int>(endpoint) - contourStartIndex + 1;
        Contour contour;

        for (int k = 0; k < len; ++k) {
            size_t idx = contourStartIndex + k;
            if ((flags[idx] & 1) == 0) {
                continue; // off-curve points are only handled via their on-curve neighbor below
            }

            Vertex onCurvePoint = toVertex(idx);
            // Two distinct font-unit coordinates can round to the same
            // screen pixel at small font sizes (seen in Astloch-Regular's
            // 'A', where indices 45/46 both land on (58,271) at size 400).
            // Feeding triangulateConstrained a duplicate point makes it
            // insert a vertex that's already there -- the new "triangle"
            // ends up with two identical corners, a zero-area sliver that
            // renders as a stray spike. Collisions aren't always between
            // adjacent font points, so check the whole contour so far.
            bool isDuplicate = false;
            for (const Vertex& existing : contour) {
                if (existing.x == onCurvePoint.x && existing.y == onCurvePoint.y) {
                    isDuplicate = true;
                    break;
                }
            }
            if (!isDuplicate) {
                contour.push_back(onCurvePoint);
            }

            // (on, off, on): a curve segment. Its control triangle's chord
            // (this on-curve point to the next one) is already a constraint
            // via `contour` above; add the two legs through the control
            // point too, so the whole control triangle is forced intact.
            size_t nextIdx = contourStartIndex + ((k + 1) % len);
            if ((flags[nextIdx] & 1) == 0) {
                size_t endIdx = contourStartIndex + ((k + 2) % len);
                Vertex control = toVertex(nextIdx);
                Vertex curveEnd = toVertex(endIdx);

                extraVertices.push_back(control);
                extraConstraints.push_back({onCurvePoint, control});
                extraConstraints.push_back({control, curveEnd});
                expectedCurveTriangles.push_back({onCurvePoint, control, curveEnd});
            }
        }

        contours.push_back(contour);
        contourStartIndex = static_cast<int>(endpoint) + 1;
    }

    triangles = triangulateConstrained(contours, extraVertices, extraConstraints, expectedCurveTriangles);

    auto sameVertexSet = [](const Triangle& a, const Triangle& b) -> bool {
        Vertex av[3] = {a.v0, a.v1, a.v2};
        Vertex bv[3] = {b.v0, b.v1, b.v2};
        bool used[3] = {false, false, false};
        for (const Vertex& vertex : av) {
            bool found = false;
            for (int j = 0; j < 3; ++j) {
                if (!used[j] && vertex == bv[j]) {
                    used[j] = true;
                    found = true;
                    break;
                }
            }
            if (!found) {
                return false;
            }
        }
        return true;
    };

    // Each triangle's position on the glyph: 0 = ordinary solid fill, 1 =
    // curve wedge using the direct Loop-Blinn test, 2 = curve wedge using
    // the flipped test. This is an ABSOLUTE per-wedge decision based on the
    // wedge's own (onCurve, control, onCurve) screen-space winding -- not
    // compared against its contour's winding. A fixed per-contour flip
    // (hole vs. outer) fixed holes but broke single-contour glyphs with
    // real local concave/convex variation within the same contour (e.g.
    // 'f''s hook curls the opposite way from the rest of its one contour).
    // Comparing each wedge's own absolute orientation to a fixed threshold
    // handles both: it was already equivalent to the per-contour rule for
    // any contour whose own winding happens to be 0 (that's why fixing
    // outer contours never needed revisiting), and naturally varies
    // per-wedge within a single contour where the old per-contour rule
    // couldn't.
    for (const Triangle& triangle : triangles) {
        size_t matchedIndex = expectedCurveTriangles.size();
        for (size_t ci = 0; ci < expectedCurveTriangles.size(); ++ci) {
            if (sameVertexSet(triangle, expectedCurveTriangles[ci])) {
                matchedIndex = ci;
                break;
            }
        }

        if (matchedIndex < expectedCurveTriangles.size()) {
            const Triangle& expected = expectedCurveTriangles[matchedIndex];
            int triangleWinding = windingSign({expected.v0, expected.v1, expected.v2});
            trianglePositions.push_back(triangleWinding == 0 ? 1 : 2);
            // expectedCurveTriangles[matchedIndex] was built as (onCurve,
            // control, onCurve), but sameVertexSet matches regardless of
            // order, so `triangle`'s own v0/v1/v2 may have the control point
            // at any position -- record which vertex it actually is.
            curveControlPoints.push_back(expected.v1);
        } else {
            trianglePositions.push_back(0);
            curveControlPoints.push_back({});
        }
    }

}

const vector<int>& Glyph::getTrianglePositions() const { return trianglePositions; }
const vector<Vertex>& Glyph::getCurveControlPoints() const { return curveControlPoints; }
