#ifndef VERTEX_LIST_H
#define VERTEX_LIST_H

#include "GeometryUtils.h"
#include <vector>

Vertex* createVertexList(const std::vector<int16_t> xCoordinates,
                                  const std::vector<int16_t> yCoordinates,
                                  const std::vector<uint8_t> flags);
Vertex* detachNode(Vertex* n);
void deleteVertexList(Vertex* head);

#endif