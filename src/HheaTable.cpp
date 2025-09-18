#include "HheaTable.h"
#include "Helpers.h"
#include <vector>
#include <cstdint>
#include <iostream>
#include <fstream>

HheaTable::HheaTable(
    double version,
    int16_t ascent,
    int16_t descent,
    int16_t lineGap,
    uint16_t advanceWidthMax,
    int16_t minLeftSideBearing,
    int16_t minRightSideBearing,
    int16_t xMaxExtent,
    int16_t caretSlopeRise,
    int16_t caretSlopeRun,
    int16_t caretOffset,
    int16_t metricDataFormat,
    uint16_t numOfLongHorMetrics
) :  version(version),
     ascent(ascent),
     descent(descent),
     lineGap(lineGap),
     advanceWidthMax(advanceWidthMax),
     minLeftSideBearing(minLeftSideBearing),
     minRightSideBearing(minRightSideBearing),
     xMaxExtent(xMaxExtent),
     caretSlopeRise(caretSlopeRise),
     caretSlopeRun(caretSlopeRun),
     caretOffset(caretOffset),
     metricDataFormat(metricDataFormat),
     numOfLongHorMetrics(numOfLongHorMetrics) {}

HheaTable HheaTable::parseHheaDirectory(const std::vector<char>& data, uint16_t hheaTableOffset) {
  int pos = hheaTableOffset;
  uint32_t version_u = read4Bytes(data, pos);
  int32_t version_raw = static_cast<int32_t>(version_u);
  double version = version_raw / 65536.0;
  int16_t ascent = static_cast<int16_t>(read2Bytes(data, pos));
  int16_t descent = static_cast<int16_t>(read2Bytes(data, pos));
  int16_t lineGap = static_cast<int16_t>(read2Bytes(data, pos));
  uint16_t advanceWidthMax = read2Bytes(data, pos);
  int16_t minLeftSideBearing = static_cast<int16_t>(read2Bytes(data, pos));
  int16_t minRightSideBearing = static_cast<int16_t>(read2Bytes(data, pos));
  int16_t xMaxExtent = static_cast<int16_t>(read2Bytes(data, pos));
  int16_t caretSlopeRise = static_cast<int16_t>(read2Bytes(data, pos));
  int16_t caretSlopeRun = static_cast<int16_t>(read2Bytes(data, pos));
  int16_t caretOffset = static_cast<int16_t>(read2Bytes(data, pos));
  read8Bytes(data, pos);
  int16_t metricDataFormat = static_cast<int16_t>(read2Bytes(data, pos));
  uint16_t numOfLongHorMetrics = read2Bytes(data, pos);

  return HheaTable(
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
    numOfLongHorMetrics
  );
}

void HheaTable::printHheaDirectory() {
  puts("-------------------------------------------------");
  puts("Hhea Table Information");
  printf("Version: %f\n", version);
  printf("Distance from baseline of highest ascender (ascent): %d\n", ascent);
  printf("Distance from baseline of lowest descender: %d\n", descent);
  printf("Line Gap: %d\n", lineGap);
  printf("Advanced Width Max: %u\n", advanceWidthMax);
  printf("Min Left Side Bearing: %d\n", minLeftSideBearing);
  printf("Min Right Side Bearing: %d\n", minRightSideBearing);
  printf("X Max Extent: %d\n", xMaxExtent);
  printf("Caret Slope Rise: %d\n", caretSlopeRise);
  printf("Caret Slope Run: %d\n", caretSlopeRun);
  printf("Caret Offset: %d\n", caretOffset);
  printf("Metric Data Format: %d\n", metricDataFormat);
  printf("Num of Advanced Widths in metric trable: %u\n", numOfLongHorMetrics);
  puts("-------------------------------------------------");
}