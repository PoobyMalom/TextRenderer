#include "GlyphTable.h"
#include "Helpers.h"
#include "TTFHeader.h"
#include "TTFFile.h"
#include "MovableLine.h"
#include "GeometryUtils.h"
#include <vector>
#include <tuple>
#include <iostream>
#include <fstream>
#include <sstream>
#include <bitset>
#include <map>
using namespace std;

GlyphPoint::GlyphPoint(
    int16_t x,
    int16_t y,
    uint8_t flag
) : x(x),
    y(y),
    flag(flag) {}

Contour::Contour(
    vector<GlyphPoint> points,
    int parent,
    vector<int> children,
    double signedArea
) : points(points),
    parent(parent),
    children(children),
    signedArea(signedArea) {}

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
    vector<int16_t> yCoordinates,
    vector<Contour> contours
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
    yCoordinates(yCoordinates),
    contours(contours) {}

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
uint16_t Glyph::getGID() const { return gid; };

vector<Contour> generateContours(vector<uint8_t> flags, vector<int16_t> xCoordinates, vector<int16_t> yCoordinates, vector<uint16_t> endPtsOfContours) {
    int currentContour = 0;
    int contourStartIndex = 0;
    
    vector<Contour> finalContours;
    vector<GlyphPoint> points;
    int pointsLength = size(xCoordinates);
    for (int i = 0; i < pointsLength; i++) {
        points.push_back(GlyphPoint(xCoordinates[i], yCoordinates[i], flags[i]));
        if (i > endPtsOfContours[currentContour]) {
            contourStartIndex = endPtsOfContours[currentContour] + 1;
            ++currentContour;
            finalContours.push_back(Contour(points, 0, vector<int>(), 0));
            points.clear();
        }
    }

    return finalContours;
}

Glyph Glyph::parseSimpleGlyph(const vector<char>& data, uint32_t offset, int16_t numberOfContours, int16_t xMin, int16_t yMin, int16_t xMax, int16_t yMax) {
    int pos = offset;

    vector<uint16_t> endPtsOfContours;
    for (int i = 0; i < numberOfContours; ++i) { endPtsOfContours.push_back(read2Bytes(data, pos)); }

    uint16_t instructionLength = read2Bytes(data, pos);
    vector<uint8_t> instructions(instructionLength);
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
            currentX += read2Bytes(data, pos);
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
            currentY += read2Bytes(data, pos);
        }
        yCoordinates.push_back(currentY);
    }

    vector<Contour> contours = generateContours(flags, xCoordinates, yCoordinates, endPtsOfContours);


    return Glyph(numberOfContours, xMin, yMin, xMax, yMax, endPtsOfContours, instructionLength, instructions, flags, xCoordinates, yCoordinates, contours);
}

