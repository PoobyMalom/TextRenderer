#pragma once

#include "edge.h"
#include "triangle.h"
#include <algorithm>
#include <functional>
#include <tuple>
#include <unordered_map>
#include <vector>

// Edge::operator== treats (v0,v1) and (v1,v0) as equal, so the hash must
// too -- hash a canonical (smaller-first) ordering of the two endpoints,
// otherwise the same logical edge could land in two different buckets.
struct EdgeHash {
  size_t operator()(const Edge& edge) const {
    Vertex a = edge.v0;
    Vertex b = edge.v1;
    if (std::tie(b.x, b.y) < std::tie(a.x, a.y)) {
      std::swap(a, b);
    }
    size_t seed = 0;
    for (int value : {a.x, a.y, b.x, b.y}) {
      seed ^= std::hash<int>()(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};

struct EdgeAdjacency {
  std::unordered_map<Edge, std::vector<int>, EdgeHash> triangleIndicesByEdge;

  void build(const std::vector<Triangle>& triangles) {
    // Without this, every re-build appends on top of stale entries from the
    // mesh's PREVIOUS state instead of replacing them -- edges that used to
    // be interior (2 owners) accumulate extra stale indices every rebuild,
    // corrupting both the size()==2 boundary check and neighbors()'s results.
    triangleIndicesByEdge.clear();

    for (size_t i = 0; i < triangles.size(); i++) {
      const Triangle& triangle = triangles[i];
      Edge sides[3] = {{triangle.v0, triangle.v1}, {triangle.v1, triangle.v2}, {triangle.v2, triangle.v0}};
      for (const Edge& side : sides) {
        triangleIndicesByEdge[side].push_back(static_cast<int>(i));
      }
    }
  }

  // Updates just the one triangle at `index` from its old geometry to its
  // new one, in O(1) amortized instead of rebuilding the whole map -- used
  // after a single edge flip, where only two triangles actually changed.
  void updateTriangle(int index, const Triangle& oldTriangle, const Triangle& newTriangle) {
    Edge oldSides[3] = {{oldTriangle.v0, oldTriangle.v1}, {oldTriangle.v1, oldTriangle.v2}, {oldTriangle.v2, oldTriangle.v0}};
    for (const Edge& side : oldSides) {
      auto it = triangleIndicesByEdge.find(side);
      if (it == triangleIndicesByEdge.end()) {
        continue;
      }
      std::vector<int>& owners = it->second;
      owners.erase(std::remove(owners.begin(), owners.end(), index), owners.end());
      if (owners.empty()) {
        triangleIndicesByEdge.erase(it);
      }
    }

    Edge newSides[3] = {{newTriangle.v0, newTriangle.v1}, {newTriangle.v1, newTriangle.v2}, {newTriangle.v2, newTriangle.v0}};
    for (const Edge& side : newSides) {
      triangleIndicesByEdge[side].push_back(index);
    }
  }

  std::vector<int> neighbors(int triangleIndex, const std::vector<Triangle> triangles) {
    std::vector<int> result = {};
    Triangle triangle = triangles[triangleIndex];
    std::vector<Edge> edges = {{triangle.v0, triangle.v1}, {triangle.v1, triangle.v2}, {triangle.v2, triangle.v0}};

    for (Edge edge : edges) {
      std::vector<int> owners = triangleIndicesByEdge[edge];
      for (int ownerIndex : owners) {
        if (ownerIndex != triangleIndex) {
          result.push_back(ownerIndex);
        }
      }
    }

    return result;
  }
};