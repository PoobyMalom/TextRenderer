# TextRenderer — Testing Plan

## Philosophy

The codebase has two distinct layers that need different testing strategies:

1. **Parsing layer** — pure data transformation (bytes → structs). Fully deterministic, no I/O, no SDL. These are straightforward unit tests with crafted byte buffers and known expected values.
2. **Rendering layer** — SDL2-dependent drawing calls. Hard to unit test directly; covered by integration smoke tests and visual inspection.

The goal is to get the parsing layer under test first, since that's where all the correctness bugs live. Rendering tests come later once the data is trustworthy.

---

## Recommended Framework: Google Test (gtest)

```bash
# Ubuntu/Debian
sudo apt install libgtest-dev

# Or via CMake FetchContent (no install needed)
```

Add to `makefile`:
```makefile
GTEST_FLAGS := $(shell pkg-config --cflags gtest_main)
GTEST_LIBS  := $(shell pkg-config --libs gtest_main)

TEST_SRCS := tests/test_helpers.cpp tests/test_ttftable.cpp tests/test_headtable.cpp \
             tests/test_maxptable.cpp tests/test_locatable.cpp tests/test_cmaptable.cpp \
             tests/test_glyphtable.cpp tests/test_ttffile.cpp \
             src/Helpers.cpp src/TTFTable.cpp src/HeadTable.cpp src/MaxpTable.cpp \
             src/LocaTable.cpp src/CmapTable.cpp src/GlyphTable.cpp src/TTFFile.cpp \
             src/MovablePoint.cpp src/SDLInitializer.cpp

test: $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) $(GTEST_FLAGS) $^ -o test_runner $(LDFLAGS) $(GTEST_LIBS)
	./test_runner
```

---

## File Structure

```
TextRenderer/
├── tests/
│   ├── test_helpers.cpp
│   ├── test_ttftable.cpp
│   ├── test_headtable.cpp
│   ├── test_maxptable.cpp
│   ├── test_locatable.cpp
│   ├── test_cmaptable.cpp
│   ├── test_glyphtable.cpp
│   ├── test_ttffile.cpp       ← integration tests (loads real .ttf)
│   └── fixtures/
│       └── buffer_helpers.h   ← shared helper for crafting byte buffers
```

---

## Shared Test Helpers (`tests/fixtures/buffer_helpers.h`)

Most unit tests need to craft raw byte buffers that look like TTF binary data. A small helper makes this readable:

```cpp
#pragma once
#include <vector>
#include <cstdint>
#include <cstring>

// Append a big-endian uint8 to a buffer
inline void pushU8(std::vector<char>& buf, uint8_t v) {
    buf.push_back(static_cast<char>(v));
}

// Append a big-endian uint16
inline void pushU16(std::vector<char>& buf, uint16_t v) {
    buf.push_back(static_cast<char>(v >> 8));
    buf.push_back(static_cast<char>(v & 0xFF));
}

// Append a big-endian uint32
inline void pushU32(std::vector<char>& buf, uint32_t v) {
    buf.push_back(static_cast<char>((v >> 24) & 0xFF));
    buf.push_back(static_cast<char>((v >> 16) & 0xFF));
    buf.push_back(static_cast<char>((v >>  8) & 0xFF));
    buf.push_back(static_cast<char>((v      ) & 0xFF));
}

// Append a big-endian int16 (signed)
inline void pushS16(std::vector<char>& buf, int16_t v) {
    pushU16(buf, static_cast<uint16_t>(v));
}

// Append a big-endian int64
inline void pushS64(std::vector<char>& buf, int64_t v) {
    for (int i = 7; i >= 0; --i)
        buf.push_back(static_cast<char>((v >> (i * 8)) & 0xFF));
}

// Pad buffer to a multiple of 4 bytes
inline void padTo4(std::vector<char>& buf) {
    while (buf.size() % 4 != 0) buf.push_back(0);
}
```

---

## Unit Tests by Module

---

### `tests/test_helpers.cpp` — Helpers

#### Endian Conversion

```
TEST: convertEndian16 known value
  convertEndian16(0x1234) == 0x3412

TEST: convertEndian16 round-trip
  convertEndian16(convertEndian16(x)) == x  for any x

TEST: convertEndian32 known value
  convertEndian32(0x12345678) == 0x78563412

TEST: convertEndian32 round-trip

TEST: convertEndian64 known value
  convertEndian64(0x0102030405060708ULL) == 0x0807060504030201ULL

TEST: convertEndian64 round-trip
```

#### Read Functions

