#include "Helpers.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <array>
#include <cmath>
#include <algorithm>

using namespace std;

void DrawBezier(SDL_Renderer* renderer, const SDL_Point point1, const SDL_Point controlPoint, const SDL_Point point2) {
    // Check if the control point is collinear with the start and end points
    auto isCollinear = [](const SDL_Point& p0, const SDL_Point& p1, const SDL_Point& p2) { // NOLINT(readability-identifier-length)
        return std::abs(((p2.y - p0.y) * (p1.x - p0.x)) - ((p1.y - p0.y) * (p2.x - p0.x))) < 1e-5;
    };

    if (isCollinear(point1, controlPoint, point2)) {
        // Draw a straight line if the points are collinear
        SDL_RenderLine(renderer, point1.x, point1.y, point2.x, point2.y);
        return;
    }

    int numPoints = 20; // Increase the number of points for a smoother curve
    float t = 0.0; // NOLINT(readability-identifier-length)
    float step = 1.0F / static_cast<float>(numPoints);
    std::array<SDL_Point, 21> points;
    int pointCount = 0;

    for (int i = 0; i <= numPoints; i++) {
        float x = (((1 - t) * (1 - t)) * (float)point1.x) + (2 * (1 - t) * t * (float)controlPoint.x) + ((t * t) * (float)point2.x); // NOLINT(readability-identifier-length)
        float y = (((1 - t) * (1 - t)) * (float)point1.y) + (2 * (1 - t) * t * (float)controlPoint.y) + ((t * t) * (float)point2.y); // NOLINT(readability-identifier-length)
        points[pointCount++] = { static_cast<int>(x), static_cast<int>(y) };
        t += step;
    }

    // Line simplification: remove consecutive points that are too close to each other
    std::array<SDL_Point, 21> simplified;
    int simplifiedCount = 0;
    const int distanceThreshold = 1;
    simplified[simplifiedCount++] = points[0];

    for (int i = 1; i < pointCount; ++i) {
        if (std::abs(points[i].x - simplified[simplifiedCount - 1].x) > distanceThreshold ||
            std::abs(points[i].y - simplified[simplifiedCount - 1].y) > distanceThreshold) {
            simplified[simplifiedCount++] = points[i];
        }
    }

    for (int i = 1; i < simplifiedCount; ++i) {
        SDL_RenderLine(renderer, simplified[i - 1].x, simplified[i - 1].y, simplified[i].x, simplified[i].y);
    }
}

void DrawBezierDashed(SDL_Renderer* renderer, const SDL_Point point1, const SDL_Point controlPoint, const SDL_Point point2,
                      float dashLength, float gapLength, float& dashPhase) {
    const float period = dashLength + gapLength;

    // Flatten the curve finely, then walk the polyline by arc length so dashes
    // have an even pixel length regardless of how the curve is sampled.
    const int numSamples = 64;
    std::array<SDL_FPoint, numSamples + 1> samples;
    for (int i = 0; i <= numSamples; i++) {
        float t = static_cast<float>(i) / static_cast<float>(numSamples); // NOLINT(readability-identifier-length)
        float u = 1.0F - t; // NOLINT(readability-identifier-length)
        samples[i].x = (u * u * static_cast<float>(point1.x)) + (2 * u * t * static_cast<float>(controlPoint.x)) + (t * t * static_cast<float>(point2.x));
        samples[i].y = (u * u * static_cast<float>(point1.y)) + (2 * u * t * static_cast<float>(controlPoint.y)) + (t * t * static_cast<float>(point2.y));
    }

    for (int i = 1; i <= numSamples; ++i) {
        SDL_FPoint from = samples[i - 1];
        SDL_FPoint to   = samples[i];
        float segLength = std::hypot(to.x - from.x, to.y - from.y);
        if (segLength <= 0.0F) {
            continue;
        }

        // Split this sample segment at the points where the pattern flips
        // between "dash" and "gap". dashPhase always stays in [0, period) and
        // is snapped exactly onto each boundary, so every pass makes progress.
        float travelled = 0.0F;
        while (travelled < segLength) {
            bool inDash = dashPhase < dashLength;
            float boundary = inDash ? dashLength : period;
            float untilFlip = boundary - dashPhase;
            float remaining = segLength - travelled;
            bool reachesFlip = untilFlip <= remaining;
            float step = reachesFlip ? untilFlip : remaining;

            if (inDash) {
                float startFrac = travelled / segLength;
                float endFrac   = (travelled + step) / segLength;
                SDL_RenderLine(renderer,
                               from.x + ((to.x - from.x) * startFrac), from.y + ((to.y - from.y) * startFrac),
                               from.x + ((to.x - from.x) * endFrac),   from.y + ((to.y - from.y) * endFrac));
            }

            if (!reachesFlip) {
                dashPhase += step;
                break;
            }
            travelled += step;
            dashPhase = inDash ? dashLength : 0.0F;
        }
    }
}

