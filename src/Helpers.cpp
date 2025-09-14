#include "Helpers.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>

void DrawBezier(SDL_Renderer* renderer, const SDL_Point point1, const SDL_Point controlPoint, const SDL_Point point2) {
    // Check if the control point is collinear with the start and end points
    auto isCollinear = [](const SDL_Point& p0, const SDL_Point& p1, const SDL_Point& p2) {
        return std::abs((p2.y - p0.y) * (p1.x - p0.x) - (p1.y - p0.y) * (p2.x - p0.x)) < 1e-5;
    };

    if (isCollinear(point1, controlPoint, point2)) {
        // Draw a straight line if the points are collinear
        SDL_RenderDrawLine(renderer, point1.x, point1.y, point2.x, point2.y);
        return;
    }

    int numPoints = 20; // Increase the number of points for a smoother curve
    float t = 0.0;
    float step = 1.0f / numPoints;
    std::vector<SDL_Point> points;

    for (int i = 0; i <= numPoints; i++) {
        float x = ((1 - t) * (1 - t)) * point1.x + 2 * (1 - t) * t * controlPoint.x + (t * t) * point2.x;
        float y = ((1 - t) * (1 - t)) * point1.y + 2 * (1 - t) * t * controlPoint.y + (t * t) * point2.y;
        SDL_Point point = { static_cast<int>(x), static_cast<int>(y) };
        points.push_back(point);
        t += step;
    }

    // Line simplification: remove consecutive points that are too close to each other
    std::vector<SDL_Point> simplifiedPoints;
    const int distanceThreshold = 1; // Distance threshold for simplification
    simplifiedPoints.push_back(points.front());

    for (size_t i = 1; i < points.size(); ++i) {
        if (std::abs(points[i].x - simplifiedPoints.back().x) > distanceThreshold ||
            std::abs(points[i].y - simplifiedPoints.back().y) > distanceThreshold) {
            simplifiedPoints.push_back(points[i]);
        }
    }

    // Draw the simplified points
    for (size_t i = 1; i < simplifiedPoints.size(); ++i) {
        SDL_RenderDrawLine(renderer, simplifiedPoints[i - 1].x, simplifiedPoints[i - 1].y, simplifiedPoints[i].x, simplifiedPoints[i].y);
    }
}

void drawCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius) {
    for (int w = -radius; w <= radius; w++) {
        for (int h = -radius; h <= radius; h++) {
            if (w*w + h*h <= radius*radius) {
                SDL_RenderDrawPoint(renderer, centerX + w, centerY + h);
            }
        }
    }
}

vector<uint32_t> stringToUnicode(const string& input) {
    vector<uint32_t> unicodePoints;
    size_t length = input.size();

    // Iterate over the string
    for (size_t i = 0; i < length;) {
        uint32_t codePoint = 0;
        unsigned char c = input[i];

        // Determine the number of bytes in the UTF-8 character
        if (c <= 0x7F) { // 1-byte character (ASCII)
            codePoint = c;
            i += 1;
        } else if (c <= 0xDF) { // 2-byte character
            codePoint = ((c & 0x1F) << 6) | (input[i + 1] & 0x3F);
            i += 2;
        } else if (c <= 0xEF) { // 3-byte character
            codePoint = ((c & 0x0F) << 12) | ((input[i + 1] & 0x3F) << 6) | (input[i + 2] & 0x3F);
            i += 3;
        } else if (c <= 0xF7) { // 4-byte character
            codePoint = ((c & 0x07) << 18) | ((input[i + 1] & 0x3F) << 12) | ((input[i + 2] & 0x3F) << 6) | (input[i + 3] & 0x3F);
            i += 4;
        } else {
            // Invalid UTF-8 byte sequence, handle error if needed
            throw runtime_error("Invalid UTF-8 sequence");
        }

        unicodePoints.push_back(codePoint);
    }

    return unicodePoints;
}

