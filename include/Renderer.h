#include <SDL3/SDL.h>
#include "GlyphTable.h"
#include "FontTypes.h"
#include "Metrics.h"

struct GlyphBox {
  int xMin;
  int xMax;
  int yMin;
  int yMax;

  bool inBoundingBox(const Vertex& point) {
    return point.x >= xMin && point.x <= xMax && point.y >= yMin && point.y <= yMax;
  }
};

struct GlyphRender {
  Glyph& glyph;
  GlyphBox& glyphBoundingBox;
  FontTransform& trans;
  SDL_Color fontColor;

  void drawGlyph(SDL_Renderer* renderer, int xOffset, int yOffset) const;
};

void drawSimpleGlyph(SDL_Renderer* renderer, const Glyph& glyph, FontTransform& ftrans, int xOffset, int yOffset, int thickness);
void drawSimpleGlyphDashed(SDL_Renderer* renderer, const Glyph& glyph, FontTransform& ftrans, int xOffset, int yOffset, int thickness);
void drawSimpleGlyphLines(SDL_Renderer* renderer, const Glyph& glyph, FontTransform& ftrans, int xOffset, int yOffset);
void drawTriangulatedGlyph(SDL_Renderer* renderer, const Glyph& glyph, int xOffset, int yOffset);
