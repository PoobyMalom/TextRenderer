#include "DemoPhases.h"
#include <cmath>

namespace {

SDL_Point toScreenPoint(const FontTransform& ftrans, const std::vector<int16_t>& xs,
                        const std::vector<int16_t>& ys, size_t idx, int xOffset, int yOffset) {
    return {
        static_cast<int>(ftrans.toPixels(xs[idx])) + xOffset,
        ftrans.toScreenY(ys[idx], yOffset)
    };
}

void drawLine(SDL_Renderer* renderer, SDL_Point from, SDL_Point to) { // NOLINT(readability-identifier-length)
    SDL_RenderDrawLine(renderer, from.x, from.y, to.x, to.y);
}

// Samples each contour's Bezier segments into straight-line vertices, in
// screen space, at a fixed number of steps per curve.
std::vector<std::vector<SDL_Point>> flattenGlyphContours(const Glyph& glyph, const FontTransform& ftrans,
                                                          int xOffset, int yOffset) {
    std::vector<std::vector<SDL_Point>> contours;

    const std::vector<uint16_t>& endpoints = glyph.getEndPtsOfContours();
    const std::vector<int16_t>& xs = glyph.getXCoordinates();
    const std::vector<int16_t>& ys = glyph.getYCoordinates();
    const std::vector<uint8_t>& flags = glyph.getFlags();

    const int stepsPerCurve = 8;

    size_t contourStart = 0;
    for (uint16_t endpoint : endpoints) {
        auto contourLen = static_cast<int>(endpoint - contourStart + 1);
        auto wrapIdx = [&](size_t base, int offset) -> size_t {
            int localIdx = static_cast<int>(base - contourStart) + offset;
            return contourStart + static_cast<size_t>(((localIdx % contourLen) + contourLen) % contourLen);
        };

        std::vector<SDL_Point> points;
        for (size_t j = contourStart; j <= endpoint; ++j) {
            if ((flags[j] & 1) == 0) {
                continue;
            }
            size_t ctrlIdx = wrapIdx(j, 1);
            size_t endIdx = wrapIdx(j, 2);

            SDL_Point p0 = toScreenPoint(ftrans, xs, ys, j, xOffset, yOffset);
            SDL_Point p1 = toScreenPoint(ftrans, xs, ys, ctrlIdx, xOffset, yOffset);
            SDL_Point p2 = toScreenPoint(ftrans, xs, ys, endIdx, xOffset, yOffset);

            for (int step = 0; step < stepsPerCurve; ++step) {
                auto t = static_cast<float>(step) / static_cast<float>(stepsPerCurve); // NOLINT(readability-identifier-length)
                points.push_back(getBezierPoint(p0, p1, p2, t));
            }
        }

        contours.push_back(std::move(points));
        contourStart = endpoint + 1;
    }

    return contours;
}

double signedArea(const std::vector<SDL_Point>& points) {
    double area = 0.0;
    for (size_t i = 0; i < points.size(); ++i) {
        const SDL_Point& current = points[i];
        const SDL_Point& next = points[(i + 1) % points.size()];
        area += (static_cast<double>(current.x) * next.y) - (static_cast<double>(next.x) * current.y);
    }
    return area * 0.5;
}

void drawArrowhead(SDL_Renderer* renderer, SDL_Point from, SDL_Point to) {
    auto dx = static_cast<float>(to.x - from.x); // NOLINT(readability-identifier-length)
    auto dy = static_cast<float>(to.y - from.y); // NOLINT(readability-identifier-length)
    float len = std::sqrt((dx * dx) + (dy * dy));
    if (len < 1.0F) {
        return;
    }
    dx /= len;
    dy /= len;
    float perpX = -dy;
    float perpY = dx;

    const float headLength = 14.0F;
    const float headWidth = 6.0F;

    SDL_Point base = {
        static_cast<int>(static_cast<float>(to.x) - (dx * headLength)),
        static_cast<int>(static_cast<float>(to.y) - (dy * headLength))
    };
    SDL_Point left = {
        static_cast<int>(static_cast<float>(base.x) + (perpX * headWidth)),
        static_cast<int>(static_cast<float>(base.y) + (perpY * headWidth))
    };
    SDL_Point right = {
        static_cast<int>(static_cast<float>(base.x) - (perpX * headWidth)),
        static_cast<int>(static_cast<float>(base.y) - (perpY * headWidth))
    };

    drawLine(renderer, to, left);
    drawLine(renderer, to, right);
}

} // namespace

