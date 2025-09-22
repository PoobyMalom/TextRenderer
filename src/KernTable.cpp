#include "KernTable.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>

using namespace std;

KernSubtable::KernSubtable(uint32_t length, kernCoverage coverage, uint16_t tupleIndex) :
                           length(length), coverage(coverage), tupleIndex(tupleIndex) {}

KernTable KernTable::parseKernDirectory(const vector<char>& data, uint32_t offset) {
  int pos = offset;
  uint16_t version = read2Bytes(data, pos);
  uint16_t nTables = read2Bytes(data, pos);
  vector<KernSubtable> subtables;

  for (uint16_t i = 0; i  < nTables; i++) {
    uint32_t length = read4Bytes(data, pos);
    uint16_t coverage = read2Bytes(data, pos);
    uint16_t tupleIndex = read2Bytes(data, pos);
    cout << "Kern Subtable #" << i+1 << " length: " << length << ", coverage: " << bitset<16>(coverage) << ", tupleIndex: " << tupleIndex << endl;
    uint16_t kernVertical = coverage & 0x8000;
    uint16_t kernCrossStream = coverage & 0x4000;
    uint16_t kernVariation = coverage & 0x2000;
    uint16_t kernUnusedBits = coverage & 0x1F00;
    uint16_t kernFormat = coverage & 0x00FF;
    cout << "Coverage Details " << "kernVertical: " << kernVertical 
                                << ", kernCrossStream: " << kernCrossStream 
                                << ", kernVariation: " << kernVariation
                                << ", kernFormat: " << kernFormat << endl;
    kernCoverage kernCov = kernCoverage(kernVertical, kernCrossStream, kernVariation, kernUnusedBits, kernFormat);
    KernSubtable kernSubtable = KernSubtable(length, kernCov, tupleIndex);

    subtables.emplace_back(KernSubtable(length, kernCov, tupleIndex));
  }

  for (KernSubtable table : subtables) {
    switch (table.coverage.kernFormat) {
      case 0:
        table.parseFormat0(data, pos);
        break;
      case 1:
        table.parseFormat1(data, pos);
        break;
      case 2:
        table.parseFormat2(data, pos);
        break;
      case 3:
        table.parseFormat3(data, pos);
        break;
      default:
        cerr << "Unknown kern subtable format: " << table.coverage.kernFormat << endl;
    }
  }
  return KernTable(version, nTables, subtables);
}

KernTable::KernTable(uint32_t version,
                     uint32_t nTables,
                     vector<KernSubtable> kernSubtables) :
                     version(version),
                     nTables(nTables),
                     kernSubtables(kernSubtables) {}

void KernSubtable::parseFormat0(const vector<char>& data, uint32_t offset) {
  int pos = offset;
  uint16_t nPairs = read2Bytes(data, pos);
  uint16_t searchRange = read2Bytes(data, pos); 
  uint16_t entrySelector = read2Bytes(data, pos);
  uint16_t rangeShift = read2Bytes(data, pos); 
  cout << "Subtable Data, Format: 0, nPairs: " << nPairs << ", searchRange: " << searchRange << ", entrySelector: " << entrySelector << ", rangeShift: " << rangeShift << endl;
}

void KernSubtable::parseFormat1(const vector<char>& data, uint32_t offset) {
  cout << "Not Done Yet" << endl;
}

void KernSubtable::parseFormat2(const vector<char>& data, uint32_t offset) {
  cout << "Not Done Yet" << endl;
}

void KernSubtable::parseFormat3(const vector<char>& data, uint32_t offset) {
  cout << "Not Done Yet" << endl;
}