#include "Helpers.h"
#include <iostream>

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

SDL_Point getBezierPoint(const SDL_Point point1, const SDL_Point controlPoint, const SDL_Point point3, float t) {
    SDL_Point temp;
    temp.x = ((1 - t) * (1 - t)) * point1.x + 2 * (1 - t) * t * controlPoint.x + (t * t) * point3.x;
    temp.y = ((1 - t) * (1 - t)) * point1.y + 2 * (1 - t) * t * controlPoint.y + (t * t) * point3.y;
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