#include "Helpers.h"
#include "MovablePoint.h"
#include <fstream>
#include <iostream>
#include <vector>

void DrawLine(SDL_Renderer* renderer, const MovablePoint& point1, const MovablePoint& point2) {
    SDL_RenderDrawLine(renderer, static_cast<int>(point1.getX()), static_cast<int>(point1.getY()), 
                       static_cast<int>(point2.getX()), static_cast<int>(point2.getY()));
}

void MovingPoint(SDL_Renderer* renderer, const MovablePoint& point1, const MovablePoint& point2, float t) {
    float x = (1 - t) * point1.getX() + t * point2.getX();
    float y = (1 - t) * point1.getY() + t * point2.getY();
    SDL_Rect rect = { static_cast<int>(x), static_cast<int>(y), 10, 10 };
    SDL_RenderDrawRect(renderer, &rect);
}

void DrawBezier(SDL_Renderer* renderer, const MovablePoint& point1, const MovablePoint& controlPoint, const MovablePoint& point2) {
    float t = 0.0;
    for (int i = 0; i < 10000; i++) {
        float x = ((1 - t)*(1-t)) * point1.getX() + 2 * (1 - t) * t * controlPoint.getX() + (t*t) * point2.getX();
        float y = ((1 - t)*(1-t)) * point1.getY() + 2 * (1 - t) * t * controlPoint.getY() + (t*t) * point2.getY();
        SDL_Point point = { static_cast<int>(x), static_cast<int>(y) };
        SDL_RenderDrawPoint(renderer, point.x, point.y);
        t += 0.0001;
    } 
}

std::vector<char> readTTFFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }
   std::ifstream::pos_type pos = file.tellg();
    std::vector<char> buffer(pos);
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), pos);
    file.close();
    return buffer;
}

// Helper function to read a big-endian 32-bit integer
uint32_t readUint32(std::ifstream& file) {
    uint32_t value;
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return __builtin_bswap32(value); // Convert from big-endian to host-endian
}