```
TEST: readByte reads correct value and advances offset
  buffer = {0xAB, 0xCD}
  offset = 0
  readByte(buf, offset) == 0xAB
  offset == 1

TEST: read2Bytes reads big-endian uint16 and advances by 2
  buffer = {0x12, 0x34}
  read2Bytes(buf, offset) == 0x1234
  offset == 2

TEST: read4Bytes reads big-endian uint32 and advances by 4
  buffer = {0x00, 0x01, 0x00, 0x00}
  read4Bytes(buf, offset) == 0x00010000

TEST: read8Bytes reads big-endian uint64 and advances by 8

TEST: sequential reads advance offset correctly
  buffer = {0x12, 0x34, 0x56, 0x78, 0x9A}
  read2Bytes at 0 -> 0x1234, offset=2
  read2Bytes at 2 -> 0x5678, offset=4
  readByte  at 4 -> 0x9A,   offset=5
```

#### stringToUnicode

```
TEST: ASCII string
  stringToUnicode("ABC") == {0x41, 0x42, 0x43}

TEST: single ASCII character
  stringToUnicode("A") == {0x41}

TEST: empty string
  stringToUnicode("") == {}

TEST: 2-byte UTF-8 character (U+00E9 é = 0xC3 0xA9)
  stringToUnicode("\xC3\xA9") == {0xE9}

TEST: 3-byte UTF-8 character (U+4E2D 中 = 0xE4 0xB8 0xAD)
  stringToUnicode("\xE4\xB8\xAD") == {0x4E2D}

TEST: 4-byte UTF-8 character (U+1F600 😀 = 0xF0 0x9F 0x98 0x80)
  stringToUnicode("\xF0\x9F\x98\x80") == {0x1F600}

TEST: mixed ASCII and multi-byte
  stringToUnicode("A\xC3\xA9") == {0x41, 0xE9}

TEST: invalid UTF-8 byte throws runtime_error
  stringToUnicode("\xFF") throws std::runtime_error
```

#### getBezierPoint

```
TEST: t=0 returns first point
  getBezierPoint({0,0}, {50,100}, {100,0}, 0.0f) == {0, 0}

TEST: t=1 returns last point
  getBezierPoint({0,0}, {50,100}, {100,0}, 1.0f) == {100, 0}

TEST: t=0.5 on symmetric curve is at apex
  getBezierPoint({0,0}, {50,100}, {100,0}, 0.5f) == {50, 50}
  (formula: 0.25*p1 + 0.5*cp + 0.25*p2)
```

#### CalcTableChecksum

```
TEST: 4-byte aligned data, known sum
  data = {0x00, 0x01, 0x00, 0x00}  (big-endian word = 0x00010000)
  CalcTableChecksum(data, 0, 4) == 0x00010000

TEST: two 4-byte words
  words are 0x00010000 and 0x00020000
  sum == 0x00030000

TEST: sum wraps modulo 2^32
  two words that sum > 0xFFFFFFFF, verify correct truncation

TEST: 3-byte trailing data (zero-padded)
  data bytes: {0x01, 0x02, 0x03}
  treated as big-endian word 0x01020300
  CalcTableChecksum(data, 0, 3) == 0x01020300

TEST: out-of-range offset throws std::out_of_range
  CalcTableChecksum(data, data.size() + 1, 1) throws

TEST: offset + length exceeds data.size() throws std::out_of_range
```

---

### `tests/test_ttftable.cpp` — TTFTable

#### parseTableDirectory

Craft a buffer with the 12-byte offset subtable followed by `n` 16-byte table records.
Each record: 4-byte tag + 4-byte checksum + 4-byte offset + 4-byte length.

```
TEST: parse one table entry
  buffer: 12 bytes of offset subtable header (numTables=1) +
          tag="head", checksum=0, offset=0x1000, length=0x36
  result: one TTFTable with correct tag/checksum/offset/length

TEST: parse multiple table entries
  buffer with numTables=3: "head", "maxp", "glyf"
  result: 3 TTFTable objects with correct tags

TEST: tag is exactly 4 bytes
  tag stored as "cmap" → getTag() == "cmap"

TEST: getOffset and getLength return correct values
```

---

### `tests/test_headtable.cpp` — HeadTable

Craft a 54-byte head table buffer (big-endian, all fields):

```
Field layout (bytes):
  version          4  (0x00010000)
  fontRevision     4
  checkSumAdj      4
  magicNumber      4  (0x5F0F3CF5)
  flags            2
  unitsPerEm       2  (e.g. 2048)
  created          8
  modified         8
  xMin             2  (signed)
  yMin             2  (signed)
  xMax             2
  yMax             2
  macStyle         2
  lowestRecPPEM    2
  fontDirHint      2
  indexToLocFormat 2  (0=16bit, 1=32bit)
  glyphDataFormat  2
```

