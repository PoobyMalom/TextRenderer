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
};

Triangle calculateSuperTriangle(const std::vector<Vertex>& vertices);