Glyph Glyph::parseCompoundGlyph(const vector<char>& data, uint32_t offset, int16_t xMin, int16_t yMin, int16_t xMax, int16_t yMax) {
    int pos = offset;
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

        int16_t a = 1.0;
        int16_t b = 0.0;
        int16_t c = 0.0;
        int16_t d = 1.0;

        if (weHaveScale) { // WE_HAVE_A_SCALE
            int16_t scale = read2Bytes(data, pos);
            a = scale;
            d = scale;
        } else if (weHaveXYScale) { // WE_HAVE_AN_X_AND_Y_SCALE
            a = read2Bytes(data, pos);
            d = read2Bytes(data, pos);
        } else if (weHaveTwoByTwo) { // WE_HAVE_A_TWO_BY_TWO
            a = read2Bytes(data, pos);
            b = read2Bytes(data, pos);
            c = read2Bytes(data, pos);
            d = read2Bytes(data, pos);
        }

        as.push_back(a);
        bs.push_back(b);
        cs.push_back(c);
        ds.push_back(d);

        int32_t m0 = max(abs(a), abs(d));
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
    vector<uint16_t> endPtsOfContours;
    uint16_t instructionLength = 0;
    vector<uint8_t> instructions;
    vector<uint8_t> flags;
    vector<int16_t> xCoordinatesPush;
    vector<int16_t> yCoordinatesPush;
    vector<uint32_t> locas = TTFFile::parse(data).getLocas();
    int contourOffset = xCoordinatesPush.size();
    for (u_long i = 0; i < glyphIndexs.size(); ++i) {
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
        for (u_long j = 0; j < xCoordinates.size(); ++j) {
            int16_t xPrime = as[i] * xCoordinates[j] + cs[i] * yCoordinates[j] + argument1s[i];
            int16_t yPrime = bs[i] * xCoordinates[j] + ds[i] * yCoordinates[j] + argument2s[i];
            xCoordinatesPush.push_back(xPrime);
            yCoordinatesPush.push_back(yPrime);
            }
        contourOffset += xCoordinatesPush.size();
    }

    return Glyph(numberOfContours, xMin, yMin, xMax, yMax, endPtsOfContours, instructionLength, instructions, flags, xCoordinatesPush, yCoordinatesPush, vector<Contour>());
}


    Glyph Glyph::parseGlyph(const vector<char>& data, uint32_t offset) {
    int pos = offset;
    int16_t numberOfContours = read2Bytes(data, pos);
    printf("Number of contours: %u\n", numberOfContours);
    int16_t xMin = read2Bytes(data, pos);
    int16_t yMin = read2Bytes(data, pos);
    int16_t xMax = read2Bytes(data, pos);
    int16_t yMax = read2Bytes(data, pos);
    
    if (numberOfContours >= 0) {
        puts("Simple Glyph");
        return Glyph::parseSimpleGlyph(data, pos, numberOfContours, xMin, yMin, xMax, yMax);
    } else {
        puts("Compound Glyph");
        return Glyph::parseCompoundGlyph(data, pos, xMin, yMin, xMax, yMax);
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
        // Dot Placement Debugging
        // if (flag & 1) {
        //     SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        // }
        // drawCircle(renderer, xCoordinates[j] * scalingFactor + xOffset, screenHeight - (yCoordinates[j] * scalingFactor) + yOffset, 2);
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
            if (static_cast<uint16_t>(j) != endpoints[currentContour] - 1) {
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
            DrawBezier(renderer, point1, controlPoint, point2);
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        }
    }
}

void Glyph::printGlyph(bool printEndPoints, bool printInstructions, bool printCoords) {
    cout << "----------------------------------------------------------------------" << endl;
    cout << "Number of contours: " << numberOfContours << endl;
    cout << "xMin: " << xMin << ", yMin: " << yMin << ", xMax: " << xMax << ", yMax: " << yMax << endl;
    cout << "Instruction Length: " << instructionLength << endl;

    if (printEndPoints) {
        cout << "Endpoints of contours: ";
        for (uint16_t endpt : endPtsOfContours) {
            cout << endpt << ", ";
        } cout << endl;
    }

    if (printInstructions) {
        cout << "Instructions: ";
        for (uint8_t inst : instructions) {
            cout << static_cast<int>(inst) << ", ";
        } cout << endl;
    }

    if (printCoords) {
        for (uint16_t i = 0; i < xCoordinates.size(); i++) {
            cout << "Point " << i << " X: " << xCoordinates[i] << ", Y: " << yCoordinates[i] << ", Flags: " << bitset<8>(flags[i]) << endl;
        }
    }
}

std::vector<GlyphName> readAdobeGlyphList(const std::string& filename) {
    std::vector<GlyphName> glyphNames;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return glyphNames;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string indexStr, name;
        
        // Read index and name separated by tab or space
        if (ss >> indexStr >> name) {
            try {
                uint16_t index = static_cast<uint16_t>(std::stoi(indexStr));
                glyphNames.emplace_back(index, name);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing line: " << line << std::endl;
            }
        }
    }
    
    file.close();
    std::cout << "Loaded " << glyphNames.size() << " glyph names" << std::endl;
    return glyphNames;
}

void loadStandardGlyphNamesMap(const std::string& filename, map<uint16_t, string>& standardGlyphMap) {
    auto glyphNames = readAdobeGlyphList(filename);
    for (const auto& glyph : glyphNames) {
        standardGlyphMap[glyph.index] = glyph.name;
    }
}

