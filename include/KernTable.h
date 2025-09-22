#pragma once
#include <vector>
#include <cstdint>
#include <variant>
#include "Helpers.h"

using namespace std;

struct kernCoverage {
    uint16_t kernVertical;
    uint16_t kernCrossStream;
    uint16_t kernVariation;
    uint16_t kernUnusedBits;
    uint16_t kernFormat;
    kernCoverage(uint16_t kVert, uint16_t kCross, uint16_t kVar, uint16_t kUnused, uint16_t kFormat) : 
                 kernVertical(kVert), kernCrossStream(kCross), kernVariation(kVar), kernUnusedBits(kUnused), kernFormat(kFormat) {}
};

class KernSubtable {
  friend class KernTable;

  public:
    KernSubtable(uint32_t length, kernCoverage coverage, uint16_t tupleIndex);
    
    void parseFormat0(const vector<char>& data, uint32_t offset);
    void parseFormat1(const vector<char>& data, uint32_t offset);
    void parseFormat2(const vector<char>& data, uint32_t offset);
    void parseFormat3(const vector<char>& data, uint32_t offset);

    struct KernFormat0 {
      struct Format0Pairs {
        uint16_t left = 0;
        uint16_t right = 0;
        int16_t value = 0;
      };
  
      uint16_t nPairs; // The number of kerning pairs in this subtable.
      uint16_t searchRange; // The largest power of two less than or equal to the value of nPairs, multiplied by the size in bytes of an entry in the subtable.
      uint16_t entrySelector; // This is calculated as log2 of the largest power of two less than or equal to the value of nPairs. This value indicates how many iterations of the search loop have to be made. For example, in a list of eight items, there would be three iterations of the loop.
      uint16_t rangeShift; // The value of nPairs minus the largest power of two less than or equal to nPairs. This is multiplied by the size in bytes of an entry in the table.
      vector<Format0Pairs> pairs; 
    };
  
    struct KernFormat1 {
      
    };
  
    struct KernFormat2 {
  
    };
  
    struct KernFormat3 {
  
    };
    

  private:
    uint32_t length;
    kernCoverage coverage;
    uint16_t tupleIndex;

    variant<KernFormat0, KernFormat1, KernFormat2, KernFormat3> formatData;
};

class KernTable {

  public:
    KernTable(uint32_t version, uint32_t nTables, vector<KernSubtable> kernSubtables);

    static KernTable parseKernDirectory(const vector<char>& data, uint32_t offset);

  private:
    uint32_t version;
    uint32_t nTables;
    vector<KernSubtable> kernSubtables;
};
