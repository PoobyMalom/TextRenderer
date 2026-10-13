#include "draw.h"
#include <cmath>

namespace {
constexpr float TWO_PI = 6.28318530717958647692F;
} // namespace

void drawVertex(SDL_Renderer* renderer, const Vertex& vertex, float radius, SDL_Color color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

  float centerX = static_cast<float>(vertex.x);
  float centerY = static_cast<float>(vertex.y);

  // Fill the circle with one horizontal line per row, from the top of the
  // circle to the bottom, each as wide as the circle is at that row.
  auto rowCount = static_cast<int>(std::ceil(radius));
  for (int row = -rowCount; row <= rowCount; ++row) {
    auto rowOffset = static_cast<float>(row);
    float halfWidth = std::sqrt(std::max(0.0F, (radius * radius) - (rowOffset * rowOffset)));
    SDL_RenderLine(renderer, centerX - halfWidth, centerY + rowOffset, centerX + halfWidth, centerY + rowOffset);
  }
}

void drawEdge(SDL_Renderer* renderer, const Edge& edge, SDL_Color color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  SDL_RenderLine(renderer,
                  static_cast<float>(edge.v0.x), static_cast<float>(edge.v0.y),
                  static_cast<float>(edge.v1.x), static_cast<float>(edge.v1.y));
}

void drawCircle(SDL_Renderer* renderer, const Circle& circle, SDL_Color color, int segments) {
  if (circle.radius <= 0.0 || segments < 3) {
    return;
  }

  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

  auto centerX = static_cast<float>(circle.centerX);
  auto centerY = static_cast<float>(circle.centerY);
  auto radius = static_cast<float>(circle.radius);

  // Walk the circumference in `segments` straight chords, same approach as
  // the flattened Bezier curves in TextRenderer.
  float prevX = centerX + radius;
  float prevY = centerY;
  for (int i = 1; i <= segments; ++i) {
    float t = TWO_PI * static_cast<float>(i) / static_cast<float>(segments); // NOLINT(readability-identifier-length)
    float x = centerX + (radius * std::cos(t)); // NOLINT(readability-identifier-length)
    float y = centerY + (radius * std::sin(t)); // NOLINT(readability-identifier-length)
    SDL_RenderLine(renderer, prevX, prevY, x, y);
    prevX = x;
    prevY = y;
  }

  // drawVertex renders a pixel-space dot, so round the precise center to int
  // only here, at the point it's actually needed for display.
  Vertex centerDot = {static_cast<int>(std::lround(circle.centerX)), static_cast<int>(std::lround(circle.centerY))};
  drawVertex(renderer, centerDot, 3.0F, {175, 175, 255, 255});
}

void drawTriangle(SDL_Renderer* renderer, const Triangle& triangle, SDL_Color color) {
  drawEdge(renderer, Edge{triangle.v0, triangle.v1}, color);
  drawEdge(renderer, Edge{triangle.v1, triangle.v2}, color);
  drawEdge(renderer, Edge{triangle.v2, triangle.v0}, color);
}
