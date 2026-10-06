#include "Renderer.h"
#include "cdt/constrain.h"
#include "cdt/draw.h"
#include <algorithm>
#include <iostream>

using namespace std;

namespace {

const float DASH_EM_FRACTION = 1.0F / 160.0F;
const float GAP_EM_FRACTION  = 1.0F / 160.0F;
} // namespace

void drawSimpleGlyph(SDL_Renderer* renderer, const Glyph& glyph, FontTransform& ftrans, int xOffset, int yOffset, int thickness) { // NOLINT(bugprone-easily-swappable-parameters)
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
            size_t nextIdx = wrapIdx(j, 1);
            SDL_Point point1 = {
                static_cast<int>(ftrans.toPixels(xCoordinates[j]) + xOffset),
                ftrans.toScreenY(yCoordinates[j], yOffset)
            };
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            if ((flags[nextIdx] & 1) != 0) {
                // Next point is on-curve too: a straight edge, not a curve.
                SDL_Point point2 = {
                    static_cast<int>(ftrans.toPixels(xCoordinates[nextIdx]) + xOffset),
                    ftrans.toScreenY(yCoordinates[nextIdx], yOffset)
                };
                SDL_RenderLine(renderer, point1.x, point1.y, point2.x, point2.y);
            } else {
                // Next point is off-curve: (on, off, on) is a quadratic curve.
                size_t endIdx = wrapIdx(j, 2);
                SDL_Point controlPoint = {
                    static_cast<int>(ftrans.toPixels(xCoordinates[nextIdx]) + xOffset),
                    ftrans.toScreenY(yCoordinates[nextIdx], yOffset)
                };
                SDL_Point point2 = {
                    static_cast<int>(ftrans.toPixels(xCoordinates[endIdx]) + xOffset),
                    ftrans.toScreenY(yCoordinates[endIdx], yOffset)
                };
                DrawBezier(renderer, point1, controlPoint, point2);
            }
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        }
    }
}

void drawSimpleGlyphDashed(SDL_Renderer* renderer, const Glyph& glyph, FontTransform& ftrans, int xOffset, int yOffset, int thickness) { // NOLINT(bugprone-easily-swappable-parameters)
    vector<uint16_t> endpoints = glyph.getEndPtsOfContours();
    vector<int> windingDirections = glyph.getWindingDirections();

    int currentContour = 0;
    int contourStartIndex = 0;

    vector<int16_t> xCoordinates = glyph.getXCoordinates();
    vector<int16_t> yCoordinates = glyph.getYCoordinates();

    vector<uint8_t> flags = glyph.getFlags();

    // Dash and gap lengths are fractions of an em so the pattern scales with
    // font size (1/40 em is 10px at 400px, 5px at 200px).
    const float dashLength = std::max(1.0F, ftrans.pixelsPerEm * DASH_EM_FRACTION);
    const float gapLength  = std::max(1.0F, ftrans.pixelsPerEm * GAP_EM_FRACTION);
    float dashPhase = 0.0F;
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
            size_t nextIdx = wrapIdx(j, 1);
            SDL_Point point1 = {
                static_cast<int>(ftrans.toPixels(xCoordinates[j]) + xOffset),
                ftrans.toScreenY(yCoordinates[j], yOffset)
            };
            if (windingDirections[currentContour] == 1) {
                // Clockwise
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            } else {
                // Counter Clockwise
                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
            }
            if ((flags[nextIdx] & 1) != 0) {
                // Next point is on-curve too: a straight edge, not a curve.
                SDL_Point point2 = {
                    static_cast<int>(ftrans.toPixels(xCoordinates[nextIdx]) + xOffset),
                    ftrans.toScreenY(yCoordinates[nextIdx], yOffset)
                };
                // A dashed straight line is a dashed curve whose control point sits on the chord.
                SDL_Point midPoint = {(point1.x + point2.x) / 2, (point1.y + point2.y) / 2};
                DrawBezierDashed(renderer, point1, midPoint, point2, dashLength, gapLength, dashPhase);
            } else {
                // Next point is off-curve: (on, off, on) is a quadratic curve.
                size_t endIdx = wrapIdx(j, 2);
                SDL_Point controlPoint = {
                    static_cast<int>(ftrans.toPixels(xCoordinates[nextIdx]) + xOffset),
                    ftrans.toScreenY(yCoordinates[nextIdx], yOffset)
                };
                SDL_Point point2 = {
                    static_cast<int>(ftrans.toPixels(xCoordinates[endIdx]) + xOffset),
                    ftrans.toScreenY(yCoordinates[endIdx], yOffset)
                };
                DrawBezierDashed(renderer, point1, controlPoint, point2, dashLength, gapLength, dashPhase);
            }
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        }
    }
}

