#include "Renderer.h"

using namespace std;

void drawSimpleGlyph(SDL_Renderer* renderer, const Glyph& glyph, int xOffset, int yOffset, double scalingFactor, int screenHeight, int thickness) { // NOLINT(bugprone-easily-swappable-parameters)
    vector<uint16_t> endpoints = glyph.getEndPtsOfContours();

    int currentContour = 0;
    int contourStartIndex = 0;

    vector<int16_t> xCoordinates = glyph.getXCoordinates();
    vector<int16_t> yCoordinates = glyph.getYCoordinates();

    vector<uint8_t> flags = glyph.getFlags();
    for (u_long j = 0; j < xCoordinates.size(); ++j) {
        uint8_t flag = flags[j];
        if (j > endpoints[currentContour]) {
            contourStartIndex = endpoints[currentContour] + 1;
            ++currentContour;
        }
        if ((flag & 1) != 0) { // If the current point is an on-curve point
            auto wrapIdx = [&](size_t base, int offset) -> size_t {
                int len = endpoints[currentContour] - contourStartIndex + 1;
                return contourStartIndex + ((base - contourStartIndex + offset) % len);
            };
            size_t ctrlIdx = wrapIdx(j, 1);
            size_t endIdx = wrapIdx(j, 2);
            SDL_Point point1 = {
                static_cast<int>((xCoordinates[j] * scalingFactor) + xOffset),
                static_cast<int>(screenHeight - (yCoordinates[j] * scalingFactor) + yOffset)
            };
            SDL_Point controlPoint = {
                static_cast<int>((xCoordinates[ctrlIdx] * scalingFactor) + xOffset),
                static_cast<int>(screenHeight - (yCoordinates[ctrlIdx] * scalingFactor) + yOffset)
            };
            SDL_Point point2 = {
                static_cast<int>((xCoordinates[endIdx] * scalingFactor) + xOffset),
                static_cast<int>(screenHeight - (yCoordinates[endIdx] * scalingFactor) + yOffset)
            };
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            DrawBezier(renderer, point1, controlPoint, point2);
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        }
    }
}