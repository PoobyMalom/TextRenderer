#pragma once
#include <vector>
#include <cstdint>
#include "Helpers.h"
#include "GlyphTable.h"

using namespace std;

class PostTable {
  public:
    PostTable(
      uint32_t format,
      uint32_t italicAngle,
      int16_t underlinePosition,
      int16_t underlineThickness,
      uint32_t isFixedPitch,
      uint32_t minMemType42,
      uint32_t maxMemType42,
      uint32_t minMemType1,
      uint32_t maxMemType1
    );

    uint32_t getFormat() const { return format; };
    uint32_t getItalicAngle() const { return italicAngle; };
    int16_t getUnderlinePosition() const { return underlinePosition; };
    int16_t getUnderlineThickness() const { return underlineThickness; };
    uint32_t getIsFixedPitch() const { return isFixedPitch; };
    uint32_t getMinMemType42() const { return minMemType42; };
    uint32_t getMaxMemType42() const { return maxMemType42; };
    uint32_t getMinMemType1() const { return minMemType1; };
    uint32_t getMaxMemType1() const { return maxMemType1; };
    vector<GlyphName> getGlyphNames() const { return glyphNames; };
    void setGlyphNames(vector<GlyphName> glyphs) { glyphNames = glyphs; }; 

    static PostTable parsePostDirectory(const vector<char>& data, uint32_t postTableOffset);
    void printPostTable();

  private:
    uint32_t format;
    uint32_t italicAngle;
    int16_t underlinePosition;
    int16_t underlineThickness;
    uint32_t isFixedPitch;
    uint32_t minMemType42;
    uint32_t maxMemType42;
    uint32_t minMemType1;
    uint32_t maxMemType1;
    vector<GlyphName> glyphNames;
};