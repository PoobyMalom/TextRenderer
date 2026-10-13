#include <SDL3/SDL.h>
#include "GlyphTable.h"
#include "FontTypes.h"
#include "Metrics.h"

void drawSimpleGlyph(SDL_Renderer* renderer, const Glyph& glyph, FontTransform& ftrans, int xOffset, int yOffset, int thickness);
void drawSimpleGlyphDashed(SDL_Renderer* renderer, const Glyph& glyph, FontTransform& ftrans, int xOffset, int yOffset, int thickness);
void drawSimpleGlyphLines(SDL_Renderer* renderer, const Glyph& glyph, FontTransform& ftrans, int xOffset, int yOffset);
void drawTriangulatedGlyph(SDL_Renderer* renderer, const Glyph& glyph, FontTransform& ftrans, int xOffset, int yOffset);