```
TEST: all fields parsed correctly at offset 0
  Set known values for each field, verify all getters

TEST: indexToLocFormat=0 (16-bit loca)
  getIndexToLocFormat() == 0

TEST: indexToLocFormat=1 (32-bit loca)
  getIndexToLocFormat() == 1

TEST: signed fields (xMin, yMin) with negative values
  xMin stored as 0xFF9C (= -100 in two's complement)
  getXMIN() == -100

TEST: parse at non-zero offset (verify offset parameter is respected)
  Pad buffer with 16 bytes of zeros before the head data
  parseHeadDirectory(buf, 16) returns correct values
```

---

### `tests/test_maxptable.cpp` — MaxpTable

Craft a 32-byte maxp table buffer (version 0x00010000 + 15 uint16 fields):

```
TEST: all fields parsed correctly
  Set known distinct values for all 15 fields, verify all getters

TEST: numGlyphs parsed correctly
  numGlyphs=500 → getNumGlyphs() == 500

TEST: parse at non-zero offset
```

---

### `tests/test_locatable.cpp` — LocaTable

#### 16-bit format (indexToLocFormat == 0)

```
TEST: parse 16-bit offsets for 3 glyphs (4 entries including sentinel)
  raw bytes: 0x0000, 0x0010, 0x0030, 0x0050
  getOffsets16() == {0, 16, 48, 80}
  (Note: TTF spec says 16-bit loca values must be multiplied by 2 at read time)

TEST: getOffsets32() throws when format is 16-bit

TEST: offsets16 has numGlyphs+1 entries
```

#### 32-bit format (indexToLocFormat == 1)

```
TEST: parse 32-bit offsets for 3 glyphs (4 entries)
  raw bytes: 0x00000000, 0x00001234, 0x00005678, 0x00009ABC
  getOffsets32() == {0, 0x1234, 0x5678, 0x9ABC}

TEST: getOffsets16() throws when format is 32-bit

TEST: offsets32 has numGlyphs+1 entries
```

---

### `tests/test_cmaptable.cpp` — CmapTable / CmapSubtable

This is the most important module to test since Format 4 has a known parsing bug (TODO B6).
After fixing B6, add these tests to lock in the correct behavior.

#### Format 0 (Macintosh Roman)

```
TEST: getGlyphIndex for Format 0
  Craft a Format 0 subtable with 256 entries
  glyphIndexArray[0x41] = 5  (character 'A' → glyph 5)
  getGlyphIndex(0x41) == 5

TEST: getGlyphIndex(0) returns glyphIndexArray[0]
```

#### Format 4 (BMP Unicode — most common)

Craft a minimal Format 4 subtable mapping 'A'–'Z' (0x0041–0x005A) to glyphs 1–26.
Use idDelta and idRangeOffset=0 for a simple linear segment.

```
TEST: single segment with idRangeOffset=0, known delta
  One segment: startCode=0x0041, endCode=0x005A, idDelta=(-0x0040), idRangeOffset=0
  getGlyphIndex(0x0041) == 1   ('A')
  getGlyphIndex(0x005A) == 26  ('Z')

TEST: character outside all segments returns 0 (or throws after B5 fix)
  getGlyphIndex(0x0100) == 0

TEST: segment with idRangeOffset != 0 (glyphIdArray lookup)
  Craft a segment using the indirect lookup path
  Verify the glyphIdArray is indexed correctly

TEST: terminator segment (endCode=0xFFFF) is handled
```

#### Format 12 (Full Unicode)

```
TEST: single group maps characters to glyphs
  group: startCharCode=0x0041, endCharCode=0x005A, startGlyphCode=1
  getGlyphIndex(0x0041) == 1
  getGlyphIndex(0x005A) == 26

TEST: character between two groups returns 0

TEST: multiple groups — correct group is selected
```

#### CmapTable Subtable Priority

```
TEST: Format 12 (platformID=3, encodingID=10) takes priority over Format 4
  Table with both Format 12 and Format 4; same codepoint mapped differently in each
  getGlyphIndex uses Format 12 result

TEST: Format 4 (platformID=3, encodingID=1) used when Format 12 absent

TEST: Format 0 (platformID=1, encodingID=0) used as last resort

TEST: no matching subtable — throws or returns 0 (after B5 fix, should throw)
```

---

### `tests/test_glyphtable.cpp` — Glyph

#### parseSimpleGlyph

