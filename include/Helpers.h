#ifndef HELPERS_H
#define HELPERS_H

#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include <tuple>
#include "MovableLine.h"

using namespace std;

<<<<<<< Updated upstream
=======
struct Line {
    float ax, ay, bx, by;
};

struct PointNode {
  int16_t x;
  int16_t y;
  bool onCurve;       // from flags[j] & 1, handy later
  PointNode* prev;
  PointNode* next;

  PointNode(int16_t _x, int16_t _y, bool _onCurve)
    : x(_x), y(_y), onCurve(_onCurve), prev(nullptr), next(nullptr) {}
};

struct Triangle {
  int16_t x1, y1;
  int16_t x2, y2;
  int16_t x3, y3;
};

void drawTriangle(SDL_Renderer* renderer, PointNode* p1, PointNode* p2, PointNode* p3, double scalingFactor, int height);
>>>>>>> Stashed changes
void DrawBezier(SDL_Renderer* renderer, const SDL_Point point1, const SDL_Point controlPoint, const SDL_Point point2);
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

uint32_t CalcTableChecksum(const std::vector<char>& data, uint32_t offset, uint32_t length);
uint32_t calculateHeadChecksum(const std::vector<char>& data, uint32_t headOffset, uint32_t length);


struct Ray {
    int x1;
    int y1;
    int x2;
    int y2;
};

#endif // HELPERS_H
