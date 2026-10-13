#include "constrain.h"

#include "triangulate.h"
#include "geometry.h"
#include <algorithm>
#include <unordered_set>

namespace {

// Standard even-odd ray-casting point-in-polygon test.
bool pointInPolygon(const Contour& polygon, double px, double py) {
  bool inside = false;
  size_t pointCount = polygon.size();

  for (size_t i = 0, j = pointCount - 1; i < pointCount; j = i++) {
    double xi = polygon[i].x;
    double yi = polygon[i].y;
    double xj = polygon[j].x;
    double yj = polygon[j].y;

    bool crossesRay = ((yi > py) != (yj > py)) && (px < (((xj - xi) * (py - yi)) / (yj - yi)) + xi);
    if (crossesRay) {
      inside = !inside;
    }
  }

  return inside;
}

// Even-odd across ALL contours combined: inside an odd number of them (e.g.
// inside the outer contour only) means solid ink; inside an even number
// (e.g. also inside a hole contour) means background. This is a property of
// the real contour geometry, not of the mesh, so unlike a dual-graph flood
// fill it isn't confused by a single contour that pinches inward and
// briefly touches itself (as this glyph's outer contour does at its
// crossbar) -- both sides of a self-touching edge are still correctly
// "inside" here, whereas walking the mesh across that edge looks
// indistinguishable from crossing a real inside/outside boundary.
bool isInsideGlyph(const std::vector<Contour>& contours, double px, double py) {
  bool inside = false;
  for (const Contour& contour : contours) {
    if (pointInPolygon(contour, px, py)) {
      inside = !inside;
    }
  }
  return inside;
}

} // namespace

bool isConstraintEdge(const Edge& edge, const std::unordered_set<Edge, EdgeHash>& constraints) {
  return constraints.find(edge) != constraints.end();
}

bool edgePresent(const Edge& edge, const EdgeAdjacency& adjacency) {
  return adjacency.triangleIndicesByEdge.find(edge) != adjacency.triangleIndicesByEdge.end();
}

Vertex thirdVertex(const Triangle& triangle, const Edge& edge) {
  if (!(triangle.v0 == edge.v0) && !(triangle.v0 == edge.v1)) {
    return triangle.v0;
  }
  if (!(triangle.v1 == edge.v0) && !(triangle.v1 == edge.v1)) {
    return triangle.v1;
  }
  return triangle.v2;
}

bool sharesSuperTriangleVertex(const Triangle& triangle, const Triangle& superTriangle) {
  return triangle.v0 == superTriangle.v0 || triangle.v0 == superTriangle.v1 || triangle.v0 == superTriangle.v2 ||
         triangle.v1 == superTriangle.v0 || triangle.v1 == superTriangle.v1 || triangle.v1 == superTriangle.v2 ||
         triangle.v2 == superTriangle.v0 || triangle.v2 == superTriangle.v1 || triangle.v2 == superTriangle.v2;
}

// Three distinct but exactly-collinear points (a tight run of near-straight
// contour points, seen e.g. in Eater-Regular's 'X' at y=232) give
// calculateCircumcircle a degenerate, infinite-radius circle, which it
// reports as radius -1. inCircumcircle can then never flag that triangle
// for a legalizing flip, so it survives into the final mesh as a true
// zero-area sliver -- it contributes no fill, and visually overlaps an edge
// of its real neighbor like a stray spike. Filtered out at classification
// time since it's provably contributing nothing.
bool isDegenerate(const Triangle& triangle) {
  long long area2 = (static_cast<long long>(triangle.v1.x - triangle.v0.x) * (triangle.v2.y - triangle.v0.y)) -
                     (static_cast<long long>(triangle.v1.y - triangle.v0.y) * (triangle.v2.x - triangle.v0.x));
  return area2 == 0;
}

bool isCrossingCandidate(const Edge& side, const Edge& constraintEdge, const std::unordered_set<Edge, EdgeHash>& constraints) {
  bool sharesEndpoint = side.v0 == constraintEdge.v0 || side.v0 == constraintEdge.v1 ||
                        side.v1 == constraintEdge.v0 || side.v1 == constraintEdge.v1;
  if (sharesEndpoint) {
    return false;
  }
  // Never flip away an edge that's itself a constraint -- it may already be
  // recovered (from an earlier contour edge), and this constraintEdge's own
  // recovery must not undo it.
  if (isConstraintEdge(side, constraints)) {
    return false;
  }
  return segmentsIntersect(side.v0, side.v1, constraintEdge.v0, constraintEdge.v1);
}