Craft minimal binary data for a simple triangle (3 points, 1 contour, all on-curve):

```
numberOfContours = 1
xMin, yMin, xMax, yMax = 0, 0, 100, 100
endPtsOfContours = [2]
instructionLength = 0
flags = [0x01, 0x01, 0x01]  (all on-curve)
xCoordinates (delta-encoded): [100, 0, -100]  → absolute: [100, 100, 0]
yCoordinates (delta-encoded): [0, 100, 0]     → absolute: [0, 100, 100]
```

```
TEST: numberOfContours == 1
TEST: xCoordinates correctly decoded from deltas == {100, 100, 0}
TEST: yCoordinates correctly decoded from deltas == {0, 100, 100}
TEST: flags == {0x01, 0x01, 0x01}
TEST: endPtsOfContours == {2}
TEST: instructionLength == 0

TEST: short-vector x coordinate with positive flag (flag & 0x02 set, flag & 0x10 set)
  delta = 50 → currentX += 50

TEST: short-vector x coordinate with negative flag (flag & 0x02 set, flag & 0x10 clear)
  delta = 50 → currentX -= 50

TEST: repeat flag expands correctly
  flag = 0x09 (on-curve + repeat), repeatCount = 2
  results in 3 identical flag entries
```

#### parseGlyph dispatch

```
TEST: positive numberOfContours → calls parseSimpleGlyph (returns Glyph with numberOfContours > 0)
TEST: zero numberOfContours → also calls parseSimpleGlyph (zero-contour empty glyph)
TEST: negative numberOfContours (-1) → calls parseCompoundGlyph (returns Glyph with numberOfContours == -1)
```

#### addPointsBetween

This is the core TrueType implicit-point-insertion algorithm.

```
TEST: two consecutive off-curve points → midpoint on-curve inserted between them
  input:  flags = {0, 0}  coords = {(0,0), (100,100)}
  output: flags = {0, 1, 0}  coords = {(0,0), (50,50), (100,100)}

TEST: first and last both off-curve (wrap-around case) → midpoint inserted at contour end
  single contour with last and first point both off-curve
  on-curve midpoint appended before contour end marker

TEST: alternating on-curve / off-curve / on-curve → no insertion needed
  input:  flags = {1, 0, 1}
  output: flags = {1, 0, 1}  (unchanged)

TEST: all on-curve points → no off-curve midpoints inserted
  input:  3 on-curve points forming a straight-edged triangle
  output: same 3 points (straight lines need no implicit points)

TEST: endPtsOfContours updated correctly after insertions
  single contour with 2 off-curve points
  after insertion: endPtsOfContours = {2} (was {1}, now has extra point)

TEST: multi-contour glyph → each contour handled independently
  two contours each with their own off-curve sequences
  endPtsOfContours has correct values for both
```

---

### `tests/test_ttffile.cpp` — Integration Tests

These tests load the real `src/fonts/JetBrainsMono-Bold.ttf` file. They're slower than unit tests but validate the full parsing pipeline end-to-end.

```cpp
class TTFFileTest : public ::testing::Test {
protected:
    static std::vector<char> fontData;
    static TTFFile* ttf;

    static void SetUpTestSuite() {
        std::ifstream f("src/fonts/JetBrainsMono-Bold.ttf", std::ios::binary);
        fontData = std::vector<char>(std::istreambuf_iterator<char>(f), {});
        ttf = new TTFFile(TTFFile::parse(fontData));
    }
    static void TearDownTestSuite() { delete ttf; }
};
```

#### Header / Table Directory

```
TEST: parse succeeds without throwing
TEST: scalarType is recognized TTF value (0x00010000 or 0x74727565)
TEST: numTables > 0
TEST: required tables exist in table map: "head", "maxp", "loca", "cmap", "glyf", "hmtx"
TEST: all table offsets are within file bounds
TEST: all table lengths are within file bounds
```

#### HeadTable (from parsed TTFFile)

```
TEST: magicNumber == 0x5F0F3CF5  (TTF magic constant — always this value)
TEST: unitsPerEm is a power of 2 between 64 and 16384
TEST: indexToLocFormat is 0 or 1
TEST: xMax > xMin
TEST: yMax > yMin
```

#### MaxpTable (from parsed TTFFile)

```
TEST: numGlyphs > 0
TEST: numGlyphs matches number of loca entries (locas.size() == numGlyphs + 1)
```

#### LocaTable (via TTFFile locas vector)

```
TEST: locas.size() == numGlyphs + 1
TEST: all loca offsets are within glyf table bounds
TEST: offsets are non-decreasing (each glyph starts at or after the previous)
```

