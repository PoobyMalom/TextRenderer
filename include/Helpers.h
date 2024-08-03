#ifndef HELPERS_H
#define HELPERS_H

#include <SDL.h>
#include <vector>
#include <string>

void DrawBezier(SDL_Renderer* renderer, const SDL_Point point1, const SDL_Point controlPoint, const SDL_Point point2);

#endif // HELPERS_H
