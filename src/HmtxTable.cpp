#include "HmtxTable.h"
#include <iostream>
#include <fstream>

HmtxTable::HmtxTable(
  vector<longHorMetric> hMetrics,
  vector<int16_t> leftSideBearings
) : hMetrics(hMetrics),
    leftSideBearings(leftSideBearings) {}

HmtxTable HmtxTable::parseHmtxDirectory(const vector<char>& data, uint32_t hmtxTableOffset, uint16_t numOfLongHorMetrics, uint16_t numGlyphs) {
  int pos = hmtxTableOffset;

  vector<longHorMetric> metrics;
  // cout << "Metrics" << endl;
  for (uint16_t i = 0; i < numOfLongHorMetrics; i++) {
    uint16_t advanceWidth = read2Bytes(data, pos);
    int16_t leftSideBearing = static_cast<int16_t>(read2Bytes(data, pos));
    // cout << "Advance Width: " << advanceWidth << ", Left Side Bearing: " << leftSideBearing << endl;
    metrics.emplace_back(longHorMetric(advanceWidth, leftSideBearing));
  }

  vector<int16_t> bearings;
  for (uint16_t i = 0; i < numGlyphs - numOfLongHorMetrics; i++) {
    bearings.emplace_back(static_cast<int16_t>(read2Bytes(data, pos)));
  }

  return HmtxTable(metrics, bearings);
}