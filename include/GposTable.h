#pragma once
#include <vector>
#include <cstdint>
#include "Helpers.h"

using namespace std;

class GposTable {
  public:
    static void parseGposDirectory(const vector<char>& data, uint32_t offset);

  private:
};