#include "GeometryUtils.h"

int toScreenX(int16_t x, double scale) {
    return static_cast<int>(x * scale + 100);
}

int toScreenY(int16_t y, double scale, int height) {
    return static_cast<int>(height - (y * scale));
}

bool isCollinear(Vertex* p0, Vertex* p1, Vertex* p2) {
  return abs((p2->y - p0->y) * (p1->x - p0->x) - (p1->y - p0->y) * (p2->x - p0->x)) < 1e-5;
}

float sign(Vertex* p1, Vertex* p2, Vertex* p3)
{
    return (p1->x - p3->x) * (p2->y - p3->y) - (p2->x - p3->x) * (p1->y - p3->y);
}

bool pointInTriangle (Vertex* pt, Vertex* v1, Vertex* v2, Vertex* v3)
{
    float d1, d2, d3;
    bool has_neg, has_pos;

    d1 = sign(pt, v1, v2);
    d2 = sign(pt, v2, v3);
    d3 = sign(pt, v3, v1);

    has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(has_neg && has_pos);
}

bool isCCW(Vertex* start) {
  int signedArea = 0;
  Vertex* cur = start->next;
  Vertex* next = cur->next;
  while (cur != start) {
    signedArea += (double)cur->x * next->y - (double)next->x * cur->y;
    cur = next;
    next = next->next;
  }

  return signedArea > 0;
}

bool isConvex(Vertex* i0, Vertex* i1, Vertex* i2, bool isCCW) {
  int64_t dx1 = i1->x - i0->x;
  int64_t dy1 = i1->y - i0->y;
  int64_t dx2 = i2->x - i1->x;
  int64_t dy2 = i2->y - i1->y;

  int64_t cross = dx1 * dy2 - dy1 * dx2;
  return isCCW ? (cross > 0) : (cross < 0);
}
