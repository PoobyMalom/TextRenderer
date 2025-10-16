#ifndef GLYPH_RENDERER_H
#define GLYPH_RENDERER_H

#include <SDL2/SDL.h>
#include "GlyphTable.h"
#include "GeometryUtils.h"
#include "VertexList.h"

class GlyphRenderer {
public:
  enum RenderMode {
    TRIANGULATION = 0,
    DEBUG_LINES = 1,
    POINTS = 2,
    INSIDE_OFF_CURVE = 3,
    OUTSIDE_OFF_CURVE = 4
  };

  static void renderGlyph(SDL_Renderer* renderer, const Glyph& glyph,
                          RenderMode mode, double scale);

private:
  static void drawInsideOffCurvePoints(SDL_Renderer* renderer, Glyph glyph, double scale);
  static void drawOutsideOffCurvePoints(SDL_Renderer* renderer, Glyph glyph, double scale);
  static void drawDebugLines(SDL_Renderer* renderer, Glyph glyph, double scale);
  static void drawPoints(SDL_Renderer* renderer, Glyph glyph, double scale);
  static void drawTriangulationTest(SDL_Renderer* renderer, Glyph glyph, double scale);

  static void drawTriangle(SDL_Renderer* renderer, Triangle* tri, double scalingFactor, int height);
  static void drawTriangle(SDL_Renderer* renderer, Vertex* p1, Vertex* p2, Vertex* p3, double scalingFactor, int height);
};

#endif