#pragma once

#include <vector>
#include "edge.h"
#include "triangle.h"

std::vector<Edge> uniqueEdges(const std::vector<Edge>& edges);
std::vector<Triangle> filterTriangles(const std::vector<Triangle>& triangles, const std::vector<int>& keep);
std::vector<Triangle> addVertex(const Vertex& vertex, const std::vector<Triangle>& existingTriangles);
std::vector<Triangle> triangulate(const std::vector<Vertex>& vertices);
std::vector<Triangle> triangulateRaw(const std::vector<Vertex>& vertices, Triangle& superTriangleOut);