const PhaseInfo& getPhaseInfo(Phase phase) {
    static const PhaseInfo kPhases[kNumPhases] = {
        {"Raw control points",
         "Every point the font file actually stores, all drawn the same color."},
        {"On-curve vs off-curve",
         "Same raw points, colored: green sits on the outline, orange only pulls it."},
        {"Straight skeleton",
         "Connect only the on-curve points in order, ignoring the off-curve points entirely."},
        {"Control handles",
         "Add a handle line from each off-curve point to its neighbors, plus a solid line for segments that are already straight (two on-curve points back to back)."},
        {"True Bezier curves",
         "Replace the straight skeleton with real quadratic curves through the control points. Dots and handles are gone."},
        {"Constant spacing (monospace font)",
         "Advance every glyph by the same fixed pixel width. Looks correct -- this font's glyphs are all the same width anyway."},
        {"Constant spacing (proportional font)",
         "Same fixed advance, unchanged, now on a font whose glyphs vary in width. Watch it overlap and gap."},
        {"Real spacing via hmtx",
         "Look up each glyph's actual advance width from the font's hmtx table. The proportional font is fixed."},
        {"Naive vertical spacing",
         "Line height = ascent only, ignoring descent and line gap. Descenders crash into the next line."},
        {"Naive vertical spacing (tall font)",
         "Same ascent-only formula, now on a font whose glyphs reach much further above and below the baseline. Lines collapse into each other."},
        {"Real vertical spacing",
         "Same tall font, now with line height = ascent - descent + lineGap from its hhea table. The collision resolves."},
        {"Earmarking for fill",
         "Back to the JetBrains Mono 'g'. Flatten it to line segments, then mark each contour's winding direction -- the first thing a fill/triangulation pass needs to know. Step with ']'."},
    };
    return kPhases[static_cast<int>(phase)];
}

void drawGlyphPoints(SDL_Renderer* renderer, const Glyph& glyph, const FontTransform& ftrans,
                      int xOffset, int yOffset, bool colorByOnCurve, int radius) {
    const std::vector<int16_t>& xs = glyph.getXCoordinates();
    const std::vector<int16_t>& ys = glyph.getYCoordinates();
    const std::vector<uint8_t>& flags = glyph.getFlags();

    for (size_t i = 0; i < xs.size(); ++i) {
        bool onCurve = (flags[i] & 1) != 0;
        if (colorByOnCurve) {
            if (onCurve) {
                SDL_SetRenderDrawColor(renderer, 0, 160, 0, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 230, 120, 0, 255);
            }
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        }

        SDL_Point point = toScreenPoint(ftrans, xs, ys, i, xOffset, yOffset);
        SDL_Rect rect = {point.x - radius, point.y - radius, radius * 2, radius * 2};
        SDL_RenderFillRect(renderer, &rect);
    }
}

void drawGlyphSkeleton(SDL_Renderer* renderer, const Glyph& glyph, const FontTransform& ftrans,
                       int xOffset, int yOffset) {
    const std::vector<uint16_t>& endpoints = glyph.getEndPtsOfContours();
    const std::vector<int16_t>& xs = glyph.getXCoordinates();
    const std::vector<int16_t>& ys = glyph.getYCoordinates();
    const std::vector<uint8_t>& flags = glyph.getFlags();

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    size_t contourStart = 0;
    for (uint16_t endpoint : endpoints) {
        int firstOnCurveIdx = -1;
        int prevOnCurveIdx = -1;

        for (size_t i = contourStart; i <= endpoint; ++i) {
            if ((flags[i] & 1) == 0) {
                continue;
            }
            if (firstOnCurveIdx == -1) {
                firstOnCurveIdx = static_cast<int>(i);
            }
            if (prevOnCurveIdx != -1) {
                drawLine(renderer,
                         toScreenPoint(ftrans, xs, ys, static_cast<size_t>(prevOnCurveIdx), xOffset, yOffset),
                         toScreenPoint(ftrans, xs, ys, i, xOffset, yOffset));
            }
            prevOnCurveIdx = static_cast<int>(i);
        }

        if (firstOnCurveIdx != -1 && prevOnCurveIdx != firstOnCurveIdx) {
            drawLine(renderer,
                     toScreenPoint(ftrans, xs, ys, static_cast<size_t>(prevOnCurveIdx), xOffset, yOffset),
                     toScreenPoint(ftrans, xs, ys, static_cast<size_t>(firstOnCurveIdx), xOffset, yOffset));
        }

        contourStart = endpoint + 1;
    }
}

