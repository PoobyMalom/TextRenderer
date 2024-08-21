#ifndef HELPERS_H
#define HELPERS_H

#include <SDL.h>
#include <vector>
#include <string>
#include <tuple>
#include "MovableLine.h"

using namespace std;

void DrawBezier(SDL_Renderer* renderer, const SDL_Point point1, const SDL_Point controlPoint, const SDL_Point point2);
vector<uint32_t> stringToUnicode(const string& input);

SDL_Point getBezierPoint(const SDL_Point point1, const SDL_Point controlPoint, const SDL_Point point3, float t);
uint16_t convertEndian16(uint16_t value);

uint32_t convertEndian32(uint32_t value);
uint64_t convertEndian64(uint64_t value);

uint8_t readByte(const vector<char>& data, int& offset);
uint16_t read2Bytes(const vector<char>& data, int& offset);
uint32_t read4Bytes(const vector<char>& data, int& offset);
uint64_t read8Bytes(const vector<char>& data, int& offset);

struct Ray {
    int x1;
    int y1;
    int x2;
    int y2;
};

#endif // HELPERS_H
