#pragma once

#include "edge.h"
#include "triangle.h"
#include "adjacency.h"
#include "contour.h"
#include <unordered_set>
#include <vector>

void recoverConstraintEdge(Edge constraintEdge, std::vector<Triangle>& triangles, EdgeAdjacency& adjacency,
                            const std::unordered_set<Edge, EdgeHash>& constraints);
void legalizeNonConstraintEdges(std::vector<Triangle>& triangles, const std::unordered_set<Edge, EdgeHash>& constraints,
                                 EdgeAdjacency& adjacency);
std::vector<Triangle> classifyAndStripExterior(const std::vector<Triangle>& triangles, const std::vector<Contour>& contours,
                                                const Triangle& superTriangle);
std::vector<Triangle> triangulateConstrained(const std::vector<Contour>& contours);