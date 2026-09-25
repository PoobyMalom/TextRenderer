#pragma once
#include <cstdint>
#include <vector>
#include "FontTypes.h"

struct HheaTable {
  Fixed version;
  FWord  ascent;
  FWord  descent;
  FWord  lineGap;
  UFWord advanceWidthMax;
  FWord  minLeftSideBearing;
  FWord  minRightSideBearing;
  FWord  xMaxExtent;
  int16_t  caretSlopeRise;
  int16_t  caretSlopeRun;
  FWord  caretOffset;
  int16_t  metricDataFormat;
  uint16_t numOfLongHorMetrics;

  static HheaTable parseHheaDirectory(const std::vector<char>& data, uint16_t hheaTableOffset);
};

struct longHorMetric {
  uint16_t advanceWidth;
  int16_t leftSideBearing;
};

class Metrics {
public:
  Metrics() : hheaTable{}, longHorMetrics{}, leftSideBearings{} {}

  const HheaTable& getHheaTable() const;
  const std::vector<longHorMetric>& getLongHorMetrics() const;
  const std::vector<int16_t>& getLeftSideBearings() const;

  void getMetrics(const std::vector<char>& data, uint16_t hheaTableOffset, uint16_t hmtxTableOffset, uint16_t numGlyphs);

private:
  HheaTable hheaTable;
  std::vector<longHorMetric> longHorMetrics;
  std::vector<int16_t> leftSideBearings;
};

// See docs/coordinates.md for the coordinate system conventions.
struct FontTransform {
  float pixelsPerEm;
  UFWord unitsPerEm;

  float toPixels(int16_t fontUnits) const {
    return fontUnits * pixelsPerEm / unitsPerEm;
  }

  // TTF is Y-up; screen/atlas is Y-down. baselineY is the pixel row of the baseline.
  int toScreenY(int16_t fontUnits, int baselineY) const {
    return baselineY - static_cast<int>(toPixels(fontUnits));
  }
};
