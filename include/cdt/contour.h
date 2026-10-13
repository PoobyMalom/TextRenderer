#pragma once

#include "vertex.h"
#include "edge.h"
#include <vector>

using Contour = std::vector<Vertex>;

std::vector<Edge> buildConstraintEdges(const std::vector<Contour>& contours);
Contour flattenContours(const std::vector<Contour>& contours);