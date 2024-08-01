#pragma once
#include <vector>
#include <stdexcept>

class LocaTable {
public:
    static LocaTable parse(bool is32bit, const std::vector<char>& data, size_t locaOffset, size_t numGlyphs);

    const std::vector<uint32_t>& getOffsets32() const;
    const std::vector<uint16_t>& getOffsets16() const;

private:
    bool is32bitFormat;
    std::vector<uint32_t> offsets32;
    std::vector<uint16_t> offsets16;
};
