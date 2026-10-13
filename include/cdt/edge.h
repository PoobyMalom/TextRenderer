#pragma once

#include "vertex.h"

struct Edge {
  Vertex v0;
  Vertex v1;

  bool operator==(const Edge& other) const {
    return (v0 == other.v0 && v1 == other.v1) || (v0 == other.v1 && v1 == other.v0);
  }
};