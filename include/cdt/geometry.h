#pragma once

#include "vertex.h"

long long orientation(const Vertex& a, const Vertex& b, const Vertex& c);
bool segmentsIntersect(const Vertex& p1, const Vertex& p2, const Vertex& p3, const Vertex& p4);
bool isConvexQuad(const Vertex& a, const Vertex& b, const Vertex& c, const Vertex& d);