void drawControlHandles(SDL_Renderer* renderer, const Glyph& glyph, const FontTransform& ftrans,
                         int xOffset, int yOffset) {
    const std::vector<uint16_t>& endpoints = glyph.getEndPtsOfContours();
    const std::vector<int16_t>& xs = glyph.getXCoordinates();
    const std::vector<int16_t>& ys = glyph.getYCoordinates();
    const std::vector<uint8_t>& flags = glyph.getFlags();

    SDL_SetRenderDrawColor(renderer, 160, 160, 160, 255);

    size_t contourStart = 0;
    for (uint16_t endpoint : endpoints) {
        size_t contourLen = endpoint - contourStart + 1;

        for (size_t i = contourStart; i <= endpoint; ++i) {
            if ((flags[i] & 1) != 0) {
                continue;
            }
            size_t offsetInContour = i - contourStart;
            size_t prevIdx = contourStart + ((offsetInContour + contourLen - 1) % contourLen);
            size_t nextIdx = contourStart + ((offsetInContour + 1) % contourLen);

            SDL_Point current = toScreenPoint(ftrans, xs, ys, i, xOffset, yOffset);
            drawLine(renderer, toScreenPoint(ftrans, xs, ys, prevIdx, xOffset, yOffset), current);
            drawLine(renderer, current, toScreenPoint(ftrans, xs, ys, nextIdx, xOffset, yOffset));
        }

        contourStart = endpoint + 1;
    }
}

void drawStraightSegments(SDL_Renderer* renderer, const Glyph& glyph, const FontTransform& ftrans,
                          int xOffset, int yOffset) {
    const std::vector<uint16_t>& endpoints = glyph.getEndPtsOfContours();
    const std::vector<int16_t>& xs = glyph.getXCoordinates();
    const std::vector<int16_t>& ys = glyph.getYCoordinates();
    const std::vector<uint8_t>& flags = glyph.getFlags();

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    size_t contourStart = 0;
    for (uint16_t endpoint : endpoints) {
        size_t contourLen = endpoint - contourStart + 1;

        for (size_t i = contourStart; i <= endpoint; ++i) {
            if ((flags[i] & 1) == 0) {
                continue;
            }
            size_t offsetInContour = i - contourStart;
            size_t nextIdx = contourStart + ((offsetInContour + 1) % contourLen);
            if ((flags[nextIdx] & 1) != 0) {
                drawLine(renderer, toScreenPoint(ftrans, xs, ys, i, xOffset, yOffset),
                         toScreenPoint(ftrans, xs, ys, nextIdx, xOffset, yOffset));
            }
        }

        contourStart = endpoint + 1;
    }
}

const SubStepInfo& getEarmarkSubStepInfo(int step) {
    static const SubStepInfo kSteps[kNumEarmarkSteps] = {
        {"Flatten to line segments",
         "Sample each Bezier curve into short straight segments -- the polygon a fill pass actually operates on."},
        {"Mark winding direction",
         "Color each contour by winding relative to the largest one, and draw an arrow along its first edge: same winding fills, opposite winding subtracts a hole. This is as far as the engine goes today -- next up is turning this into triangles and a real fill."},
    };
    return kSteps[step];
}

void drawFlattenedOutline(SDL_Renderer* renderer, const Glyph& glyph, const FontTransform& ftrans,
                          int xOffset, int yOffset) {
    std::vector<std::vector<SDL_Point>> contours = flattenGlyphContours(glyph, ftrans, xOffset, yOffset);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    for (const std::vector<SDL_Point>& points : contours) {
        for (size_t i = 0; i < points.size(); ++i) {
            drawLine(renderer, points[i], points[(i + 1) % points.size()]);
        }
    }
}

void drawWindingDirections(SDL_Renderer* renderer, const Glyph& glyph, const FontTransform& ftrans,
                           int xOffset, int yOffset) {
    std::vector<std::vector<SDL_Point>> contours = flattenGlyphContours(glyph, ftrans, xOffset, yOffset);
    if (contours.empty()) {
        return;
    }

    std::vector<double> areas(contours.size());
    size_t outerIdx = 0;
    double maxAbsArea = 0.0;
    for (size_t c = 0; c < contours.size(); ++c) {
        areas[c] = signedArea(contours[c]);
        double absArea = std::abs(areas[c]);
        if (absArea > maxAbsArea) {
            maxAbsArea = absArea;
            outerIdx = c;
        }
    }
    bool outerPositive = areas[outerIdx] >= 0.0;

    for (size_t c = 0; c < contours.size(); ++c) {
        bool matchesOuterWinding = (areas[c] >= 0.0) == outerPositive;
        if (matchesOuterWinding) {
            SDL_SetRenderDrawColor(renderer, 0, 160, 0, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 210, 30, 30, 255);
        }

        const std::vector<SDL_Point>& points = contours[c];
        for (size_t i = 0; i < points.size(); ++i) {
            drawLine(renderer, points[i], points[(i + 1) % points.size()]);
        }
        if (points.size() >= 2) {
            drawArrowhead(renderer, points[0], points[1]);
        }
    }
}
