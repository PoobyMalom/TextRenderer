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

// extraVertices get inserted into the mesh alongside the contour points
// (e.g. quadratic Bezier control points), and extraConstraints get forced
// as mesh edges alongside the contour boundary (e.g. the two "legs" from a
// control point to its curve's two on-curve endpoints). expectedTriangles
// names the exact triangles (as unordered vertex triples) this should
// produce from that -- e.g. {onCurveStart, control, onCurveEnd} for each
// curve segment. Those sit outside the fill polygon by design (the control
// point is off the true curve), so classifyAndStripExterior's normal
// inside/outside test would otherwise discard them: whichever of them
// actually exists in the raw mesh (matched by vertex set, not by touching
// an extra vertex at all -- a control point also ends up with ordinary
// "exterior fabric" triangles neither pair of legs encloses) is added back
// into the result. The caller is expected to already know expectedTriangles
// and so can identify which of the returned triangles are these, to treat
// as a distinct "needs its own curve test" bucket rather than plain solid fill.
std::vector<Triangle> triangulateConstrained(const std::vector<Contour>& contours, const std::vector<Vertex>& extraVertices,
                                              const std::vector<Edge>& extraConstraints,
                                              const std::vector<Triangle>& expectedTriangles);