// One O(triangles) scan for every edge currently crossing constraintEdge.
// recoverConstraintEdge used to redo a scan like this after every single
// flip; a flip only ever touches 2 triangles (replacing one diagonal with
// another), so after the first scan it's cheaper to just patch the
// candidate list for those 2 triangles than to rescan the whole mesh again.
std::vector<Edge> findCrossingEdges(const std::vector<Triangle>& triangles, const Edge& constraintEdge,
                                     const std::unordered_set<Edge, EdgeHash>& constraints) {
  std::vector<Edge> candidates;

  for (const Triangle& triangle : triangles) {
    Edge sides[3] = {{triangle.v0, triangle.v1}, {triangle.v1, triangle.v2}, {triangle.v2, triangle.v0}};
    for (const Edge& side : sides) {
      if (!isCrossingCandidate(side, constraintEdge, constraints)) {
        continue;
      }
      if (std::find(candidates.begin(), candidates.end(), side) == candidates.end()) {
        candidates.push_back(side);
      }
    }
  }

  return candidates;
}

void recoverConstraintEdge(Edge constraintEdge, std::vector<Triangle>& triangles, EdgeAdjacency& adjacency,
                            const std::unordered_set<Edge, EdgeHash>& constraints) {
  if (edgePresent(constraintEdge, adjacency)) {
    return;
  }

  std::vector<Edge> candidates = findCrossingEdges(triangles, constraintEdge, constraints);

  const int maxIterations = static_cast<int>(triangles.size()) * 4 + 16;
  int guard = 0;

  while (!edgePresent(constraintEdge, adjacency)) {
    if (++guard > maxIterations) {
      return;
    }

    bool flipped = false;

    for (size_t c = 0; c < candidates.size(); ++c) {
      Edge side = candidates[c];

      auto it = adjacency.triangleIndicesByEdge.find(side);
      if (it == adjacency.triangleIndicesByEdge.end() || it->second.size() != 2) {
        // A previous flip in this loop removed this edge from the mesh.
        candidates.erase(candidates.begin() + static_cast<long>(c));
        --c;
        continue;
      }

      int indexA = it->second[0];
      int indexB = it->second[1];
      Vertex oppositeA = thirdVertex(triangles[indexA], side);
      Vertex oppositeB = thirdVertex(triangles[indexB], side);

      if (!isConvexQuad(side.v0, oppositeA, side.v1, oppositeB)) {
        continue; // illegal flip here -- try the next candidate instead
      }

      Triangle newA = {side.v0, oppositeA, oppositeB};
      Triangle newB = {side.v1, oppositeA, oppositeB};
      adjacency.updateTriangle(indexA, triangles[indexA], newA);
      adjacency.updateTriangle(indexB, triangles[indexB], newB);
      triangles[indexA] = newA;
      triangles[indexB] = newB;

      candidates.erase(candidates.begin() + static_cast<long>(c));

      // The flip retires `side` and introduces exactly one new edge, the
      // opposite diagonal (oppositeA,oppositeB) -- the other 4 edges of the
      // quad already existed before the flip, so the initial scan already
      // would have caught them if they qualified. Only the new diagonal can
      // possibly be a crossing candidate that wasn't seen before.
      Edge newDiagonal{oppositeA, oppositeB};
      if (isCrossingCandidate(newDiagonal, constraintEdge, constraints) &&
          std::find(candidates.begin(), candidates.end(), newDiagonal) == candidates.end()) {
        candidates.push_back(newDiagonal);
      }

      flipped = true;
      break;
    }

    if (!flipped) {
      return;
    }
  }
}

void legalizeNonConstraintEdges(std::vector<Triangle>& triangles, const std::unordered_set<Edge, EdgeHash>& constraints,
                                 EdgeAdjacency& adjacency) {
  // A worklist of edges that might not be locally Delaunay yet, instead of
  // restarting a full scan over every triangle after each single flip (the
  // previous version did exactly that -- O(triangles) work thrown away per
  // flip). This is the standard Lawson-flip-with-a-queue formulation:
  // provably equivalent to repeated full rescans, since a flip can only
  // possibly un-legalize the 4 edges bordering the quad it just changed, so
  // only those need re-checking afterward. Duplicate entries in the queue
  // are harmless (a redundant check is just a quick no-op), so there's no
  // need to de-duplicate it.
  std::vector<Edge> queue;
  for (const Triangle& triangle : triangles) {
    Edge sides[3] = {{triangle.v0, triangle.v1}, {triangle.v1, triangle.v2}, {triangle.v2, triangle.v0}};
    for (const Edge& side : sides) {
      if (!isConstraintEdge(side, constraints)) {
        queue.push_back(side);
      }
    }
  }

  const int maxOperations = static_cast<int>(triangles.size()) * 8 + 64;
  int guard = 0;

  while (!queue.empty()) {
    if (++guard > maxOperations) {
      return;
    }

    Edge side = queue.back();
    queue.pop_back();

    auto it = adjacency.triangleIndicesByEdge.find(side);
    if (it == adjacency.triangleIndicesByEdge.end() || it->second.size() != 2) {
      continue; // boundary edge, or already flipped away since being queued
    }

    int indexA = it->second[0];
    int indexB = it->second[1];
    Vertex oppositeA = thirdVertex(triangles[indexA], side);
    Vertex oppositeB = thirdVertex(triangles[indexB], side);

    bool needsFlip = triangles[indexA].inCircumcircle(oppositeB) || triangles[indexB].inCircumcircle(oppositeA);
    if (!needsFlip) {
      continue;
    }
    if (!isConvexQuad(side.v0, oppositeA, side.v1, oppositeB)) {
      continue;
    }

    Triangle newA = {side.v0, oppositeA, oppositeB};
    Triangle newB = {side.v1, oppositeA, oppositeB};
    adjacency.updateTriangle(indexA, triangles[indexA], newA);
    adjacency.updateTriangle(indexB, triangles[indexB], newB);
    triangles[indexA] = newA;
    triangles[indexB] = newB;

    Edge affected[4] = {{side.v0, oppositeA}, {side.v0, oppositeB}, {side.v1, oppositeA}, {side.v1, oppositeB}};
    for (const Edge& edge : affected) {
      if (!isConstraintEdge(edge, constraints)) {
        queue.push_back(edge);
      }
    }
  }
}

