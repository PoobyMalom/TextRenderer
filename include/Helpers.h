#ifndef HELPERS_H
#define HELPERS_H

#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include <tuple>
#include "MovableLine.h"

using namespace std;

struct Line {
    float ax, ay, bx, by;
};

void DrawBezier(SDL_Renderer* renderer, const SDL_Point point1, const SDL_Point controlPoint, const SDL_Point point2);
void drawCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius);
vector<uint32_t> stringToUnicode(const string& input);
string hexToAscii(uint32_t value);
uint32_t findUnicodevector(vector<uint32_t> startCharCodes, vector<uint32_t> endCharCodes, vector<uint32_t> startGlyphCodes, uint16_t value);

SDL_Point getBezierPoint(const SDL_Point point1, const SDL_Point controlPoint, const SDL_Point point3, float t);

uint8_t  convertEndian8(uint8_t value);
uint16_t convertEndian16(uint16_t value);
uint32_t convertEndian32(uint32_t value);
uint64_t convertEndian64(uint64_t value);

uint8_t readByte(const vector<char>& data, int& offset);
uint16_t read2Bytes(const vector<char>& data, int& offset);
uint32_t read4Bytes(const vector<char>& data, int& offset);
uint64_t read8Bytes(const vector<char>& data, int& offset);

string readPascalString(const vector<char>& data, int& offset);

uint32_t CalcTableChecksum(const std::vector<char>& data, uint32_t offset, uint32_t length);
uint32_t calculateHeadChecksum(const std::vector<char>& data, uint32_t headOffset, uint32_t length);
vector<Line> normalizeToNDC(const std::vector<Line>& segs, float margin = 0.05f, bool flipY = false);

struct Ray {
    int x1;
    int y1;
    int x2;
    int y2;
};


#endif // HELPERS_H
