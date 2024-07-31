#ifndef HELPERS_H
#define HELPERS_H

#include <SDL.h>
#include <vector>
#include <string>
#include "MovablePoint.h"

// Function to draw a line between two MovablePoints
void DrawLine(SDL_Renderer* renderer, const MovablePoint& point1, const MovablePoint& point2);
void MovingPoint(SDL_Renderer* renderer, const MovablePoint& point1, const MovablePoint& point2, float t);
void DrawBezier(SDL_Renderer* renderer, const MovablePoint& point1, const MovablePoint& controlPoint, const MovablePoint& point2);
std::vector<char> readTTFFile(const std::string& filePath);
uint32_t readUint32(std::ifstream& file);

#endif // HELPERS_H
