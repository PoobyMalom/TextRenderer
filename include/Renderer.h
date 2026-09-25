#include "SDL2/SDL.h"
#include "GlyphTable.h"
#include "FontTypes.h"
#include "Metrics.h"

void drawSimpleGlyph(SDL_Renderer* renderer, const Glyph& glyph, FontTransform& ftrans, int xOffset, int yOffset, int thickness);