#pragma once

#include <SDL2/SDL.h>
#include "GlyphTable.h"
#include "Metrics.h"

enum class Phase {
    RawPoints = 0,
    ColoredPoints,
    Skeleton,
    ControlHandles,
    BezierCurves,
    MonoConstantSpacing,
    BrokenConstantSpacing,
    HmtxSpacing,
    NaiveVerticalSpacing,
    TallFontNaiveVerticalSpacing,
    RealVerticalSpacing,
    EarmarkingContours,
    Count
};

constexpr int kNumPhases = static_cast<int>(Phase::Count);

struct PhaseInfo {
    const char* name;
    const char* description;
};

const PhaseInfo& getPhaseInfo(Phase phase);

// Phase 12 (last) has its own internal sub-steps, stepped manually with the
// same '[' / ']' keys once you're on the phase: a first look at what filling
// a glyph will need, starting from the same JetBrains Mono 'g'.
constexpr int kNumEarmarkSteps = 2;

struct SubStepInfo {
    const char* name;
    const char* description;
};

const SubStepInfo& getEarmarkSubStepInfo(int step);

// Phases 1-2: dots at every raw point. colorByOnCurve=false draws every dot
// the same color; true colors on-curve vs off-curve points differently.
void drawGlyphPoints(SDL_Renderer* renderer, const Glyph& glyph, const FontTransform& ftrans,
                      int xOffset, int yOffset, bool colorByOnCurve, int radius);

// Phase 3: straight lines connecting only the on-curve points of each
// contour, in original point order, skipping over any off-curve points.
void drawGlyphSkeleton(SDL_Renderer* renderer, const Glyph& glyph, const FontTransform& ftrans,
                       int xOffset, int yOffset);

// Phase 4: a thin "control polygon" line from each off-curve point to its
// immediate neighbors in the same contour.
void drawControlHandles(SDL_Renderer* renderer, const Glyph& glyph, const FontTransform& ftrans,
                         int xOffset, int yOffset);

// Phase 4 (companion): a solid line between two consecutive raw contour
// points that are both on-curve -- a genuine straight edge, not an
// approximation, unlike the skip-lines in drawGlyphSkeleton.
void drawStraightSegments(SDL_Renderer* renderer, const Glyph& glyph, const FontTransform& ftrans,
                          int xOffset, int yOffset);

// Phase 12, sub-step 1: flatten every contour's Bezier curves into straight
// line segments -- the polygon a fill/triangulation pass would actually
// work with.
void drawFlattenedOutline(SDL_Renderer* renderer, const Glyph& glyph, const FontTransform& ftrans,
                          int xOffset, int yOffset);

// Phase 12, sub-step 2: same flattened polygon, with each contour colored by
// winding direction relative to the largest (outer) contour, plus a direction
// arrow -- the "earmarking" a nonzero-winding fill needs before it can tell
// solid outline from hole.
void drawWindingDirections(SDL_Renderer* renderer, const Glyph& glyph, const FontTransform& ftrans,
                           int xOffset, int yOffset);
