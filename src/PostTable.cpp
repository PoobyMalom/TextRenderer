#include "PostTable.h"
#include "Helpers.h"
#include "GlyphTable.h"
#include <vector>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <map>

PostTable::PostTable(
  uint32_t format,
  uint32_t italicAngle,
  int16_t underlinePosition,
  int16_t underlineThickness,
  uint32_t isFixedPitch,
  uint32_t minMemType42,
  uint32_t maxMemType42,
  uint32_t minMemType1,
  uint32_t maxMemType1
) : format(format),
    italicAngle(italicAngle),
    underlinePosition(underlinePosition),
    underlineThickness(underlineThickness),
    isFixedPitch(isFixedPitch),
    minMemType42(minMemType42),
    maxMemType42(maxMemType42),
    minMemType1(minMemType1),
    maxMemType1(maxMemType1) {}

PostTable PostTable::parsePostDirectory(const vector<char>& data, uint32_t postTableOffset) {
  int pos = postTableOffset;
  uint32_t format = static_cast<uint32_t>(read4Bytes(data, pos) >> 16);
  uint32_t italicAngle = static_cast<uint32_t>(read4Bytes(data, pos) >> 16);
  int16_t underlinePosition = static_cast<int16_t>(read2Bytes(data, pos));
  int16_t underlineThickness = static_cast<int16_t>(read2Bytes(data, pos));
  uint32_t isFixedPitch = read4Bytes(data, pos);
  uint32_t minMemType42 = read4Bytes(data, pos);
  uint32_t maxMemType42 = read4Bytes(data, pos);
  uint32_t minMemType1 = read4Bytes(data, pos);
  uint32_t maxMemType1 = read4Bytes(data, pos);
  
  PostTable table = PostTable(format, 
                              italicAngle, 
                              underlinePosition, 
                              underlineThickness, 
                              isFixedPitch,
                              minMemType42,
                              maxMemType42,
                              minMemType1,
                              maxMemType1);
    
  std::map<uint16_t, std::string> standardGlyphMap;
  loadStandardGlyphNamesMap("stdglyph.txt", standardGlyphMap);

  if (format == 1) {
    table.setGlyphNames(readAdobeGlyphList("stdglyph.txt"));
} else if (format == 2) {
    uint16_t numberOfGlyphs = read2Bytes(data, pos);
    vector<uint16_t> glyphNameIndexs;
    vector<string> names;
    vector<GlyphName> glyphs;

    for (uint16_t i = 0; i < numberOfGlyphs; i++) {
      uint16_t glyphIndex = read2Bytes(data, pos);
      glyphNameIndexs.push_back(glyphIndex);
    }

    int customNameCount = 0;
    for (uint16_t& index : glyphNameIndexs) {
        if (index >= 258) {
            customNameCount++;
        }
    }

    for (int i = 0; i < customNameCount; i++) {
        string name = readPascalString(data, pos);
        names.push_back(name);
    }

    int customNameIndex = 0;
    for (uint16_t i = 0; i < numberOfGlyphs; i++) {
        uint16_t index = glyphNameIndexs[i];
        string name;
        
        if (index < 258) {
            name = getStandardGlyphNameFast(index, standardGlyphMap);  // You'll need this function
        } else {
            name = names[customNameIndex];
            customNameIndex++;
        }
        
        glyphs.push_back(GlyphName(index, name));
    }

    table.setGlyphNames(glyphs);
  } else {
    cerr << "Unsupported Post Table Format" << endl;
  }
  //table.printPostTable();
  return table;
};

void PostTable::printPostTable() {
  cout << "Post Table Information" << endl;
  cout << "Format: " << format << endl;
  cout << "Italic Angle: " << italicAngle << endl;
  cout << "Underline Position (in FUnits): " << underlinePosition << endl;
  cout << "Underline Thickness (in FUnits): " << underlineThickness << endl;
  cout << "Is Fixed Pitch (anything other than 0 is monospaced): " << isFixedPitch << endl;
  cout << "Minimum Memory Usage for Type 42: " << minMemType42 << endl;
  cout << "Maximum Memory Usage for Type 42: " << maxMemType42 << endl; 
  cout << "Minimum Memory Usage for Type 1: " << minMemType1 << endl; 
  cout << "Maximum Memory Usage for Type 1: " << maxMemType1 << endl; 
  cout << "Glyph Index + Name" << endl;
  for (size_t i = 0; i < glyphNames.size(); i++) {
    cout << "Glyph index: " << glyphNames[i].index << ", name: " << glyphNames[i].name << endl;
  }
}