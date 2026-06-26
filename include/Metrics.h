#pragma once
#include <cstdint>
#include <vector>

using FWord = int16_t;
using UFWord = uint16_t;
using Fixed = uint32_t;

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

  const HheaTable& getHheaTable() const;
  const std::vector<longHorMetric>& getLongHorMetrics() const;
  const std::vector<int16_t>& getLeftSideBearings() const;

  void getMetrics(const std::vector<char>& data, uint16_t hheaTableOffset, uint16_t hmtxTableOffset, uint16_t numGlyphs);

private:
  HheaTable hheaTable;
  std::vector<longHorMetric> longHorMetrics;
  std::vector<int16_t> leftSideBearings;
};
