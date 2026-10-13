#include "geometry.h"
#include <cmath>

static inline int sign(long long v) { return (v > 0) - (v < 0); }

static inline bool onSegment(const Vertex& a, const Vertex& b, const Vertex& c) {
  return std::min(a.x, b.x) <= c.x && c.x <= std::max(a.x, b.x) &&
         std::min(a.y, b.y) <= c.y && c.y <= std::max(a.y, b.y);
}

long long orientation(const Vertex& a, const Vertex& b, const Vertex& c) {
  return (b - a).cross(c - a);
}

bool segmentsIntersect(const Vertex& p1, const Vertex& p2, const Vertex& p3, const Vertex& p4) {
  int d1 = sign(orientation(p3, p4, p1));
  int d2 = sign(orientation(p3, p4, p2));
  int d3 = sign(orientation(p1, p2, p3));
  int d4 = sign(orientation(p1, p2, p4));

  if (d1 * d2 < 0 && d3 * d4 < 0) return true;

  if (d1 == 0 && onSegment(p3, p4, p1)) return true;
  if (d2 == 0 && onSegment(p3, p4, p2)) return true;
  if (d3 == 0 && onSegment(p1, p2, p3)) return true;
  if (d4 == 0 && onSegment(p1, p2, p4)) return true;

  return false;
}

bool isConvexQuad(const Vertex& a, const Vertex& b, const Vertex& c, const Vertex& d) {
  // a-c is the current diagonal (the shared triangle edge), b-d is the
  // proposed new one. The quad a-b-c-d is convex enough to legally flip
  // iff the two diagonals actually cross each other -- this also accepts
  // the exactly-collinear corner cases the old 4-cross-product test
  // rejected, which are common along axis-aligned glyph outlines.
  return segmentsIntersect(a, c, b, d);
}