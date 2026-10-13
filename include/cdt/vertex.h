#pragma once

struct Vertex {
  int x;
  int y;

  bool operator==(const Vertex& other) const {
    return x == other.x && y == other.y;
  }

  Vertex operator-(const Vertex& other) const {
    return {other.x - x, other.y - y};
  }

  Vertex operator+(const Vertex& other) const {
    return {other.x + x, other.y + y};
  }

  double sumSquare() const {
    return static_cast<double>(x) * x + static_cast<double>(y) * y;
  }

  long long cross(const Vertex& other) const {
    return static_cast<long long>(x) * other.y -
           static_cast<long long>(y) * other.x;
  }
};