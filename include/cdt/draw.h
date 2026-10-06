#pragma once

#include <SDL3/SDL.h>
#include "vertex.h"
#include "edge.h"
#include "circle.h"
#include "triangle.h"

// A small filled circle centered on the vertex.
void drawVertex(SDL_Renderer* renderer, const Vertex& vertex, float radius = 4.0F,
                SDL_Color color = {0, 0, 0, 255});

// A single line between the edge's two endpoints.
void drawEdge(SDL_Renderer* renderer, const Edge& edge, SDL_Color color = {0, 0, 0, 255});

// The circle's outline, approximated with straight segments, plus a small dot at its center.
// Does nothing if the circle is degenerate (radius <= 0, as calculateCircumcircle returns for
// three collinear points).
void drawCircle(SDL_Renderer* renderer, const Circle& circle, SDL_Color color = {0, 0, 0, 255},
                int segments = 48);

// The triangle's three edges.
void drawTriangle(SDL_Renderer* renderer, const Triangle& triangle, SDL_Color color = {0, 0, 0, 255});
