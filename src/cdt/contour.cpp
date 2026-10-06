#include "contour.h"

std::vector<Edge> buildConstraintEdges(const std::vector<Contour>& contours) {
  std::vector<Edge> edges;

  for (Contour contour : contours) {
    auto pointCount = static_cast<int>(contour.size());
    for (int i = 0; i < pointCount; i++) {
      edges.push_back({contour[i], contour[(i + 1) % pointCount]});
    }
  }

  return edges;
}

Contour flattenContours(const std::vector<Contour>& contours) {
  Contour all;

  for (Contour contour : contours) {
    for (Vertex vertex : contour) {
      all.push_back(vertex);
    }
  }

  return all;
}