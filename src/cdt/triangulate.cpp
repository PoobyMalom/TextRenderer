#include "triangulate.h"
#include "vertex.h"
#include "edge.h"

std::vector<Edge> uniqueEdges(const std::vector<Edge>& edges) {
  std::vector<Edge> uniqueEdges;

  for (int i = 0; i < edges.size(); i++) {
    bool isUnique = true;

    for (int j = 0; j < edges.size(); j++) {
      if (i != j && edges[i] == edges[j]) {
        isUnique = false;
        break;
      }
    }

    if (isUnique) {
      uniqueEdges.push_back(edges[i]);
    }
  }

  return uniqueEdges;
}

std::vector<Triangle> filterTriangles(const std::vector<Triangle>& triangles, const std::vector<int>& keep) {
    std::vector<Triangle> filtered;
    filtered.reserve(triangles.size());

    for (size_t i = 0; i < triangles.size(); ++i) {
        if (keep[i] != 0) {
            filtered.push_back(triangles[i]);
        }
    }
    return filtered;
}

std::vector<Triangle> addVertex(const Vertex& vertex, const std::vector<Triangle>& existingTriangles) {
  std::vector<Edge> edges;
  std::vector<int> keep;

  for (Triangle triangle : existingTriangles) {
    if (triangle.inCircumcircle(vertex)) {
      edges.push_back({triangle.v0, triangle.v1});
      edges.push_back({triangle.v1, triangle.v2});
      edges.push_back({triangle.v2, triangle.v0});
      keep.push_back(0);
    } else {
      keep.push_back(1);
    }
  }

  std::vector<Triangle> triangles = filterTriangles(existingTriangles, keep);

  edges = uniqueEdges(edges);

  for (Edge edge : edges) {
    triangles.push_back({edge.v0, edge.v1, vertex});
  }

  return triangles;
}

std::vector<Triangle> triangulate(const std::vector<Vertex>& vertices) {
  Triangle superTriangle = calculateSuperTriangle(vertices);

  std::vector<Triangle> triangles = {superTriangle};

  for (Vertex vertex : vertices) {
    triangles = addVertex(vertex, triangles);
  }

  std::vector<int> keep;
  keep.reserve(triangles.size());

  for (const Triangle& triangle : triangles) {
      bool sharesVertex =
          triangle.v0 == superTriangle.v0 || triangle.v0 == superTriangle.v1 || triangle.v0 == superTriangle.v2 ||
          triangle.v1 == superTriangle.v0 || triangle.v1 == superTriangle.v1 || triangle.v1 == superTriangle.v2 ||
          triangle.v2 == superTriangle.v0 || triangle.v2 == superTriangle.v1 || triangle.v2 == superTriangle.v2;

      keep.push_back(sharesVertex ? 0 : 1);
  }

  triangles = filterTriangles(triangles, keep);

  return triangles;
}

std::vector<Triangle> triangulateRaw(const std::vector<Vertex>& vertices, Triangle& superTriangleOut) {
  superTriangleOut = calculateSuperTriangle(vertices);

  std::vector<Triangle> triangles = {superTriangleOut};

  for (Vertex vertex : vertices) {
    triangles = addVertex(vertex, triangles);
  }

  return triangles;
}