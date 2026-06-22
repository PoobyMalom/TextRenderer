#pragma once
#include <vector>
#include <cstdint>
#include <stdexcept>

class LocaTable {
public:
    LocaTable() = default;
    static LocaTable parse(bool is32bit, const std::vector<char>& data, size_t locaOffset, size_t numGlyphs);

    const std::vector<uint32_t>& getOffsets() const;

private:
    std::vector<uint32_t> offsets;
};