void drawSimpleGlyphLines(SDL_Renderer* renderer, const Glyph& glyph, FontTransform& ftrans, int xOffset, int yOffset) { // NOLINT(bugprone-easily-swappable-parameters)
    const vector<uint16_t>& endpoints = glyph.getEndPtsOfContours();
    const vector<int16_t>& xs = glyph.getXCoordinates();
    const vector<int16_t>& ys = glyph.getYCoordinates();
    const vector<uint8_t>& flags = glyph.getFlags();

    auto toScreen = [&](size_t idx) -> SDL_FPoint {
        return {
            static_cast<float>(ftrans.toPixels(xs[idx])) + static_cast<float>(xOffset),
            static_cast<float>(ftrans.toScreenY(ys[idx], yOffset))
        };
    };

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    // A straight line between every pair of consecutive ON-CURVE points in
    // each contour, skipping over any off-curve control points in between,
    // closing back to the first on-curve point.
    size_t contourStart = 0;
    for (uint16_t endpoint : endpoints) {
        size_t count = endpoint - contourStart + 1;

        vector<size_t> onCurveIndices;
        for (size_t k = 0; k < count; ++k) {
            size_t idx = contourStart + k;
            // if ((flags[idx] & 1) != 0) {
            //     onCurveIndices.push_back(idx);
            // }
            onCurveIndices.push_back(idx);
        }

        for (size_t k = 0; k < onCurveIndices.size(); ++k) {
            SDL_FPoint from = toScreen(onCurveIndices[k]);
            SDL_FPoint to   = toScreen(onCurveIndices[(k + 1) % onCurveIndices.size()]);
            SDL_RenderLine(renderer, from.x, from.y, to.x, to.y);
        }

        contourStart = static_cast<size_t>(endpoint) + 1;
    }
}

void drawTriangulatedGlyph(SDL_Renderer* renderer, const Glyph& glyph, FontTransform& ftrans, int xOffset, int yOffset) {
    vector<uint16_t> endpoints = glyph.getEndPtsOfContours();
    vector<int> windingDirections = glyph.getWindingDirections();

    int currentContour = 0;
    int contourStartIndex = 0;

    vector<int16_t> xCoordinates = glyph.getXCoordinates();
    vector<int16_t> yCoordinates = glyph.getYCoordinates();

    vector<uint8_t> flags = glyph.getFlags();

    vector<Contour> contours;
    contours.reserve(endpoints.size());

    for (uint16_t endpoint : endpoints) {
        Contour contour = {};
        size_t count = endpoint - contourStartIndex + 1;
        for (size_t k = 0; k < count; ++k) {
            SDL_Point point1 = {
                static_cast<int>(ftrans.toPixels(xCoordinates[contourStartIndex + k]) + xOffset),
                ftrans.toScreenY(yCoordinates[contourStartIndex + k], yOffset)
            };
            if ((flags[contourStartIndex + k] & 1) != 0) {
                contour.push_back({point1.x, point1.y});
            }
        }
        contours.push_back(contour);
        contourStartIndex = static_cast<int>(endpoint) + 1;
    }

    vector<Triangle> triangles = triangulateConstrained(contours);

    for (Triangle triangle : triangles) {
        drawTriangle(renderer, triangle);
    }
}
