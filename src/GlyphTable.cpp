#include "GlyphTable.h"
#include "Helpers.h"
#include "TTFHeader.h"
#include "TTFFile.h"
#include <vector>
#include <iostream>
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

int16_t Glyph::getNumberOfContours() const {
    return numberOfContours;
}

int16_t Glyph::getXMin() const {
    return xMin;
}

int16_t Glyph::getYMin() const {
    return yMin;
}

int16_t Glyph::getXMax() const {
    return xMax;
}

int16_t Glyph::getYMax() const {
    return yMax;
}

vector<uint16_t> Glyph::getEndPtsOfContours() const {
    return endPtsOfContours;
}

uint16_t Glyph::getInstructionLength() const {
    return instructionLength;
}

vector<uint8_t> Glyph::getInstructions() const {
    return instructions;
}

vector<uint8_t> Glyph::getFlags() const {
    return flags;
}

vector<int16_t> Glyph::getXCoordinates() const {
    return xCoordinates;
}

vector<int16_t> Glyph::getYCoordinates() const {
    return yCoordinates;
}

Glyph Glyph::parseSimpleGlyph(const vector<char>& data, uint32_t pos, int16_t numberOfContours, int16_t xMin, int16_t yMin, int16_t xMax, int16_t yMax) {
    vector<uint16_t> endPtsOfContours;
    for (int i = 0; i < numberOfContours; ++i) {
        uint16_t endPtsOfContour = Glyph::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[pos]));
        endPtsOfContours.push_back(endPtsOfContour);
        pos += 2;
    }

    uint16_t instructionLength = Glyph::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[pos]));
    pos += 2;
    vector<uint8_t> instructions(instructionLength);
    std::memcpy(instructions.data(), &data[pos], instructionLength);
    pos += instructionLength;

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
            int16_t delta = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
            currentX += delta;
            pos += 2;
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
            int16_t delta = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
            currentY += delta;
            pos += 2;
        }
        yCoordinates.push_back(currentY);
    }

    return Glyph(numberOfContours, xMin, yMin, xMax, yMax, endPtsOfContours, instructionLength, instructions, flags, xCoordinates, yCoordinates);
}

Glyph Glyph::parseCompoundGlyph(const vector<char>& data, uint32_t pos, int16_t xMin, int16_t yMin, int16_t xMax, int16_t yMax) {
    bool keepGoing = true;
    vector<uint16_t> glyphIndexs;
    vector<int16_t> argument1s;
    vector<int16_t> argument2s;
    vector<int16_t> as;
    vector<int16_t> bs;
    vector<int16_t> cs;
    vector<int16_t> ds;
    vector<int32_t> ms;
    vector<int32_t> ns;

    while (keepGoing) {
        uint16_t flag = Glyph::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[pos]));
        pos += 2;
        bool isWord = flag & 1;
        bool isXY = flag >> 1 & 1;
        bool moreGlyphs = flag >> 5 & 1;
        if (!moreGlyphs) {
            keepGoing = false;
        }
        bool weHaveScale = flag >> 3 & 1;
        bool weHaveXYScale = flag >> 6 & 1;
        bool weHaveTwoByTwo = flag >> 7 & 1;
        uint16_t glyphIndex = Glyph::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[pos]));
        glyphIndexs.push_back(glyphIndex);
        pos += 2;
        int16_t argument1;
        int16_t argument2;
        if (isWord) {
            if (isXY) {
                argument1 = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
                pos += 2;
                argument2 = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
                pos += 2;
            } else {
                argument1 = Glyph::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[pos]));
                pos += 2;
                argument2 = Glyph::convertEndian16(*reinterpret_cast<const uint16_t*>(&data[pos]));
                pos += 2;
            }
        } else {
            if (isXY) {
                argument1 = static_cast<int16_t>(*reinterpret_cast<const int8_t*>(&data[pos]));
                pos += 1;
                argument2 = static_cast<int16_t>(*reinterpret_cast<const int8_t*>(&data[pos]));
                pos += 1;
            } else {
                argument1 = static_cast<int16_t>(*reinterpret_cast<const uint8_t*>(&data[pos]));
                pos += 1;
                argument2 = static_cast<int16_t>(*reinterpret_cast<const uint8_t*>(&data[pos]));
                pos += 1;
            }
        }
        argument1s.push_back(argument1);
        argument2s.push_back(argument2);

        int16_t a = 1.0;
        int16_t b = 0.0;
        int16_t c = 0.0;
        int16_t d = 1.0;

        if (weHaveScale) { // WE_HAVE_A_SCALE
            int16_t scale = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
            a = scale;
            d = scale;
            pos += 2;
        } else if (weHaveXYScale) { // WE_HAVE_AN_X_AND_Y_SCALE
            a = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
            pos += 2;
            d = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
            pos += 2;
        } else if (weHaveTwoByTwo) { // WE_HAVE_A_TWO_BY_TWO
            a = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
            pos += 2;
            b = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
            pos += 2;
            c = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
            pos += 2;
            d = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
            pos += 2;
        }

        as.push_back(a);
        bs.push_back(b);
        cs.push_back(c);
        ds.push_back(d);

        int32_t m0 = max(abs(a), abs(d)); // Changed b to d
        int32_t n0 = max(abs(c), abs(d));
        int32_t m;
        int32_t n;
        if ((abs(a) - abs(c)) <= 33 / 65536) {
            m = 2 * m0;
        } else {
            m = m0;
        }
        if ((abs(b) - abs(d)) <= 33 / 65536) {
            n = 2 * n0;
        } else {
            n = n0;
        }

        ms.push_back(m);
        ns.push_back(n);
    }

    int16_t numberOfContours = 0;
    std::vector<uint16_t> endPtsOfContours;
    uint16_t instructionLength = 0;
    std::vector<uint8_t> instructions;
    std::vector<uint8_t> flags;
    std::vector<int16_t> xCoordinatesPush;
    std::vector<int16_t> yCoordinatesPush;
    vector<uint32_t> locas = TTFFile::parse(data).getLocas();
    int contourOffset = xCoordinatesPush.size();
    for (int i = 0; i < glyphIndexs.size(); ++i) {
        Glyph glyph = Glyph::parseGlyph(data, TTFFile::parse(data).getGlyfOffset() + locas[glyphIndexs[i]]);
        numberOfContours += glyph.getNumberOfContours();
        for (uint16_t endPtsOfContour : glyph.getEndPtsOfContours()) {
            endPtsOfContours.push_back(endPtsOfContour + contourOffset);
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
        for (int j = 0; j < xCoordinates.size(); ++j) {
            int16_t xPrime = as[i] * xCoordinates[j] + cs[i] * yCoordinates[j] + argument1s[i];
            int16_t yPrime = bs[i] * xCoordinates[j] + ds[i] * yCoordinates[j] + argument2s[i];
            xCoordinatesPush.push_back(xPrime);
            yCoordinatesPush.push_back(yPrime);
            }
        contourOffset += xCoordinatesPush.size();
    }

    return Glyph(numberOfContours, xMin, yMin, xMax, yMax, endPtsOfContours, instructionLength, instructions, flags, xCoordinatesPush, yCoordinatesPush);
}


    Glyph Glyph::parseGlyph(const vector<char>& data, uint32_t offset) {
    uint32_t pos = offset;
    int16_t numberOfContours = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
    pos += 2;
    int16_t xMin = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
    pos += 2;
    int16_t yMin = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
    pos += 2;
    int16_t xMax = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
    pos += 2;
    int16_t yMax = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
    pos += 2;

    if (numberOfContours >= 0) {
        return Glyph::parseSimpleGlyph(data, pos, numberOfContours, xMin, yMin, xMax, yMax);
    } else {
        return Glyph::parseCompoundGlyph(data, pos, xMin, yMin, xMax, yMax);
    }

}




