#pragma once

#include "vertex.h"
#include "circle.h"
#include "edge.h"
#include <cmath>
#include <vector>

struct Triangle {
  Vertex v0;
  Vertex v1;
  Vertex v2;

  Circle circumCirc = calculateCircumcircle(v0, v1, v2);

  bool inCircumcircle(const Vertex& v) const {
    double dx = circumCirc.centerX - v.x;
    double dy = circumCirc.centerY - v.y;
    return std::sqrt(dx * dx + dy * dy) <= circumCirc.radius;
  }

  bool hasEdge(const Edge& edge) const {
    return edge == Edge{v0, v1} || edge == Edge{v1, v2} || edge == Edge{v2, v0};
  }

  bool pointInTriangle(const Vertex& p) const { // NOLINT(readability-identifier-length)
    auto cross = [](const Vertex& a, const Vertex& b, const Vertex& c) -> long long { // NOLINT(readability-identifier-length)
      return static_cast<long long>(b.x - a.x) * (c.y - a.y) -
             static_cast<long long>(b.y - a.y) * (c.x - a.x);
    };

    long long d0 = cross(v0, v1, p);
    long long d1 = cross(v1, v2, p);
    long long d2 = cross(v2, v0, p);

    bool hasNeg = (d0 < 0) || (d1 < 0) || (d2 < 0);
    bool hasPos = (d0 > 0) || (d1 > 0) || (d2 > 0);
    return !(hasNeg && hasPos);
  }
};

Triangle calculateSuperTriangle(const std::vector<Vertex>& vertices);