string getStandardGlyphNameFast(uint16_t index, map<uint16_t, string>& standardGlyphMap) {
    auto it = standardGlyphMap.find(index);
    if (it != standardGlyphMap.end()) {
        return it->second;
    }
    return ".notdef";
}

vector<Line> Glyph::getGlyphSegments(int xOffset, int yOffset,
                                     double scalingFactor,
                                     int screenHeight)
{
    std::vector<Line> segs;
    
    const std::vector<uint16_t> endpoints = endPtsOfContours; // last index of each contour
    const std::vector<int16_t>  xs        = xCoordinates;
    const std::vector<int16_t>  ys        = yCoordinates;
    const std::vector<uint8_t>  gflags     = flags;
    cout << "Here 1" << endl;
    auto toScreen = [&](int idx) {
        float sx = static_cast<float>(xs[idx]) * static_cast<float>(scalingFactor) + static_cast<float>(xOffset);
        float sy = static_cast<float>(screenHeight) - static_cast<float>(ys[idx]) * static_cast<float>(scalingFactor) + static_cast<float>(yOffset);
        return std::pair<float,float>(sx, sy);
    };

    auto midpoint = [&](float xA, float yA, float xB, float yB) {
        return std::pair<float,float>((xA + xB) * 0.5f, (yA + yB) * 0.5f);
    };

    int contourStart = 0;
    for (size_t c = 0; c < endpoints.size(); ++c) {
        const int contourEnd = static_cast<int>(endpoints[c]);   // inclusive
        const int n          = contourEnd - contourStart + 1;
        if (n <= 0) { contourStart = contourEnd + 1; continue; }

        // Cache this contour's points in screen space and on-curve flags
        std::vector<float> px(n), py(n);
        std::vector<bool>  on(n);
        for (int i = 0; i < n; ++i) {
            const int idx = contourStart + i;
            auto [sx, sy] = toScreen(idx);
            px[i] = sx; py[i] = sy;
            on[i] = (gflags[idx] & 0x01) != 0;  // TrueType: bit0 = on-curve
        }

        // Determine starting "previous on-curve" point:
        // If first is on-curve, start there.
        // Else if last is on-curve, start at last.
        // Else start at implicit on-curve midpoint between last and first off-curve points.
        float prevX, prevY;
        int iStart = 0;
        if (on[0]) {
            prevX = px[0]; prevY = py[0];
            iStart = 1;
        } else if (on[n - 1]) {
            prevX = px[n - 1]; prevY = py[n - 1];
            iStart = 0;
        } else {
            auto [mx, my] = midpoint(px[n - 1], py[n - 1], px[0], py[0]);
            prevX = mx; prevY = my;
            iStart = 0;
        }

        // Walk the contour once, adding segments between successive on-curve endpoints.
        int i = iStart;
        while (i < n) {
            if (on[i]) {
                // Explicit on-curve → segment from previous on-curve to this on-curve
                segs.push_back(Line{prevX, prevY, px[i], py[i]});
                prevX = px[i]; prevY = py[i];
                ++i;
            } else {
                // Off-curve (quadratic control). Look at the next vertex (wrap to 0).
                int ni = (i + 1) % n;
                if (on[ni]) {
                    // Off-curve followed by on-curve:
                    // Curve segment endpoints are (prevOn) → (next on-curve).
                    segs.push_back(Line{prevX, prevY, px[ni], py[ni]});
                    prevX = px[ni]; prevY = py[ni];
                    i += 2; // we consumed the next point
                } else {
                    // Two consecutive off-curve points:
                    // Insert implicit on-curve at their midpoint.
                    auto [mx, my] = midpoint(px[i], py[i], px[ni], py[ni]);
                    segs.push_back(Line{prevX, prevY, mx, my});
                    prevX = mx; prevY = my;
                    ++i; // advance by one; next iter will consider (off,off) pair again
                }
            }
        }

        contourStart = contourEnd + 1;
    }

    return segs;
}
