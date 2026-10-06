#include "Metrics.h"
#include "Helpers.h"
#include <algorithm>

HheaTable HheaTable::parseHheaDirectory(const std::vector<char>& data, uint16_t hheaTableOffset) {
  int pos = hheaTableOffset;
  uint32_t version = read4Bytes(data, pos);
  int16_t  ascent = static_cast<int16_t>(read2Bytes(data, pos));
  int16_t  descent = static_cast<int16_t>(read2Bytes(data, pos));
  int16_t  lineGap = static_cast<int16_t>(read2Bytes(data, pos));
  uint16_t advanceWidthMax = read2Bytes(data, pos);
  int16_t  minLeftSideBearing = static_cast<int16_t>(read2Bytes(data, pos));
  int16_t  minRightSideBearing = static_cast<int16_t>(read2Bytes(data, pos));
  int16_t  xMaxExtent = static_cast<int16_t>(read2Bytes(data, pos));
  int16_t  caretSlopeRise = static_cast<int16_t>(read2Bytes(data, pos));
  int16_t  caretSlopeRun = static_cast<int16_t>(read2Bytes(data, pos));
  int16_t  caretOffset = static_cast<int16_t>(read2Bytes(data, pos));
  pos = pos + 8;
  int16_t  metricDataFormat = static_cast<int16_t>(read2Bytes(data, pos));
  uint16_t numOfLongHorMetrics = read2Bytes(data, pos);

  return {
    version,
    ascent,
    descent,
    lineGap,
    advanceWidthMax,
    minLeftSideBearing,
    minRightSideBearing,
    xMaxExtent,
    caretSlopeRise,
    caretSlopeRun,
    caretOffset,
    metricDataFormat,
    numOfLongHorMetrics,
  };
}

const HheaTable& Metrics::getHheaTable() const { return hheaTable; }
const std::vector<longHorMetric>& Metrics::getLongHorMetrics() const { return longHorMetrics; }
const std::vector<int16_t>& Metrics::getLeftSideBearings() const { return leftSideBearings; }

void Metrics::getMetrics(const std::vector<char>& data, uint16_t hheaTableOffset, uint16_t hmtxTableOffset, uint16_t numGlyphs) {
  hheaTable = HheaTable::parseHheaDirectory(data, hheaTableOffset);

  int pos = hmtxTableOffset;

  // numOfLongHorMetrics must never exceed numGlyphs per the TTF spec, but
  // some fonts in the wild violate this (e.g. CraftyGirls-Regular.ttf reads
  // 5121 against only 2570 glyphs). Trusting it unconditionally turned a
  // negative "remaining" count into a huge size_t passed to reserve() below,
  // crashing with std::length_error -- clamp to both the spec-mandated cap
  // and to how many bytes are actually left in the buffer so a malformed
  // font can't make either read run past the end of the data.
  int longHorMetricCount = std::min<int>(hheaTable.numOfLongHorMetrics, numGlyphs);
  longHorMetricCount = std::max(0, std::min(longHorMetricCount, static_cast<int>(data.size() - pos) / 4));

  longHorMetrics.reserve(longHorMetricCount);
  for (int i = 0; i < longHorMetricCount; i++) {
    uint16_t aw = read2Bytes(data, pos);
    int16_t lsb = static_cast<int16_t>(read2Bytes(data, pos));
    longHorMetrics.push_back({aw, lsb});
  };

  int remaining = static_cast<int>(numGlyphs) - longHorMetricCount;
  remaining = std::max(0, std::min(remaining, static_cast<int>(data.size() - pos) / 2));
  leftSideBearings.reserve(remaining);

  for (int i = 0; i < remaining; i++) {
    leftSideBearings.push_back(static_cast<int16_t>(read2Bytes(data, pos)));
  }
}