double signedArea(const std::vector<int16_t>& xCoordinates, const std::vector<int16_t>& yCoordinates,
                  size_t start, size_t end) { // end is the contour's last index
    double sum = 0.0;
    size_t count = end - start + 1;
    for (size_t k = 0; k < count; ++k) {
        size_t cur  = start + k;
        size_t next = start + ((k + 1) % count);
        sum += static_cast<double>(xCoordinates[cur]) * yCoordinates[next] - static_cast<double>(xCoordinates[next]) * yCoordinates[cur];
    }
    return sum / 2.0;
}

vector<int> calculateWindingDirections(const std::vector<uint16_t>& endPtsOfContours, const std::vector<int16_t>& xCoordinates, const std::vector<int16_t>& yCoordinates) {
    vector<int> windingDirections;
    windingDirections.reserve(endPtsOfContours.size());

    size_t start = 0;
    for (uint16_t end : endPtsOfContours) {
        // 1 = clockwise (negative area in y-up font units), 0 = counter-clockwise
        windingDirections.push_back(signedArea(xCoordinates, yCoordinates, start, end) < 0 ? 1 : 0);
        start = static_cast<size_t>(end) + 1;
    }
    return windingDirections;
}

vector<uint32_t> stringToUnicode(const string& input) {
    vector<uint32_t> unicodePoints;
    size_t length = input.size();

    // Iterate over the string
    for (size_t i = 0; i < length;) {
        uint32_t codePoint = 0;
        unsigned char character = input[i];

        // Determine the number of bytes in the UTF-8 character
        if (character <= 0x7F) { // 1-byte character (ASCII)
            codePoint = character;
            i += 1;
        } else if (character <= 0xDF) { // 2-byte character
            codePoint = ((character & 0x1F) << 6) | (input[i + 1] & 0x3F);
            i += 2;
        } else if (character <= 0xEF) { // 3-byte character
            codePoint = ((character & 0x0F) << 12) | ((input[i + 1] & 0x3F) << 6) | (input[i + 2] & 0x3F);
            i += 3;
        } else if (character <= 0xF7) { // 4-byte character
            codePoint = ((character & 0x07) << 18) | ((input[i + 1] & 0x3F) << 12) | ((input[i + 2] & 0x3F) << 6) | (input[i + 3] & 0x3F);
            i += 4;
        } else {
            // Invalid UTF-8 byte sequence, handle error if needed
            throw runtime_error("Invalid UTF-8 sequence");
        }

        unicodePoints.push_back(codePoint);
    }

    return unicodePoints;
}

uint32_t findUnicodevector(const vector<uint32_t>& startCharCodes, const vector<uint32_t>& endCharCodes, const vector<uint32_t>& startGlyphCodes, uint16_t value) {
    for (int i = 0; i < static_cast<int>(startCharCodes.size()); ++i) {
        if (value >= startCharCodes[i] && value <= endCharCodes[i]) {
            cout << "startCharCode: " << startCharCodes[i] << " | endCharCode: " << endCharCodes[i] << " | startGlyphCode: " << startGlyphCodes[i] << '\n';
            return startGlyphCodes[i] + (value - startCharCodes[i]);
        }
    }
    return 0;
}

SDL_Point getBezierPoint(const SDL_Point point1, const SDL_Point controlPoint, const SDL_Point point3, float t) { // NOLINT(readability-identifier-length)
    SDL_Point temp;
    temp.x = (int)((((1 - t) * (1 - t)) * (float)point1.x) + (2 * (1 - t) * t * (float)controlPoint.x) + ((t * t) * (float)point3.x));
    temp.y = (int)((((1 - t) * (1 - t)) * (float)point1.y) + (2 * (1 - t) * t * (float)controlPoint.y) + ((t * t) * (float)point3.y));
    return temp;
}

