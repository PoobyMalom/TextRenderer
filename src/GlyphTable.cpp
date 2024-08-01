#include "GlyphTable.h"
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

Glyph Glyph::parseGlyph(const vector<char>& data, uint16_t offset) {
    uint16_t pos = offset;
    std::cout << "offset in function: " << pos << std::endl;
    int16_t numberOfContours = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
    std::cout << "number of contours in method: " << numberOfContours << std::endl;
    pos += 2;
    int16_t xMin = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
    pos += 2;
    int16_t yMin = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
    pos += 2;
    int16_t xMax = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
    pos += 2;
    int16_t yMax = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
    pos += 2;

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
        std::cout << "x[" << i << "]: " << currentX << std::endl; // Debug print
    }

    vector<int16_t> yCoordinates;
    int16_t currentY = 0;
    for (int i = 0; i < totalPoints; ++i) {
        if (flags[i] & 4) { // Short vector
            if (flags[i] & 32) { // Short vector with positive values
                uint8_t delta = static_cast<uint8_t>(data[pos++]);
                currentY += delta;
            } else { // Short vector with negative values
                int8_t delta = static_cast<int8_t>(data[pos++]);
                currentY -= delta;
            }
        } else if (!(flags[i] & 32)) { // Long vector
            int16_t delta = Glyph::convertEndian16(*reinterpret_cast<const int16_t*>(&data[pos]));
            currentY += delta;
            pos += 2;
        }
        yCoordinates.push_back(currentY);
        std::cout << "y[" << i << "]: " << currentY << std::endl; // Debug print
    }

    return Glyph(numberOfContours, xMin, yMin, xMax, yMax, endPtsOfContours, instructionLength, instructions, flags, xCoordinates, yCoordinates);
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

