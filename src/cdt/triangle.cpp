#include "triangle.h"

#include <algorithm>

Triangle calculateSuperTriangle(const std::vector<Vertex>& vertices) {
  float minx = INFINITY;
  float miny = INFINITY;
  float maxx = -INFINITY;
  float maxy = -INFINITY;

  for (Vertex vertex : vertices) {
    minx = std::min(minx, static_cast<float>(vertex.x));
    miny = std::min(miny, static_cast<float>(vertex.y));
    maxx = std::max(maxx, static_cast<float>(vertex.x));
    maxy = std::max(maxy, static_cast<float>(vertex.y));
  }

  // Needs to be large enough that the super triangle's corners behave like
  // points at infinity relative to the real point set -- too small, and a
  // couple of legitimate hull-adjacent triangles get pulled into the final
  // "touches a super corner" removal along with the ones that should go.
  // *10 measurably wasn't enough; *100 has margin to spare (confirmed
  // converging well before this and staying exact up to *10000).
  float dx = (maxx - minx) * 100;
  float dy = (maxy - miny) * 100;

  Vertex v0 = {static_cast<int>(minx - dx), static_cast<int>(miny - dy * 3)};
  Vertex v1 = {static_cast<int>(minx - dx), static_cast<int>(maxy + dy)};
  Vertex v2 = {static_cast<int>(maxx + dx * 3), static_cast<int>(maxy + dy)};

  return {v0, v1, v2};
}
