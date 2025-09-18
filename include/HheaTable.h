#pragma once
#include <vector>
#include <cstdint>
#include "Helpers.h"

using namespace std;

// Fixed	version	0x00010000 (1.0)
// FWord	ascent	Distance from baseline of highest ascender
// FWord	descent	Distance from baseline of lowest descender
// FWord	lineGap	typographic line gap
// uFWord	advanceWidthMax	must be consistent with horizontal metrics
// FWord	minLeftSideBearing	must be consistent with horizontal metrics
// FWord	minRightSideBearing	must be consistent with horizontal metrics
// FWord	xMaxExtent	max(lsb + (xMax-xMin))
// int16	caretSlopeRise	used to calculate the slope of the caret (rise/run) set to 1 for vertical caret
// int16	caretSlopeRun	0 for vertical
// FWord	caretOffset	set value to 0 for non-slanted fonts
// int16	reserved	set value to 0
// int16	reserved	set value to 0
// int16	reserved	set value to 0
// int16	reserved	set value to 0
// int16	metricDataFormat	0 for current format
// uint16	numOfLongHorMetrics	number of advance widths in metrics table

class HheaTable {
public:
  HheaTable(
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
  );

  double getVersion() const { return version; };
  int16_t getAscent() const { return ascent; };
  int16_t getDescent() const { return descent; };
  int16_t getLineGap() const { return lineGap; };
  uint16_t getAdvanceWidthMax() const { return advanceWidthMax; };
  int16_t getMinLeftSideBearing() const { return minLeftSideBearing; };
  int16_t getMinRightSideBearing() const { return minRightSideBearing; };
  int16_t getXMaxExtent() const { return xMaxExtent; };
  int16_t getCaretSlopeRise() const { return caretSlopeRise; };
  int16_t getCaretSlopeRun() const { return caretSlopeRun; };
  int16_t getCaretOffset() const { return caretOffset; };
  int16_t getMetricDataFormat() const { return metricDataFormat; };
  uint16_t getNumOfLongHorMetrics() const { return numOfLongHorMetrics; };

  static HheaTable parseHheaDirectory(const vector<char>& data, uint16_t hheaTableOffset);
  void printHheaDirectory();

private:
  double version;
  int16_t ascent;
  int16_t descent;
  int16_t lineGap;
  uint16_t advanceWidthMax;
  int16_t minLeftSideBearing;
  int16_t minRightSideBearing;
  int16_t xMaxExtent;
  int16_t caretSlopeRise;
  int16_t caretSlopeRun;
  int16_t caretOffset;
  int16_t metricDataFormat;
  uint16_t numOfLongHorMetrics;
};