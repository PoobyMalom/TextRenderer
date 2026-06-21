#include "GlyphTable.h"
#include "Helpers.h"
#include "TTFHeader.h"
#include <vector>
#include <tuple>
#include <iostream>
#include <bitset>
#include <cmath>
using namespace std;

Glyph::Glyph(
    int16_t numberOfContours, // NOLINT(bugprone-easily-swappable-parameters)
    int16_t xMin,
    int16_t yMin, // NOLINT(bugprone-easily-swappable-parameters)
    int16_t xMax,
    int16_t yMax,
    vector<uint16_t> endPtsOfContours,
    uint16_t instructionLength,
    vector<uint8_t> instructions,
    vector<uint8_t> flags,
    vector<int16_t> xCoordinates,
    vector<int16_t> yCoordinates
) : numberOfContours(numberOfContours),
    xMin(xMin),
    yMin(yMin),
    xMax(xMax),
    yMax(yMax),
    endPtsOfContours(std::move(endPtsOfContours)),
    instructionLength(instructionLength),
    instructions(std::move(instructions)),
    flags(std::move(flags)),
    xCoordinates(std::move(xCoordinates)),
    yCoordinates(std::move(yCoordinates)) {}

int16_t Glyph::getNumberOfContours() const { return numberOfContours; }
int16_t Glyph::getXMin() const { return xMin; }
int16_t Glyph::getYMin() const { return yMin; }
int16_t Glyph::getXMax() const { return xMax; }
int16_t Glyph::getYMax() const { return yMax; }
const vector<uint16_t>& Glyph::getEndPtsOfContours() const { return endPtsOfContours; }
uint16_t Glyph::getInstructionLength() const { return instructionLength; }
const vector<uint8_t>& Glyph::getInstructions() const { return instructions; }
const vector<uint8_t>& Glyph::getFlags() const { return flags; }
const vector<int16_t>& Glyph::getXCoordinates() const { return xCoordinates; }
const vector<int16_t>& Glyph::getYCoordinates() const { return yCoordinates; }

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

    return {numberOfContours, xMin, yMin, xMax, yMax, endPtsOfContours, instructionLength, instructions, flags, xCoordinates, yCoordinates};
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

    return {numberOfContours, xMin, yMin, xMax, yMax, endPtsOfContours, instructionLength, instructions, flags, xCoordinatesPush, yCoordinatesPush};
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
            } else if (isLastPointOnCurve && isFirstPointOnCurve) {
                // Add an off-curve midpoint so straight segments are also driven as Bezier curves
                auto midX = static_cast<int16_t>((xCoordinates[lastPointIndex] + xCoordinates[firstPointIndex]) / 2);
                auto midY = static_cast<int16_t>((yCoordinates[lastPointIndex] + yCoordinates[firstPointIndex]) / 2);

                newXCoordinates.push_back(midX);
                newYCoordinates.push_back(midY);
                newFlags.push_back(0); // Off-curve point
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
            } else if (isCurrentOnCurve && isNextOnCurve) {
                // Add an off-curve midpoint so straight segments are also driven as Bezier curves
                auto midX = static_cast<int16_t>((xCoordinates[i] + xCoordinates[nextIndex]) / 2);
                auto midY = static_cast<int16_t>((yCoordinates[i] + yCoordinates[nextIndex]) / 2);

                newXCoordinates.push_back(midX);
                newYCoordinates.push_back(midY);
                newFlags.push_back(0); // Off-curve point
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