#include "circle.h"

#include <cmath>

Circle calculateCircumcircle(const Vertex& p1, const Vertex& p2, const Vertex& p3) {
  // Same overflow risk as Vertex::sumSquare: cast before multiplying, not
  // after, so a far-away point (a super-triangle corner) can't overflow the
  // int multiplication before it ever reaches double.
  double d = 2.0 * ((static_cast<double>(p1.x) * (p2.y - p3.y)) +
                    (static_cast<double>(p2.x) * (p3.y - p1.y)) +
                    (static_cast<double>(p3.x) * (p1.y - p2.y)));

  if (std::abs(d) < 1e-9) {
    return {0.0, 0.0, -1.0};
  }

  double cx = (p1.sumSquare() * (p2.y - p3.y) +
               p2.sumSquare() * (p3.y - p1.y) +
               p3.sumSquare() * (p1.y - p2.y)
              ) / d;

  double cy = (p1.sumSquare() * (p3.x - p2.x) +
               p2.sumSquare() * (p1.x - p3.x) +
               p3.sumSquare() * (p2.x - p1.x)
              ) / d;

  double radius = std::sqrt((p1.x - cx) * (p1.x - cx) + (p1.y - cy) * (p1.y - cy));

  return {cx, cy, radius};
}