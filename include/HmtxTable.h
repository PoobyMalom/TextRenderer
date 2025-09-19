#pragma once
#include <vector>
#include <cstdint>
#include "Helpers.h"

using namespace std;

struct longHorMetric {
  uint16_t advanceWidth;
  int16_t leftSideBearing;

  longHorMetric(uint16_t aw, int16_t lsb) : advanceWidth(aw), leftSideBearing(lsb) {}
};

class HmtxTable {
  public:
    HmtxTable(
      vector<longHorMetric> hMetrics,
      vector<int16_t> leftSideBearings
    );

    vector<longHorMetric> getHMetrics() const { return hMetrics; };
    vector<int16_t> getLeftSideBearings() const { return leftSideBearings; };

    static HmtxTable parseHmtxDirectory(const vector<char>& data, 
                             uint32_t hmtxTableOffset, 
                             uint16_t numOfLongHorMetrics,
                             uint16_t numGlyphs);

  private:
    vector<longHorMetric> hMetrics;
    vector<int16_t> leftSideBearings;
};