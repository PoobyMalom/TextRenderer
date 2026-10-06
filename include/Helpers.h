#pragma once

#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include <tuple>
#include "FontTypes.h"

void DrawBezier(SDL_Renderer* renderer, const SDL_Point point1, const SDL_Point controlPoint, const SDL_Point point2);
// Draws the same curve as DrawBezier, but as a dashed line: dashLength pixels
// drawn, then gapLength pixels skipped, repeating. dashPhase is the position
// within the dash+gap pattern (start it at 0); pass the same variable for every
// segment of a glyph so the pattern continues across segment boundaries.
void DrawBezierDashed(SDL_Renderer* renderer, const SDL_Point point1, const SDL_Point controlPoint, const SDL_Point point2,
                      float dashLength, float gapLength, float& dashPhase);
double signedArea(const std::vector<int16_t>& xs, const std::vector<int16_t>& ys, size_t start, size_t end);
std::vector<int> calculateWindingDirections(const std::vector<uint16_t>& endPtsOfContours, const std::vector<int16_t>& xCoordinates, const std::vector<int16_t>& yCoordinates);
std::vector<uint32_t> stringToUnicode(const std::string& input);
uint32_t findUnicodevector(const std::vector<uint32_t>& startCharCodes, const std::vector<uint32_t>& endCharCodes, const std::vector<uint32_t>& startGlyphCodes, uint16_t value);

SDL_Point getBezierPoint(const SDL_Point point1, const SDL_Point controlPoint, const SDL_Point point3, float t);

uint16_t convertEndian16(uint16_t value);
uint32_t convertEndian32(uint32_t value);
uint64_t convertEndian64(uint64_t value);

uint8_t readByte(const std::vector<char>& data, int& offset);
uint16_t read2Bytes(const std::vector<char>& data, int& offset);
uint32_t read4Bytes(const std::vector<char>& data, int& offset);
uint64_t read8Bytes(const std::vector<char>& data, int& offset);

int16_t readS16(const std::vector<char>& data, int& offset);

uint32_t calcTableChecksum(const std::vector<char>& data, uint32_t offset, uint32_t length);
uint32_t calculateHeadChecksum(const std::vector<char>& data, uint32_t headOffset, uint32_t length);


struct Ray {
    int x1;
    int y1;
    int x2;
    int y2;
};
