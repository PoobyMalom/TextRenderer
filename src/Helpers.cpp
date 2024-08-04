#include "Helpers.h"
#include "MovablePoint.h"
#include <fstream>
#include <iostream>
#include <vector>

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

    int numPoints = 28; // Increase the number of points for a smoother curve
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

std::vector<uint32_t> stringToUnicode(const std::string& input) {
    std::vector<uint32_t> unicodePoints;
    size_t length = input.size();

    // Iterate over the string
    for (size_t i = 0; i < length;) {
        std::cout << "letter " << input[i] << std::endl;
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
            throw std::runtime_error("Invalid UTF-8 sequence");
        }

        unicodePoints.push_back(codePoint);
    }

    return unicodePoints;
}