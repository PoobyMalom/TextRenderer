#include "Renderer.h"
#include "Helpers.h"
#include "cdt/draw.h"
#include "cdt/triangle.h"
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

void drawTriangulatedGlyph(SDL_Renderer* renderer, const Glyph& glyph, int xOffset, int yOffset) {
    // Glyph::triangulateGlyph() (called once, at parse time) already did the
    // CDT work in glyph-local pixel space and picked each triangle's color.
    // Re-running the full pipeline here every frame was the original design
    // and was wasteful -- translation doesn't change triangle connectivity,
    // so drawing is just a per-vertex offset.
    const vector<Triangle>& triangles = glyph.getTriangles();

    for (size_t i = 0; i < triangles.size(); ++i) {
        const Triangle& triangle = triangles[i];
        Triangle translated{
            {triangle.v0.x + xOffset, triangle.v0.y + yOffset},
            {triangle.v1.x + xOffset, triangle.v1.y + yOffset},
            {triangle.v2.x + xOffset, triangle.v2.y + yOffset}
        };
        drawTriangle(renderer, translated, {0, 0, 0, 255});
    }
}


namespace {
// Loop-Blinn implicit curve test: map pixel p into the triangle's (u,v)
// parametric space via barycentric coordinates, then test there. Loop-Blinn
// assigns fixed texture coords to the three control points regardless of
// where the triangle sits on screen: the two on-curve points get (0,0) and
// (1,1) (order between them doesn't matter -- a quadratic Bezier traces the
// same point set with its endpoints swapped and t -> 1-t), and the off-curve
// control point gets (0.5,0). `control` identifies which of tri's own
// v0/v1/v2 is actually the control point -- CDT's mesh triangle can store
// them in any rotation, so this can't be assumed to always be v1.
bool loopBlinnInside(const Triangle& tri, const Vertex& control, const Vertex& p) {
    Vertex onCurveA = tri.v0;
    Vertex onCurveB = tri.v1;
    if (control == tri.v0) {
        onCurveA = tri.v1;
        onCurveB = tri.v2;
    } else if (control == tri.v1) {
        onCurveA = tri.v0;
        onCurveB = tri.v2;
    } else {
        onCurveA = tri.v0;
        onCurveB = tri.v1;
    }

    double denom = static_cast<double>(control.x - onCurveA.x) * (onCurveB.y - onCurveA.y) -
                   static_cast<double>(onCurveB.x - onCurveA.x) * (control.y - onCurveA.y);
    double lambda1 = (static_cast<double>(p.x - onCurveA.x) * (onCurveB.y - onCurveA.y) -
                       static_cast<double>(onCurveB.x - onCurveA.x) * (p.y - onCurveA.y)) / denom;
    double lambda2 = (static_cast<double>(control.x - onCurveA.x) * (p.y - onCurveA.y) -
                       static_cast<double>(p.x - onCurveA.x) * (control.y - onCurveA.y)) / denom;
    double u = 0.5 * lambda1 + lambda2;
    double v = lambda2;
    return (u * u - v) < 0;
}
} // namespace

void GlyphRender::drawGlyph(SDL_Renderer* renderer, int xOffset, int yOffset) const {
    const vector<Triangle>& triangles = glyph.getTriangles();
    const vector<int>& trianglePositions = glyph.getTrianglePositions();
    const vector<Vertex>& curveControlPoints = glyph.getCurveControlPoints();

    // glyphBoundingBox is in raw font units (straight off the glyph header),
    // but the triangles are in pixel space -- convert through the same
    // FontTransform calls Glyph::triangulateGlyph used to build them, or the
    // loop range below doesn't line up with where the triangles actually are.
    // toScreenY flips sign (font-unit yMax, the top of the glyph, becomes the
    // most negative screen y), so the screen-space min/max swap accordingly.
    int xMinPx = static_cast<int>(trans.toPixels(glyphBoundingBox.xMin));
    int xMaxPx = static_cast<int>(trans.toPixels(glyphBoundingBox.xMax));
    int yMinPx = trans.toScreenY(glyphBoundingBox.yMax, 0);
    int yMaxPx = trans.toScreenY(glyphBoundingBox.yMin, 0);

    for (int i = xMinPx; i <= xMaxPx; i++) {
        for (int j = yMinPx; j <= yMaxPx; j++) {
            // triangles are in glyph-local space (no offset baked in, same
            // as drawTriangulatedGlyph) -- test locally, translate only the
            // point actually drawn to the screen.
            Vertex v = {i, j};
            for (int k = 0; k < trianglePositions.size(); k++) {
                if (triangles[k].pointInTriangle(v) && trianglePositions[k] == 0) {
                    // SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    SDL_RenderPoint(renderer, i + xOffset, j + yOffset);
                }
                if (triangles[k].pointInTriangle(v) && trianglePositions[k] == 1) {
                    if (loopBlinnInside(triangles[k], curveControlPoints[k], v) == 1) {
                        // SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                        SDL_RenderPoint(renderer, i + xOffset, j + yOffset);
                    }
                }
                if (triangles[k].pointInTriangle(v) && trianglePositions[k] == 2) {
                    if (loopBlinnInside(triangles[k], curveControlPoints[k], v) == 0) {
                        // SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                        SDL_RenderPoint(renderer, i + xOffset, j + yOffset);
                    }
                }
            }
        }
    }
}