uint16_t convertEndian16(uint16_t value) {
    return (value >> 8) | (value << 8);
}
uint32_t convertEndian32(uint32_t value) {
    return ((value >> 24) & 0x000000FF) |
           ((value >> 8)  & 0x0000FF00) |
           ((value << 8)  & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
}
uint64_t convertEndian64(uint64_t value) {
    return ((value >> 56) & 0x00000000000000FF) |
           ((value >> 40) & 0x000000000000FF00) |
           ((value >> 24) & 0x0000000000FF0000) |
           ((value >> 8)  & 0x00000000FF000000) |
           ((value << 8)  & 0x000000FF00000000) |
           ((value << 24) & 0x0000FF0000000000) |
           ((value << 40) & 0x00FF000000000000) |
           ((value << 56) & 0xFF00000000000000);
}

uint8_t readByte(const vector<char>& data, int& offset) {
    uint8_t value = *reinterpret_cast<const uint8_t*>(&data[offset]);
    offset += 1;
    return value;
};
uint16_t read2Bytes(const vector<char>& data, int& offset) {
    uint16_t value = convertEndian16(*reinterpret_cast<const uint16_t*>(&data[offset]));
    offset += 2;
    return value;
}
uint32_t read4Bytes(const vector<char>& data, int& offset) {
    uint32_t value = convertEndian32(*reinterpret_cast<const uint32_t*>(&data[offset]));
    offset += 4;
    return value;
}
uint64_t read8Bytes(const vector<char>& data, int& offset) {
    uint64_t value = convertEndian64(*reinterpret_cast<const uint64_t*>(&data[offset]));
    offset += 8;
    return value;
}

int16_t readS16(const std::vector<char>& data, int& offset) {
    int16_t value = static_cast<int16_t>(convertEndian16(*reinterpret_cast<const uint16_t*>(&data[offset])));
    offset += 2;
    return value;
}

// Sum a table exactly as per TTF spec (big-endian words, zero-padded to 4 bytes)
uint32_t calcTableChecksum(const std::vector<char>& data, uint32_t offset, uint32_t length) {
    if (offset > data.size() || length > data.size() - offset) {
        throw std::out_of_range("Table range out of bounds");
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.data()) + offset;

    uint32_t sum = 0;
    uint32_t i = 0; // NOLINT(readability-identifier-length)

    // Sum full 4-byte big-endian words
    for (; i + 4 <= length; i += 4) {
        uint32_t width = (uint32_t(bytes[i])   << 24) |
                     (uint32_t(bytes[i+1]) << 16) |
                     (uint32_t(bytes[i+2]) <<  8) |
                     (uint32_t(bytes[i+3])      );
        sum += width;  // wraps modulo 2^32
    }

    // Trailing 1–3 bytes, zero-padded on the right (low bytes)
    if (i < length) {
        uint32_t width = 0;
        uint32_t rem = length - i;
        width |= uint32_t(bytes[i]) << 24;
        if (rem >= 2) {width |= uint32_t(bytes[i+1]) << 16;}
        if (rem >= 3) {width |= uint32_t(bytes[i+2]) <<  8;}
        sum += width;
    }

    return sum;
}

uint32_t calculateHeadChecksum(const std::vector<char>& data, uint32_t headOffset, uint32_t headLength)
{
    // build checksum with adjustment zeroed
    if (headLength < 12) {return 0;}

    std::vector<uint8_t> copy(headLength);
    std::memcpy(copy.data(),
                reinterpret_cast<const uint8_t*>(data.data()) + headOffset,
                headLength);
    copy[8] = copy[9] = copy[10] = copy[11] = 0;

    auto sum_be_words = [](const uint8_t* b, uint32_t n)->uint32_t { // NOLINT(readability-identifier-length)
        uint32_t s = 0; // NOLINT(readability-identifier-length)
        uint32_t i = 0; // NOLINT(readability-identifier-length)
        for (; i + 4 <= n; i += 4) {
            s += (uint32_t(b[i])<<24)|(uint32_t(b[i+1])<<16)|
                 (uint32_t(b[i+2])<<8)|uint32_t(b[i+3]);}
        if (i < n) {
            uint32_t w = 0; // NOLINT(readability-identifier-length)
            uint32_t r = n - i; // NOLINT(readability-identifier-length)
            w |= uint32_t(b[i]) << 24;
            if (r >= 2) {w |= uint32_t(b[i+1]) << 16;}
            if (r >= 3) {w |= uint32_t(b[i+2]) << 8;}
            s += w;
        }
        return s;
    };

    uint32_t calc = sum_be_words(copy.data(), headLength);
    return calc;
}