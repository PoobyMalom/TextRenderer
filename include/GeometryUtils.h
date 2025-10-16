#ifndef GEOMETRY_UTILS_H
#define GEOMETRY_UTILS_H

#include <cstdint>
#include <cmath>

struct Vertex {
  int16_t x;
  int16_t y;
  uint8_t flag;
  Vertex* prev;
  Vertex* next;

  Vertex(int16_t _x, int16_t _y, uint8_t _flag) : x(_x), y(_y), flag(_flag), prev(nullptr), next(nullptr) {}
};

struct Triangle {
  int16_t x1, y1;
  int16_t x2, y2;
  int16_t x3, y3;
};

// Coordindate conversion
int toScreenX(int16_t x, double scale);
int toScreenY(int16_t y, double scale, int height);

// Geometry calculations
bool isCollinear(Vertex* p0, Vertex* p1, Vertex* p2);
float sign(Vertex* p0, Vertex* p1, Vertex* p2);
bool pointInTriangle(Vertex* pt, Vertex* p0, Vertex* p1, Vertex* p2);
bool isCCW(Vertex* start);
bool isConvex(Vertex* p0, Vertex* p1, Vertex* i2, bool isCCW);

#endif 