std::vector<Triangle> classifyAndStripExterior(const std::vector<Triangle>& triangles,
                                                const std::vector<Contour>& contours,
                                                const Triangle& superTriangle) {
  std::vector<Triangle> result;

  for (const Triangle& triangle : triangles) {
    if (sharesSuperTriangleVertex(triangle, superTriangle)) {
      continue;
    }
    if (isDegenerate(triangle)) {
      continue;
    }

    double centroidX = (triangle.v0.x + triangle.v1.x + triangle.v2.x) / 3.0;
    double centroidY = (triangle.v0.y + triangle.v1.y + triangle.v2.y) / 3.0;

    if (isInsideGlyph(contours, centroidX, centroidY)) {
      result.push_back(triangle);
    }
  }

  return result;
}

namespace {

bool sameVertexSet(const Triangle& a, const Triangle& b) {
  Vertex av[3] = {a.v0, a.v1, a.v2};
  Vertex bv[3] = {b.v0, b.v1, b.v2};
  bool used[3] = {false, false, false};
  for (const Vertex& vertex : av) {
    bool found = false;
    for (int j = 0; j < 3; ++j) {
      if (!used[j] && vertex == bv[j]) {
        used[j] = true;
        found = true;
        break;
      }
    }
    if (!found) {
      return false;
    }
  }
  return true;
}

} // namespace

std::vector<Triangle> triangulateConstrained(const std::vector<Contour>& contours, const std::vector<Vertex>& extraVertices,
                                              const std::vector<Edge>& extraConstraints,
                                              const std::vector<Triangle>& expectedTriangles) {
  std::vector<Vertex> vertices = flattenContours(contours);
  vertices.insert(vertices.end(), extraVertices.begin(), extraVertices.end());

  std::vector<Edge> constraints = buildConstraintEdges(contours);
  constraints.insert(constraints.end(), extraConstraints.begin(), extraConstraints.end());
  std::unordered_set<Edge, EdgeHash> constraintSet(constraints.begin(), constraints.end());

  Triangle superTriangle{};
  std::vector<Triangle> triangles = triangulateRaw(vertices, superTriangle);

  EdgeAdjacency adjacency;
  adjacency.build(triangles);

  for (const Edge& constraintEdge : constraints) {
    recoverConstraintEdge(constraintEdge, triangles, adjacency, constraintSet);
  }

  legalizeNonConstraintEdges(triangles, constraintSet, adjacency);

  std::vector<Triangle> result = classifyAndStripExterior(triangles, contours, superTriangle);

  // Control triangles (an off-curve point plus its two on-curve neighbors)
  // sit outside the on-curve fill polygon by design, so classifyAndStripExterior
  // just discarded them like any other exterior triangle. Add back whichever
  // of expectedTriangles actually exists in the raw mesh, matched by exact
  // vertex set -- not just "touches one of our extraVertices", since a
  // control point also ends up with ordinary exterior-fabric triangles that
  // neither of its two forced legs encloses, and those aren't genuine
  // control triangles.
  for (const Triangle& expected : expectedTriangles) {
    auto match = std::find_if(triangles.begin(), triangles.end(), [&](const Triangle& triangle) {
                    return sameVertexSet(triangle, expected);
                  });
    if (match == triangles.end()) {
      continue;
    }

    bool alreadyIncluded = std::find_if(result.begin(), result.end(), [&](const Triangle& kept) {
                              return sameVertexSet(kept, *match);
                            }) != result.end();
    if (!alreadyIncluded) {
      result.push_back(*match);
    }
  }

  return result;
}

std::vector<Triangle> triangulateConstrained(const std::vector<Contour>& contours) {
  return triangulateConstrained(contours, {}, {}, {});
}