#### CmapTable (from parsed TTFFile)

```
TEST: space character (U+0020) → glyph index 0 or a blank glyph (no contours)
TEST: 'A' (U+0041) → non-zero glyph index
TEST: 'a' (U+0061) → non-zero glyph index, different from 'A'
TEST: 'Z' (U+005A) → non-zero glyph index
TEST: digit '0' (U+0030) → non-zero glyph index
```

#### Glyph Parsing (from parsed TTFFile)

```
TEST: parseGlyph for 'A' succeeds without throwing
TEST: glyph for 'A' has numberOfContours >= 1
TEST: glyph for 'A' has non-empty xCoordinates and yCoordinates
TEST: glyph for 'A' xCoordinates.size() == yCoordinates.size() == flags.size()
TEST: parseGlyphs for short ASCII string succeeds
  "Hello" → vector of 5 Glyph objects, no exceptions
TEST: parseGlyphs count matches input string length
TEST: all glyphs in "Hello" have consistent coordinate/flag sizes
```

#### Full Parse of All Glyphs (stress test)

```
TEST: parse every glyph in the font without crashing
  for glyphIndex in 0..numGlyphs:
      parse glyph, verify coordinates.size() == flags.size()
  This catches off-by-one errors in loca offsets and coordinate reading
  (mark as SLOW / run separately with --gtest_filter=*SlowTest*)
```

---

## Flows to Test

Beyond individual functions, these end-to-end flows should be validated:

### Flow 1: Font Load Pipeline
```
open file → read bytes → TTFFile::parse →
  header parsed → tables indexed → head parsed → maxp parsed →
  loca parsed → cmap parsed → return TTFFile

VERIFY: all sub-steps produce consistent data (numGlyphs from maxp == loca count)
```

### Flow 2: Character → Rendered Glyph
```
string "A" → stringToUnicode → {0x41} →
  cmapTable.getGlyphIndex(0x41) → glyphIndex N →
  locas[N] → locaOffset →
  glyfOffset + locaOffset → glyphOffset →
  Glyph::parseGlyph(data, glyphOffset) → raw Glyph →
  addPointsBetween() → normalized Glyph

VERIFY: each step has valid output (no empty vectors, no OOB indices)
```

### Flow 3: Space Character Handling
```
parseGlyph(data, 0x20) → should return empty Glyph (numberOfContours == 0)
  without throwing
drawSimpleGlyph on empty glyph → should no-op, not crash
```

### Flow 4: Compound Glyph Resolution (after fixing B3)
```
Find a compound glyph (numberOfContours < 0) in the font
parseGlyph → parseCompoundGlyph →
  for each component: look up sub-glyph, apply transform, merge coordinates
VERIFY: resulting Glyph has numberOfContours > 0 (merged from components)
VERIFY: xCoordinates.size() == yCoordinates.size() == flags.size()
```

---

## What Is Hard to Test (and why)

| Area | Problem | Workaround |
|---|---|---|
| `drawSimpleGlyph` | Requires live `SDL_Renderer*` | Extract coordinate computation into a pure function, test that; test drawing separately |
| `DrawBezier` | Requires live `SDL_Renderer*` | Test `getBezierPoint` (pure math) instead |
| `initializeWindow/Renderer` | Opens an actual OS window | Only test in manual/visual runs |
| `parseCompoundGlyph` transforms | Depends on full font context | After fixing B3, unit test with crafted compound data + a stub sub-glyph |
| Visual rendering correctness | No pixel oracle | Manual visual inspection; compare against freetype or browser rendering |

---

## Suggested Test Run Order

1. `test_helpers` — no dependencies, pure math
2. `test_ttftable` — depends on Helpers
3. `test_headtable` / `test_maxptable` — depends on Helpers
4. `test_locatable` — depends on Helpers
5. `test_cmaptable` — depends on Helpers (fix B6 first for meaningful Format 4 tests)
6. `test_glyphtable` — depends on Helpers (fix B3, B4 first)
7. `test_ttffile` — integration, depends on all of the above + real font file

---

## Bugs to Fix Before Writing Tests

Some tests are only meaningful after specific bugs are fixed:

| Test | Prerequisite Fix |
|---|---|
| Format 4 cmap parsing | B6 — skipped header fields |
| Compound glyph parsing | B3 — TTFFile::parse inside loop |
| `instructions` vector contents | B4 — double initialization |
| `getGlyphIndex` throws on miss | B5 — unthrown runtime_error |
| Coordinate delta signs | B12 — unsigned read for signed delta |

Write the test first, watch it fail on the current code, then fix the bug and watch it pass.
