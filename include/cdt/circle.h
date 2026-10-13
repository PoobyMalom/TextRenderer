#pragma once

#include "vertex.h"

// The circumcenter is kept at full double precision -- rounding it to a
// Vertex's int coordinates (as calculateCircumcircle used to) is fine for
// drawing but corrupts the inside-circle test Triangle::inCircumcircle
// relies on, since a rounded center can shift a borderline point from
// "inside" to "outside" or back. Round to int only where a Vertex is
// actually needed for rendering, e.g. drawCircle's center dot.
struct Circle {
  double centerX;
  double centerY;
  double radius;
};

Circle calculateCircumcircle(const Vertex& p1, const Vertex& p2, const Vertex& p3);