uint32_t Glyph::convertEndian32(uint32_t value) {
    return ((value >> 24) & 0x000000FF) |
           ((value >> 8)  & 0x0000FF00) |
           ((value << 8)  & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
}

uint16_t Glyph::convertEndian16(uint16_t value) {
    return ((value >> 8) & 0x00FF) |
           ((value << 8)  & 0xFF00);
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

void Glyph::drawSimpleGlyph(SDL_Renderer* renderer, Glyph glyph, int xOffset, int yOffset, double scalingFactor, int thickness, int screenHeight) {
    glyph.addPointsBetween();
    vector<uint16_t> endpoints = glyph.getEndPtsOfContours();

    int currentContour = 0;
    int contourStartIndex = 0;

    vector<int16_t> xCoordinates = glyph.getXCoordinates();
    vector<int16_t> yCoordinates = glyph.getYCoordinates();

    vector<uint8_t> flags = glyph.getFlags();
    for (int j = 0; j < xCoordinates.size(); ++j) {
        uint8_t flag = flags[j];
        if (j > endpoints[currentContour]) {
            contourStartIndex = endpoints[currentContour] + 1;
            ++currentContour;
        }
        if (flag & 1) { // If the current point is an on-curve point
            // Adjust the y-coordinates based on screen height and scaling factor
            SDL_Point point1 = { 
                static_cast<int>(xCoordinates[j] * scalingFactor + xOffset), 
                static_cast<int>(screenHeight - (yCoordinates[j] * scalingFactor) + yOffset) 
            };
            SDL_Point controlPoint;
            SDL_Point point2;
            if (j != endpoints[currentContour] - 1) {
                controlPoint.x = static_cast<int>(xCoordinates[j + 1] * scalingFactor + xOffset);
                controlPoint.y = static_cast<int>(screenHeight - (yCoordinates[j + 1] * scalingFactor) + yOffset);
                point2.x = static_cast<int>(xCoordinates[j + 2] * scalingFactor + xOffset);
                point2.y = static_cast<int>(screenHeight - (yCoordinates[j + 2] * scalingFactor) + yOffset);
            } else {
                controlPoint.x = static_cast<int>(xCoordinates[j + 1] * scalingFactor + xOffset);
                controlPoint.y = static_cast<int>(screenHeight - (yCoordinates[j + 1] * scalingFactor) + yOffset);
                point2.x = static_cast<int>(xCoordinates[contourStartIndex] * scalingFactor + xOffset);
                point2.y = static_cast<int>(screenHeight - (yCoordinates[contourStartIndex] * scalingFactor) + yOffset);
            }
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            DrawBezier(renderer, point1, controlPoint, point2, thickness);
        }
    }
}
