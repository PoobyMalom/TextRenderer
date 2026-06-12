#include "GlyphTable.h"
#include "Helpers.h"
#include "TTFHeader.h"
#include "MovableLine.h"
#include <vector>
#include <tuple>
#include <iostream>
#include <bitset>
#include <cmath>
using namespace std;

Glyph::Glyph(
    int16_t numberOfContours,
    int16_t xMin,
    int16_t yMin,
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
    endPtsOfContours(endPtsOfContours),
    instructionLength(instructionLength),
    instructions(instructions),
    flags(flags),
    xCoordinates(xCoordinates),
    yCoordinates(yCoordinates) {}

int16_t Glyph::getNumberOfContours() const { return numberOfContours; }
int16_t Glyph::getXMin() const { return xMin; }
int16_t Glyph::getYMin() const { return yMin; }
int16_t Glyph::getXMax() const { return xMax; }
int16_t Glyph::getYMax() const { return yMax; }
vector<uint16_t> Glyph::getEndPtsOfContours() const { return endPtsOfContours; }
uint16_t Glyph::getInstructionLength() const { return instructionLength; }
vector<uint8_t> Glyph::getInstructions() const { return instructions; }
vector<uint8_t> Glyph::getFlags() const { return flags; }
vector<int16_t> Glyph::getXCoordinates() const { return xCoordinates; }
vector<int16_t> Glyph::getYCoordinates() const { return yCoordinates; }

Glyph Glyph::parseSimpleGlyph(const vector<char>& data, uint32_t offset, int16_t numberOfContours, int16_t xMin, int16_t yMin, int16_t xMax, int16_t yMax) {
    int pos = offset;

    vector<uint16_t> endPtsOfContours;
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
        if (flag & 8) { // Repeat flag
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
        if (flags[i] & 2) { // Short vector
            uint8_t delta = static_cast<uint8_t>(data[pos++]);
            if (!(flags[i] & 16)) { // Short vector with negative values
                currentX -= delta;
            } else { // Short vector with positive values
                currentX += delta;
            }
        } else if (!(flags[i] & 16)) { // Long vector
            currentX += readS16(data, pos);
        }
        xCoordinates.push_back(currentX);
    }

    vector<int16_t> yCoordinates;
    int16_t currentY = 0;
    for (int i = 0; i < totalPoints; ++i) {
        if (flags[i] & 4) { // Short vector
            uint8_t delta = static_cast<uint8_t>(data[pos++]);
            if (flags[i] & 32) { // Short vector with positive values
                currentY += delta;
            } else { // Short vector with negative values
                currentY -= delta;
            }
        } else if (!(flags[i] & 32)) { // Long vector
            currentY += readS16(data, pos);
        }
        yCoordinates.push_back(currentY);
    }

    return Glyph(numberOfContours, xMin, yMin, xMax, yMax, endPtsOfContours, instructionLength, instructions, flags, xCoordinates, yCoordinates);
}

Glyph Glyph::parseCompoundGlyph(const vector<char>& data, const vector<uint32_t>& locas, uint32_t glyfTableBase, uint32_t componentDataStart, int16_t xMin, int16_t yMin, int16_t xMax, int16_t yMax) {
    int pos = componentDataStart;
    bool keepGoing = true;
    vector<uint16_t> glyphIndexs;
    vector<int16_t> argument1s;
    vector<int16_t> argument2s;
    vector<float> as;
    vector<float> bs;
    vector<float> cs;
    vector<float> ds;
    vector<int32_t> ms;
    vector<int32_t> ns;

    while (keepGoing) {
        uint16_t flag = read2Bytes(data, pos);
        bool isWord = flag & 1;
        bool isXY = flag >> 1 & 1;
        bool moreGlyphs = flag >> 5 & 1;
        if (!moreGlyphs) {
            keepGoing = false;
        }
        bool weHaveScale = flag >> 3 & 1;
        bool weHaveXYScale = flag >> 6 & 1;
        bool weHaveTwoByTwo = flag >> 7 & 1;

        glyphIndexs.push_back(read2Bytes(data, pos));

        int16_t argument1;
        int16_t argument2;
        if (isWord) {
            if (isXY) {
                argument1 = read2Bytes(data, pos);
                argument2 = read2Bytes(data, pos);
            } else {
                argument1 = read2Bytes(data, pos);
                argument2 = read2Bytes(data, pos);
            }
        } else {
            if (isXY) {
                argument1 = static_cast<int16_t>(readByte(data, pos));
                argument2 = static_cast<int16_t>(readByte(data, pos));
            } else {
                argument1 = static_cast<int16_t>(readByte(data, pos));
                argument2 = static_cast<int16_t>(readByte(data, pos));
            }
        }
        argument1s.push_back(argument1);
        argument2s.push_back(argument2);

        float a = 1.0;
        float b = 0.0;
        float c = 0.0;
        float d = 1.0;

        if (weHaveScale) { // WE_HAVE_A_SCALE
            int16_t scale = static_cast<int16_t>(read2Bytes(data, pos)) / 16384.0f;
            a = scale;
            d = scale;
        } else if (weHaveXYScale) { // WE_HAVE_AN_X_AND_Y_SCALE
            a = static_cast<int16_t>(read2Bytes(data, pos)) / 16384.0f;
            d = static_cast<int16_t>(read2Bytes(data, pos)) / 16384.0f;
        } else if (weHaveTwoByTwo) { // WE_HAVE_A_TWO_BY_TWO
            a = static_cast<int16_t>(read2Bytes(data, pos)) / 16384.0f;
            b = static_cast<int16_t>(read2Bytes(data, pos)) / 16384.0f;
            c = static_cast<int16_t>(read2Bytes(data, pos)) / 16384.0f;
            d = static_cast<int16_t>(read2Bytes(data, pos)) / 16384.0f;
        }

        as.push_back(a);
        bs.push_back(b);
        cs.push_back(c);
        ds.push_back(d);

        float m0 = max(abs(a), abs(d));
        float n0 = max(abs(c), abs(d));
        float m;
        float n;
        if ((abs(a) - abs(c)) <= 33.0f / 65536.0f) {
            m = 2 * m0;
        } else {
            m = m0;
        }
        if ((abs(b) - abs(d)) <= 33.0f / 65536.0f) {
            n = 2 * n0;
        } else {
            n = n0;
        }

        ms.push_back(m);
        ns.push_back(n);
    }

    int16_t numberOfContours = 0;
    vector<uint16_t> endPtsOfContours;
    uint16_t instructionLength = 0;
    vector<uint8_t> instructions;
    vector<uint8_t> flags;
    vector<int16_t> xCoordinatesPush;
    vector<int16_t> yCoordinatesPush;
    for (u_long i = 0; i < glyphIndexs.size(); ++i) {
        size_t pointOffset = xCoordinatesPush.size();
        Glyph glyph = Glyph::parseGlyph(data, locas, glyfTableBase, glyfTableBase + locas[glyphIndexs[i]]);
        numberOfContours += glyph.getNumberOfContours();
        for (uint16_t endPtsOfContour : glyph.getEndPtsOfContours()) {
            endPtsOfContours.push_back(endPtsOfContour + pointOffset);
        }
        for (uint8_t flag : glyph.getFlags()) {
            flags.push_back(flag);
        }
        instructionLength += glyph.getInstructionLength();
        for (uint8_t instruction : glyph.getInstructions()) {
            instructions.push_back(instruction);
        }
        vector<int16_t> xCoordinates = glyph.getXCoordinates();
        vector<int16_t> yCoordinates = glyph.getYCoordinates();
        for (u_long j = 0; j < xCoordinates.size(); ++j) {
            float xPrime = as[i] * xCoordinates[j] + cs[i] * yCoordinates[j] + argument1s[i];
            float yPrime = bs[i] * xCoordinates[j] + ds[i] * yCoordinates[j] + argument2s[i];
            xCoordinatesPush.push_back(static_cast<int16_t>(std::round(xPrime)));
            yCoordinatesPush.push_back(static_cast<int16_t>(std::round(yPrime)));
        }
    }

    return Glyph(numberOfContours, xMin, yMin, xMax, yMax, endPtsOfContours, instructionLength, instructions, flags, xCoordinatesPush, yCoordinatesPush);
}


Glyph Glyph::parseGlyph(const vector<char>& data, const vector<uint32_t>& locas, uint32_t glyfTableBase, uint32_t offset) {
    int pos = offset;
    int16_t numberOfContours = read2Bytes(data, pos);
    int16_t xMin = read2Bytes(data, pos);
    int16_t yMin = read2Bytes(data, pos);
    int16_t xMax = read2Bytes(data, pos);
    int16_t yMax = read2Bytes(data, pos);

    if (numberOfContours >= 0) {
        return Glyph::parseSimpleGlyph(data, pos, numberOfContours, xMin, yMin, xMax, yMax);
    } else {
        return Glyph::parseCompoundGlyph(data, locas, glyfTableBase, pos, xMin, yMin, xMax, yMax);
    }
}

void Glyph::addPointsBetween() {
    vector<int16_t> newXCoordinates;
    vector<int16_t> newYCoordinates;
    vector<uint8_t> newFlags;
    vector<uint16_t> newEndPtsOfContours;

    size_t n = xCoordinates.size();
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

            bool isLastPointOnCurve = (flags[lastPointIndex] & 1);
            bool isFirstPointOnCurve = (flags[firstPointIndex] & 1);

            if (!isLastPointOnCurve && !isFirstPointOnCurve) {
                // Add an on-curve point between two off-curve points
                int16_t midX = (xCoordinates[lastPointIndex] + xCoordinates[firstPointIndex]) / 2;
                int16_t midY = (yCoordinates[lastPointIndex] + yCoordinates[firstPointIndex]) / 2;

                newXCoordinates.push_back(midX);
                newYCoordinates.push_back(midY);
                newFlags.push_back(1); // On-curve point
            } else if (isLastPointOnCurve && isFirstPointOnCurve) {
                // Add an off-curve point between two on-curve points
                int16_t midX = (xCoordinates[lastPointIndex] + xCoordinates[firstPointIndex]) / 2;
                int16_t midY = (yCoordinates[lastPointIndex] + yCoordinates[firstPointIndex]) / 2;

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
            bool isCurrentOnCurve = (flags[i] & 1);
            bool isNextOnCurve = (flags[nextIndex] & 1);

            // Add points between current and next points
            if (!isCurrentOnCurve && !isNextOnCurve) {
                // Add an on-curve point between two off-curve points
                int16_t midX = (xCoordinates[i] + xCoordinates[nextIndex]) / 2;
                int16_t midY = (yCoordinates[i] + yCoordinates[nextIndex]) / 2;

                newXCoordinates.push_back(midX);
                newYCoordinates.push_back(midY);
                newFlags.push_back(1); // On-curve point
            } else if (isCurrentOnCurve && isNextOnCurve) {
                // Add an off-curve point between two on-curve points
                int16_t midX = (xCoordinates[i] + xCoordinates[nextIndex]) / 2;
                int16_t midY = (yCoordinates[i] + yCoordinates[nextIndex]) / 2;

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

void Glyph::drawSimpleGlyph(SDL_Renderer* renderer, Glyph glyph, int xOffset, int yOffset, double scalingFactor, int screenHeight,int thickness) {
    vector<uint16_t> endpoints = glyph.getEndPtsOfContours();

    int currentContour = 0;
    int contourStartIndex = 0;

    vector<int16_t> xCoordinates = glyph.getXCoordinates();
    vector<int16_t> yCoordinates = glyph.getYCoordinates();

    vector<uint8_t> flags = glyph.getFlags();
    for (u_long j = 0; j < xCoordinates.size(); ++j) {
        uint8_t flag = flags[j];
        if (j > endpoints[currentContour]) {
            contourStartIndex = endpoints[currentContour] + 1;
            ++currentContour;
        }
        if (flag & 1) { // If the current point is an on-curve point
            auto wrapIdx = [&](size_t base, int offset) -> size_t {
                int len = endpoints[currentContour] - contourStartIndex + 1;
                return contourStartIndex + (base - contourStartIndex + offset) % len;
            };
            size_t ci = wrapIdx(j, 1);
            size_t ei = wrapIdx(j, 2);
            SDL_Point point1 = {
                static_cast<int>(xCoordinates[j] * scalingFactor + xOffset),
                static_cast<int>(screenHeight - (yCoordinates[j] * scalingFactor) + yOffset)
            };
            SDL_Point controlPoint = {
                static_cast<int>(xCoordinates[ci] * scalingFactor + xOffset),
                static_cast<int>(screenHeight - (yCoordinates[ci] * scalingFactor) + yOffset)
            };
            SDL_Point point2 = {
                static_cast<int>(xCoordinates[ei] * scalingFactor + xOffset),
                static_cast<int>(screenHeight - (yCoordinates[ei] * scalingFactor) + yOffset)
            };
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            DrawBezier(renderer, point1, controlPoint, point2);
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        }
    }
}

void Glyph::printGlyph() {
    cout << "----------------------------------------------------------------------" << endl;
    cout << "Number of contours: " << numberOfContours << endl;
    cout << "xMin: " << xMin << ", yMin: " << yMin << ", xMax: " << xMax << "yMax: " << yMax << endl;
    cout << "Instruction Length: " << instructionLength << endl;

    cout << "Endpoints of contours: ";
    for (uint16_t endpt : endPtsOfContours) {
        cout << endpt << ", ";
    } cout << endl;

    cout << "Endpoints of contours: ";
    for (uint8_t inst : instructions) {
        cout << static_cast<int>(inst) << ", ";
    } cout << endl;

    for (uint16_t i = 0; i < xCoordinates.size(); i++) {
        cout << "Point " << i << " X: " << xCoordinates[i] << ", Y: " << yCoordinates[i] << ", Flags: " << bitset<8>(flags[i]) << endl;
    }
}