string hexToAscii(uint32_t value) {
    /*
    Convert from hex string to ASCII character
    */
    std::stringstream ss;
    ss << hex << uppercase << setw(8) << setfill('0') << value;
    string hexString = ss.str();
    string ascii;
    for (size_t i = 0; i < hexString.length(); i += 2) {
        // Convert each pair of hex digits to an integer
        string byteString = hexString.substr(i, 2);
        char byte = static_cast<char>(stoi(byteString, nullptr, 16));
        ascii.push_back(byte);
    }
    return ascii;
}

uint32_t findUnicodevector(vector<uint32_t> startCharCodes, vector<uint32_t> endCharCodes, vector<uint32_t> startGlyphCodes, uint16_t value) {
    for (int i = 0; i < static_cast<int>(startCharCodes.size()); ++i) {
        if (value >= startCharCodes[i] && value <= endCharCodes[i]) {
            cout << "startCharCode: " << startCharCodes[i] << " | endCharCode: " << endCharCodes[i] << " | startGlyphCode: " << startGlyphCodes[i] << endl;
            return startGlyphCodes[i] + (value - startCharCodes[i]);
        }
    }
    return 0;
}

SDL_Point getBezierPoint(const SDL_Point point1, const SDL_Point controlPoint, const SDL_Point point3, float t) {
    SDL_Point temp;
    temp.x = ((1 - t) * (1 - t)) * point1.x + 2 * (1 - t) * t * controlPoint.x + (t * t) * point3.x;
    temp.y = ((1 - t) * (1 - t)) * point1.y + 2 * (1 - t) * t * controlPoint.y + (t * t) * point3.y;
    return temp;
}

uint8_t convertEndian8(uint8_t value) {
    return (value >> 4) | (value << 4);
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

// Sum a table exactly as per TTF spec (big-endian words, zero-padded to 4 bytes)
uint32_t CalcTableChecksum(const std::vector<char>& data, uint32_t offset, uint32_t length) {
    if (offset > data.size() || length > data.size() - offset) {
        throw std::out_of_range("Table range out of bounds");
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.data()) + offset;

    uint32_t sum = 0;
    uint32_t i = 0;

    // Sum full 4-byte big-endian words
    for (; i + 4 <= length; i += 4) {
        uint32_t w = (uint32_t(bytes[i])   << 24) |
                     (uint32_t(bytes[i+1]) << 16) |
                     (uint32_t(bytes[i+2]) <<  8) |
                     (uint32_t(bytes[i+3])      );
        sum += w;  // wraps modulo 2^32
    }

    // Trailing 1–3 bytes, zero-padded on the right (low bytes)
    if (i < length) {
        uint32_t w = 0;
        uint32_t rem = length - i;
        w |= uint32_t(bytes[i]) << 24;
        if (rem >= 2) w |= uint32_t(bytes[i+1]) << 16;
        if (rem >= 3) w |= uint32_t(bytes[i+2]) <<  8;
        sum += w;
    }

    return sum;
}

uint32_t calculateHeadChecksum(const std::vector<char>& data, uint32_t headOffset, uint32_t headLength)
{
    // build checksum with adjustment zeroed
    if (headLength < 12) return false;

    std::vector<uint8_t> copy(headLength);
    std::memcpy(copy.data(),
                reinterpret_cast<const uint8_t*>(data.data()) + headOffset,
                headLength);
    copy[8] = copy[9] = copy[10] = copy[11] = 0;

    auto sum_be_words = [](const uint8_t* b, uint32_t n)->uint32_t {
        uint32_t s = 0, i = 0;
        for (; i + 4 <= n; i += 4)
            s += (uint32_t(b[i])<<24)|(uint32_t(b[i+1])<<16)|
                 (uint32_t(b[i+2])<<8)|uint32_t(b[i+3]);
        if (i < n) {
            uint32_t w = 0, r = n - i;
            w |= uint32_t(b[i]) << 24;
            if (r >= 2) w |= uint32_t(b[i+1]) << 16;
            if (r >= 3) w |= uint32_t(b[i+2]) << 8;
            s += w;
        }
        return s;
    };

    uint32_t calc = sum_be_words(copy.data(), headLength);
    return calc;
}