# TextRenderer — TODO

Generated from full code audit. Issues grouped by category and priority.
Subtasks are listed under each item — check them off as you go.

---

## BUGS (wrong behavior)

- [x] **B1** `main.cpp:41` — `sizeof(file)` checks the size of the `ifstream` object (~280 bytes), not the file size. Should guard against a genuinely empty or truncated file.
  - [x] Change `sizeof(file)` to `fileSize` on line 41
  - [x] Confirm the condition reads `if (fileSize < sizeof(uint32_t))`

- [x] **B2** `main.cpp:36–38` — No `file.is_open()` check after opening. If the font file doesn't exist, `seekg` silently fails and `fileSize` becomes garbage.
  - [x] Add `if (!file.is_open())` check immediately after the `ifstream` constructor call
  - [x] Print the attempted filename in the error message so failures are actionable
  - [x] Return 1 (or throw) before any `seekg`/`tellg` calls

- [x] **B3** `GlyphTable.cpp:206–209` — `TTFFile::parse()` called inside the compound glyph component loop, re-parsing the entire font file from scratch for every component of every compound glyph.
  - [x] Add `uint32_t glyfOffset` and `const std::vector<uint32_t>& locas` parameters to `Glyph::parseCompoundGlyph` signature in `GlyphTable.h`
  - [x] Add the same two parameters to `Glyph::parseGlyph` in `GlyphTable.h` so it can forward them when dispatching to `parseCompoundGlyph`
  - [x] Update `Glyph::parseGlyph` implementation in `GlyphTable.cpp` to accept and forward the new parameters to `parseCompoundGlyph`
  - [x] Replace the `TTFFile::parse(data).getLocas()` and `TTFFile::parse(data).getGlyfOffset()` calls inside `parseCompoundGlyph` with the passed-in parameters
  - [x] Update `TTFFile::parseGlyph` in `TTFFile.cpp` to pass `this->glyfOffset` and `this->locas` when calling `Glyph::parseGlyph`
  - [x] Remove the `#include "TTFFile.h"` from `GlyphTable.cpp` if it was only there for this call (check for other uses first)
  - [x] Run the program and verify compound glyphs still render (test with a font that has compound glyphs)

- [x] **B4** `GlyphTable.cpp:56` — `instructions` vector double-initialized: `vector<uint8_t> instructions(instructionLength)` creates zeros, then the loop pushes more elements on top, resulting in a vector twice as long.
  - [x] Change `vector<uint8_t> instructions(instructionLength)` to `vector<uint8_t> instructions`
  - [x] Add `instructions.reserve(instructionLength)` on the next line
  - [x] Verify the `push_back` loop still reads exactly `instructionLength` bytes from the buffer (the reads and pos advancement are correct; only the storage was wrong)

- [x] **B5** `CmapTable.cpp:144` — `std::runtime_error` is constructed but not thrown — silently returns 0, mapping any unmapped character to glyph 0 with no warning.
  - [x] Add `throw` keyword before `std::runtime_error("could not find glyph")` on line 144
  - [x] Verify that the call sites (`TTFFile::parseGlyph`, `TTFFile::parseGlyphs`) already wrap calls in try/catch — they do, but double-check the catch prints something useful
  - [x] Remove the now-unreachable `return 0` below the throw

- [x] **B6** `CmapTable.cpp:31–44` — Format 4 cmap parsing skips `searchRange`, `entrySelector`, `rangeShift` (6 bytes) and the `reservedPad` (2 bytes), shifting all subsequent field reads by 8 bytes. Currently masked because JetBrainsMono uses Format 12.
  - [x] After reading `segCountX2`, add reads (or `pos += 6`) to consume `searchRange`, `entrySelector`, `rangeShift`
  - [x] After reading the `endCodes` array, add a read (or `pos += 2`) to consume the `reservedPad` field
  - [x] Load a font known to use only Format 4 (older fonts, e.g. many system fonts) and verify characters map to the correct glyphs
  - [x] Write a unit test per `TESTING.md` with a hand-crafted Format 4 buffer

- [x] **B7** `GlyphTable.cpp:156–159` — Compound glyph transformation matrix components stored as `int16_t` (`a = 1.0`, `b = 0.0`). These are F2Dot14 fixed-point values and must be decoded as floats.
  - [x] Change `as`, `bs`, `cs`, `ds` parallel vectors from `vector<int16_t>` to `vector<float>` (or remove them as part of D6)
  - [x] Change local variables `a`, `b`, `c`, `d` from `int16_t` to `float`
  - [x] Decode scale values read from the file with `static_cast<int16_t>(read2Bytes(data, pos)) / 16384.0f`
  - [x] Update `xPrime`/`yPrime` calculation to use float arithmetic: `float xp = a*x + c*y + dx; float yp = b*x + d*y + dy;`
  - [x] Cast results back when pushing: `static_cast<int16_t>(std::round(xp))`
  - [x] Add `#include <cmath>` to `GlyphTable.cpp` if not already present (for `std::round` and `std::abs`)

- [x] **B8** `GlyphTable.cpp:180–193` — `33 / 65536` uses integer division and always evaluates to 0, making the condition always true so `m` is always `2 * m0`.
  - [x] Change `33 / 65536` to `33.0f / 65536.0f` on line 184
  - [x] Change `33 / 65536` to `33.0f / 65536.0f` on line 189
  - [x] Update `m0`, `n0`, `m`, `n` from `int32_t` to `float` so the comparisons work correctly with float `a`, `b`, `c`, `d` (this aligns with the B7 fix)

- [x] **B9** `GlyphTable.cpp:229` — `contourOffset` is set to the total accumulated size of `xCoordinatesPush` after appending the current component, not the count before — so second and later components get wrong endpoint indices.
  - [x] Before the coordinate push loop for component `i`, capture `size_t prevSize = xCoordinatesPush.size()`
  - [x] Use `prevSize` as the offset when adjusting `endPtsOfContours` entries from the sub-glyph
  - [x] Move the `contourOffset` update to after the coordinate push loop: `contourOffset = xCoordinatesPush.size()`
  - [x] Verify: after the fix, `endPtsOfContours` values for the second component should be shifted by exactly the number of points in the first component

- [x] **B10** `SDLInitializer.cpp:4,9` — `SDL_Init` called twice in sequence (`INIT_EVERYTHING` then `INIT_VIDEO`).
  - [x] Remove lines 9–11 (the second `SDL_Init(SDL_INIT_VIDEO)` call and its surrounding if block)
  - [x] Keep only the first `SDL_Init(SDL_INIT_EVERYTHING)` with its return-value check

- [x] **B11** `GlyphTable.cpp:363–373` — `xCoordinates[j + 1]` and `xCoordinates[j + 2]` accessed in `drawSimpleGlyph` without bounds checking, which is UB near the end of a contour.
  - [x] Understand the current triplet-access pattern: for each on-curve point, it reads the next point (j+1) as the control point and (j+2) as the end point
  - [x] Determine the correct wrap-around: when near the end of a contour, j+1 or j+2 should wrap back to `contourStartIndex`
  - [x] Add a helper lambda or inline logic to get the wrapped index within a contour given a base index and an offset
  - [x] Replace raw `j+1` and `j+2` accesses with the wrapped-index lookups
  - [x] Test with a glyph that has many contours and short contours to exercise the edge cases

- [x] **B12** `GlyphTable.cpp:82–99` — Long-vector coordinate deltas read via `read2Bytes` (returns `uint16_t`). Negative signed deltas get added as large positive numbers — technically UB via int16_t overflow, though it works in practice due to two's complement.
  - [x] Add `int16_t readS16(const std::vector<char>& data, int& offset)` declaration to `Helpers.h`
  - [x] Implement `readS16` in `Helpers.cpp`: `return static_cast<int16_t>(convertEndian16(*reinterpret_cast<const uint16_t*>(&data[offset]))); offset += 2;`
  - [x] In `parseSimpleGlyph`, replace `currentX += read2Bytes(data, pos)` (long-vector x case) with `currentX += readS16(data, pos)`
  - [x] Replace `currentY += read2Bytes(data, pos)` (long-vector y case) with `currentY += readS16(data, pos)`
  - [x] Verify glyphs with negative coordinate deltas (descenders, leftward strokes) still render correctly

---

## MEMORY LEAKS

- [ ] **M1** `TTFTable.cpp:50` — All `TTFTable` objects heap-allocated with `new`, stored in `vector<TTFTable*>`, never deleted.
  - [ ] Change `TTFTable::parseTableDirectory` return type from `vector<TTFTable*>` to `vector<TTFTable>` in `TTFTable.h`
  - [ ] Change `tables.push_back(new TTFTable(...))` to `tables.emplace_back(tag, checksum, tableOffset, length)` in `TTFTable.cpp`
  - [ ] Update `TableVec` alias in `TTFHeader.h` from `vector<TTFTable*>` to `vector<TTFTable>`
  - [ ] Update `TableMap` alias in `TTFHeader.h` from `unordered_map<string, TTFTable*>` to `unordered_map<string, TTFTable>` (stores by value)
  - [ ] Update `buildTableMap` in `TTFHeader.cpp` to copy by value: `map.emplace(t.getTag(), t)` (iterating over `const TTFTable&` not `TTFTable*`)
  - [ ] Update `TTFHeader::tablesList` member from `TableVec` (pointer vec) to `vector<TTFTable>`
  - [ ] Update `TTFFile.h`: change `vector<TTFTable*> tables` member to `vector<TTFTable>`
  - [ ] Find all `.cpp` files that call `->` on a `TTFTable*` from these containers and change to `.`
  - [ ] Compile and confirm no remaining raw `new TTFTable` calls

- [ ] **M2** `TTFFile.cpp:46` — `TTFTable::parseTableDirectory` called a second time inside `TTFFile::parse`, leaking the entire first allocation.
  - [ ] Remove the line `vector<TTFTable*> tables = TTFTable::parseTableDirectory(data, header.getNumTables());` from `TTFFile::parse`
  - [ ] Remove the debug print loop on lines 49–51 that iterates over the now-removed `tables` local (or rewrite it to iterate `header.getTables()` if you want to keep the print)
  - [ ] Verify nothing downstream in `TTFFile::parse` depended on the local `tables` variable (the `tableMap` from `header.getTables()` is used for all actual lookups)

---

## DESIGN / ARCHITECTURE

- [ ] **D1** Mixed include guards — five headers use `#ifndef`/`#define`/`#endif`, the rest use `#pragma once`.
  - [ ] Replace the `#ifndef`/`#define`/`#endif` guard in `TTFFile.h` with `#pragma once`
  - [ ] Replace in `Helpers.h`
  - [ ] Replace in `SDLInitializer.h`
  - [ ] Replace in `MovableLine.h` (will be deleted in D8, but clean it now)
  - [ ] Replace in `MovablePoint.h` (same)

- [ ] **D2** `Helpers.h:10` — `using namespace std;` at global scope in a header, polluting every includer's namespace.
  - [ ] Remove the `using namespace std;` line from `Helpers.h`
  - [ ] Add `std::` prefix to every `vector`, `string`, and `tuple` in the function declarations in `Helpers.h`
  - [ ] Compile and fix any errors in files that were relying on the implicit `std::` from the header (most `.cpp` files have their own `using namespace std` so this should be minimal)

- [ ] **D3** `TTFFile` constructor takes 11 parameters, 5 of which (`cmapOffset`, `glyfOffset`, `headOffset`, `locaOffset`, `maxpOffset`) are redundant — already accessible from the stored table map.
  - [ ] Remove `cmapOffset`, `glyfOffset`, `headOffset`, `locaOffset`, `maxpOffset` from the `TTFFile` constructor parameter list in `TTFFile.h`
  - [ ] Remove the 5 corresponding private member variables from `TTFFile.h`
  - [ ] Remove the 5 getter method declarations (`getCmapOffset`, `getGlyfOffset`, `getHeadOffset`, `getLocaOffset`, `getMaxpOffset`) from `TTFFile.h`
  - [ ] Remove the 5 getter implementations from `TTFFile.cpp`
  - [ ] Remove the 5 arguments from the `TTFFile(...)` constructor call in `TTFFile::parse`
  - [ ] Remove the 5 member initializations from the `TTFFile` constructor initializer list in `TTFFile.cpp`
  - [ ] Update `TTFFile::parseGlyph` to derive `glyfOffset` on demand: `uint32_t glyfOffset = tableMap.at("glyf").getOffset();` (or equivalent access through the stored header/tables)
  - [ ] Grep for any remaining call sites of the removed getters: `grep -r "getGlyfOffset\|getCmapOffset\|getHeadOffset\|getLocaOffset\|getMaxpOffset" .`

- [ ] **D4** `GlyphTable.h` includes `SDL2/SDL.h`, coupling the data-parsing layer to the rendering layer.
  - [ ] Create `include/Renderer.h` with a declaration: `void drawSimpleGlyph(SDL_Renderer* renderer, const Glyph& glyph, int xOffset, int yOffset, double scalingFactor, int screenHeight, int thickness)`
  - [ ] Add `#include "SDL2/SDL.h"` and `#include "GlyphTable.h"` to `Renderer.h`
  - [ ] Create `src/Renderer.cpp` and move the `drawSimpleGlyph` implementation there from `GlyphTable.cpp`
  - [ ] Remove the `drawSimpleGlyph` static method declaration from the `Glyph` class in `GlyphTable.h`
  - [ ] Remove `#include "SDL2/SDL.h"` from `GlyphTable.h`
  - [ ] Remove `#include "MovableLine.h"` from `GlyphTable.h` (it was only there for the drawing code)
  - [ ] Remove the `drawSimpleGlyph` implementation from `GlyphTable.cpp`
  - [ ] Update `main.cpp` to `#include "Renderer.h"` and call `drawSimpleGlyph(renderer, glyphs[i], ...)` as a free function
  - [ ] Add `src/Renderer.cpp` to `SRCS` in the makefile
  - [ ] Compile and confirm `GlyphTable.h` no longer requires SDL2 on the include path

- [ ] **D5** Getters return vectors by value, causing copies on every call. `drawSimpleGlyph` also takes `Glyph` by value.
  - [ ] Change `Glyph::getEndPtsOfContours()` return type from `vector<uint16_t>` to `const std::vector<uint16_t>&` in `GlyphTable.h` and `GlyphTable.cpp`
  - [ ] Change `Glyph::getInstructions()` to `const std::vector<uint8_t>&`
  - [ ] Change `Glyph::getFlags()` to `const std::vector<uint8_t>&`
  - [ ] Change `Glyph::getXCoordinates()` to `const std::vector<int16_t>&`
  - [ ] Change `Glyph::getYCoordinates()` to `const std::vector<int16_t>&`
  - [ ] Change `CmapTable::getSubtables()` return type to `const std::vector<CmapSubtable>&`
  - [ ] Change `drawSimpleGlyph` (in Renderer.h/cpp after D4) parameter from `Glyph glyph` to `const Glyph& glyph`
  - [ ] Compile and check for any callers that were relying on the copy (e.g. storing a returned vector in a non-const local and then modifying it — unlikely but check)

- [ ] **D6** `parseCompoundGlyph` uses nine parallel vectors to track per-component data, which is fragile and hard to read.
  - [ ] Define a `struct ComponentData` near the top of `GlyphTable.cpp` (or in `GlyphTable.h`) with fields: `uint16_t glyphIndex; float a, b, c, d; int16_t dx, dy;`
  - [ ] Replace the nine parallel vectors (`glyphIndexs`, `argument1s`, `argument2s`, `as`, `bs`, `cs`, `ds`, `ms`, `ns`) with `std::vector<ComponentData> components`
  - [ ] Rewrite the `while (keepGoing)` loop to build one `ComponentData` per iteration and `push_back` it to `components`
  - [ ] Remove `ms` and `ns` — their computation was wrong (B8) and they were never used after being computed; omit them in the refactor
  - [ ] Rewrite the coordinate-transform loop to use `components[i].a`, `components[i].dx`, etc.
  - [ ] This resolves N4 (`glyphIndexs` rename) as a side effect — no need for the rename if the vector is gone

- [ ] **D7** `MovableLine.h` declares three methods with no implementation file — linker failure if ever called.
  - [ ] `grep -r "MovableLine" .` to confirm no call sites in the rendering pipeline
  - [ ] Remove `include/MovableLine.h`
  - [ ] Remove `#include "MovableLine.h"` from `GlyphTable.h` (also done as part of D4)
  - [ ] Remove `#include "MovableLine.h"` from `Helpers.h` if present

- [ ] **D8** `MovablePoint` and `MovableLine` are unused dead code. `MovablePoint::move()` is a documented no-op.
  - [ ] Confirm neither class is referenced anywhere: `grep -r "MovablePoint\|MovableLine" .`
  - [ ] Delete `src/MovablePoint.cpp`
  - [ ] Delete `include/MovablePoint.h`
  - [ ] Delete `include/MovableLine.h` if not already done in D7
  - [ ] Remove `src/MovablePoint.cpp` from `SRCS` in the makefile
  - [ ] Compile and confirm nothing breaks

- [ ] **D9** `HeadTable` and `MaxpTable` are pure data containers — 30+ trivial getters add noise with no benefit.
  - [ ] Convert `HeadTable` to a `struct` with all fields `public` in `HeadTable.h` (keep `static parseHeadDirectory`)
  - [ ] Remove all 17 getter declarations from `HeadTable.h`
  - [ ] Remove all 17 getter implementations from `HeadTable.cpp`
  - [ ] Find all call sites of `HeadTable` getters (grep for `headTable.get`) and change to direct field access (e.g. `headTable.getIndexToLocFormat()` → `headTable.indexToLocFormat`)
  - [ ] Convert `MaxpTable` to a `struct` the same way
  - [ ] Remove all 15 getter declarations from `MaxpTable.h`
  - [ ] Remove all 15 getter implementations from `MaxpTable.cpp`
  - [ ] Update all `MaxpTable` getter call sites to direct field access
  - [ ] Compile and confirm no remaining getter calls

- [ ] **D10** `LocaTable` stores two separate arrays and throws if you call the wrong getter. Unify into a single `vector<uint32_t>`.
  - [ ] Remove the `bool is32bitFormat` private member from `LocaTable`
  - [ ] Remove the `offsets16` (`vector<uint16_t>`) private member from `LocaTable`
  - [ ] Rename `offsets32` to `offsets` and make it the sole storage member (type `vector<uint32_t>`)
  - [ ] Update `LocaTable::parse`: for the 16-bit format path, multiply each value by 2 before storing (`offsets32[i] = read2Bytes(data, pos) * 2u`) — this is required by the TTF spec
  - [ ] Remove `getOffsets16()` and `getOffsets32()` declarations and implementations
  - [ ] Add `const std::vector<uint32_t>& getOffsets() const` declaration and implementation
  - [ ] Update `TTFFile::parse` to remove the `if (indexToLocFormat)` branch for building `locas` — replace with a single `locaTable.getOffsets()`

- [ ] **D11** `CmapTable::getGlyphIndex` performs three separate linear scans through the subtables list on every character lookup.
  - [ ] Add a `const CmapSubtable* activeSubtable = nullptr` private member to `CmapTable` in `CmapTable.h`
  - [ ] In the `CmapTable` constructor, after the subtable parse loop, do a single pass to select the best subtable: prefer (platformID=3, encodingID=10) → (3, 1) → (1, 0) — store a pointer to the selected element in `activeSubtable`
  - [ ] Note: `activeSubtable` must point into the `subtables` vector, so make sure `subtables` is never reallocated after this point (it isn't — the constructor is done)
  - [ ] Rewrite `CmapTable::getGlyphIndex` to call `activeSubtable->getGlyphIndex(unicodeValue)` directly
  - [ ] Handle `activeSubtable == nullptr` by throwing `std::runtime_error("no supported cmap subtable found")`
  - [ ] Remove the three old for-loops and the dead `break` statements (resolves X2)

---

## PERFORMANCE

- [ ] **P1** `main.cpp:124–147` — Entire canvas cleared and all glyphs redrawn every frame even when nothing changes.
  - [ ] Add `bool canvasDirty = true` before the main loop
  - [ ] Wrap the canvas clear + glyph draw block (`SDL_SetRenderTarget` → second `SDL_SetRenderTarget(nullptr)`) in `if (canvasDirty)`
  - [ ] Set `canvasDirty = false` at the end of the draw block
  - [ ] Set `canvasDirty = true` in the `SDLK_PLUS` and `SDLK_EQUALS` key cases (zoom in)
  - [ ] Set `canvasDirty = true` in the `SDLK_MINUS` key case (zoom out)
  - [ ] Keep the `SDL_RenderCopy` from canvas to window outside the dirty check — it runs every frame to blit the cached canvas

- [ ] **P2** `Helpers.cpp:22–30` — `DrawBezier` heap-allocates two `vector<SDL_Point>` on every call.
  - [ ] Add `#include <array>` to `Helpers.cpp`
  - [ ] Replace `std::vector<SDL_Point> points` with `std::array<SDL_Point, 21> points` and add `int pointCount = 0`
  - [ ] Replace `points.push_back(point)` with `points[pointCount++] = point`
  - [ ] Replace `std::vector<SDL_Point> simplifiedPoints` with `std::array<SDL_Point, 21> simplified` and `int simplifiedCount = 0`
  - [ ] Replace `simplifiedPoints.push_back(...)` with `simplified[simplifiedCount++] = ...`
  - [ ] Update `points.front()` → `points[0]`, `simplifiedPoints.back()` → `simplified[simplifiedCount - 1]`
  - [ ] Update all `.size()` calls to use `pointCount` and `simplifiedCount` respectively

- [ ] **P3** `main.cpp:77–78` — `ADVANCEWIDTH` and `ADVANCEHEIGHT` recomputed on every iteration of the outer render loop and every iteration of the inner glyph loop.
  - [ ] Move `ADVANCEWIDTH` and `ADVANCEHEIGHT` declarations to just before the `while (!quit)` loop
  - [ ] Since they depend on `scalingFactor`, update them in the same key-event cases where `scalingFactor` changes (alongside setting `canvasDirty = true` from P1)

- [ ] **P4** `main.cpp:77` — Advance width hardcoded to 600 font units for all glyphs. Blocked on `hmtx` implementation from PLANS.md Phase 1.
  - [ ] Extract `600` to `const int NOMINAL_ADVANCE_UNITS = 600` near the top of `main.cpp`
  - [ ] Extract `1320` to `const int NOMINAL_LINE_HEIGHT_UNITS = 1320`
  - [ ] Add a comment: `// TODO: replace with per-glyph hmtx.getAdvanceWidth() once hmtx is parsed`
  - [ ] Wire up real values once PLANS.md Phase 1 (hmtx) is complete

- [ ] **P5** `GlyphTable.cpp:339–347` — `drawSimpleGlyph` copies all coordinate and flag vectors via by-value getters on every draw call every frame.
  - [ ] *Depends on D5* — no independent subtasks; this is automatically resolved once D5 is complete and getters return `const&`

---

## BUILD SYSTEM

- [ ] **BLD1** `makefile` — SDL2 paths hardcoded to macOS Homebrew; the project runs on Linux where these paths don't exist.
  - [ ] Replace `-I/opt/homebrew/include/SDL2` in `CXXFLAGS` with `$(shell sdl2-config --cflags)`
  - [ ] Replace `-L/opt/homebrew/lib -lSDL2` in `LDFLAGS` with `$(shell sdl2-config --libs)`
  - [ ] Run `make clean && make` on Linux to confirm it builds
  - [ ] Optional: add a fallback `$(shell pkg-config --cflags sdl2)` in case `sdl2-config` isn't on PATH

- [ ] **BLD2** No release build target — only debug builds with `-g` and no optimization.
  - [ ] Add `RELEASE_FLAGS := -O2 -DNDEBUG` near the top of the makefile
  - [ ] Add `release` to the `.PHONY` line
  - [ ] Add a `release` target that sets `CXXFLAGS += $(RELEASE_FLAGS)` and depends on `$(TARGET)`
  - [ ] Ensure `-g` stays only in the default debug `CXXFLAGS` and is not inherited by the release target

- [ ] **BLD3** `TEST_TARGET` and `TEST_SRCS` reference `test.cpp` which doesn't exist.
  - [ ] Create a `tests/` directory
  - [ ] Create `tests/test_helpers.cpp` with a single placeholder gtest: `TEST(Placeholder, True) { EXPECT_TRUE(true); }`
  - [ ] Update `TEST_SRCS` in the makefile to point to `tests/test_helpers.cpp` instead of `test.cpp`
  - [ ] Update the `$(TEST_TARGET)` link line to include `$(shell pkg-config --cflags --libs gtest_main)`
  - [ ] Rename the test output binary to `test_runner` for clarity
  - [ ] Expand with real tests per `TESTING.md` as bug fixes are made

- [ ] **BLD4** `main.o` and `main.d` emitted to the project root while all other objects go under `build/`.
  - [ ] Change `MAIN_OBJ := main.o` to `MAIN_OBJ := $(OBJDIR)/main.o`
  - [ ] The `MAIN_DEP` variable derives from `MAIN_OBJ` automatically — no change needed there
  - [ ] Add `@mkdir -p $(dir $@)` to the `$(MAIN_OBJ): main.cpp` compile rule (same pattern as the other objects)
  - [ ] Update the `clean` target — since `MAIN_OBJ` is now under `$(OBJDIR)`, removing `$(OBJDIR)` cleans it; remove the explicit `$(MAIN_OBJ) $(MAIN_DEP)` from the clean rule

- [ ] **BLD5** No `run` target.
  - [ ] Add `run` to the `.PHONY` line
  - [ ] Add a `run: $(TARGET)` rule with `./$(TARGET)` as the recipe

---

## NAMING / TYPOS

- [ ] **N1** `maxTwighlightPoints` misspelled in `MaxpTable` everywhere — field, constructor param, getter.
  - [ ] Rename the private field in `MaxpTable.h`: `maxTwighlightPoints` → `maxTwilightPoints`
  - [ ] Rename the constructor parameter in `MaxpTable.h`
  - [ ] Rename the getter declaration: `getMaxTwighlightPoints` → `getMaxTwilightPoints` in `MaxpTable.h`
  - [ ] Rename the field in the constructor initializer list in `MaxpTable.cpp`
  - [ ] Rename the getter definition in `MaxpTable.cpp`
  - [ ] Rename the local variable in `parseMaxpDirectory` in `MaxpTable.cpp`
  - [ ] Run `grep -r "Twighlight" .` to catch any remaining occurrences

- [ ] **N2** `intializeTexture` missing the second 'i'.
  - [ ] Rename `intializeTexture` → `initializeTexture` in `SDLInitializer.h` declaration
  - [ ] Rename in `SDLInitializer.cpp` function definition
  - [ ] Update the call site in `main.cpp:66`

- [ ] **N3** `CalcTableChecksum` uses PascalCase; all other free functions use camelCase.
  - [ ] Rename `CalcTableChecksum` → `calcTableChecksum` in `Helpers.h`
  - [ ] Rename the definition in `Helpers.cpp`
  - [ ] Update the call site in `TTFTable.cpp`

- [ ] **N4** `glyphIndexs` should be `glyphIndices` in `GlyphTable.cpp:110`.
  - [ ] Note: *this variable is eliminated by D6* (it becomes part of `ComponentData`). If doing D6, skip this item. If not doing D6 yet, rename it now.
  - [ ] If renaming independently: find-replace `glyphIndexs` → `glyphIndices` within `parseCompoundGlyph`

- [ ] **N5** `u_long` is POSIX-specific; use `size_t`.
  - [ ] Change `for (u_long i = 0;` → `for (size_t i = 0;` on `GlyphTable.cpp:208`
  - [ ] Change `for (u_long j = 0;` → `for (size_t j = 0;` on `GlyphTable.cpp:223`

- [ ] **N6** `buildTableMap` in `TTFHeader.cpp` has external linkage but is only used in that translation unit.
  - [ ] Add `static` before `TableMap buildTableMap(...)` in `TTFHeader.cpp`

---

## DEAD / COMMENTED-OUT CODE

- [ ] **DC1** `TTFTable.cpp:44–48` — Checksum verification logic is complete but commented out.
  - [ ] Uncomment the checksum verification block
  - [ ] Change `fprintf(stderr, ...)` and `printf(...)` to `std::cerr` to match the rest of the codebase
  - [ ] Change behavior on mismatch from silent to `std::cerr` warning (not a throw — some fonts have benign checksum issues)
  - [ ] Run with the actual font files and confirm all table checksums pass (fix checksum logic if any don't)

- [ ] **DC2** `CmapTable.cpp` — Multiple debug `cout` prints left in from development.
  - [ ] Remove the subtable-info `cout` on line 109 (inside the constructor loop)
  - [ ] Remove the `cout << "Using format 12"` on line 121
  - [ ] Remove the `cout << "Using format 4"` on line 130
  - [ ] Remove the `cout << "Using format 0"` on line 139
  - [ ] Note: after D11, the three format-selection loops are replaced — these will be gone automatically

- [ ] **DC3** `TTFHeader.cpp:39` — `cout << "ttf format detected"` printed on every font parse.
  - [ ] Remove the `cout << "ttf format detected\n"` line
  - [ ] Keep the `cerr` lines for unsupported formats (typ1, OTTO, unknown) — those are legitimate warnings worth keeping

- [ ] **DC4** `main.cpp:170–171` — Comment says "uncomment for fps debug" but the FPS `cout` is already active.
  - [ ] Remove the `//uncomment for fps debug on console` comment on line 171
  - [ ] Optionally: gate the FPS print behind `#ifdef DEBUG` if you don't want it in release builds (aligns with BLD2)

- [ ] **DC5** `hexToAscii` declared and implemented but never called anywhere.
  - [ ] Run `grep -r "hexToAscii" .` to confirm no call sites
  - [ ] Remove the `string hexToAscii(uint32_t value);` declaration from `Helpers.h`
  - [ ] Remove the `hexToAscii` implementation block from `Helpers.cpp` (lines 83–98)

- [ ] **DC6** `LocaTable.cpp:4` — `#include <arpa/inet.h>` included but `ntohl`/`ntohs` not used.
  - [ ] Remove `#include <arpa/inet.h>` from `LocaTable.cpp`

---

## MISC

- [ ] **X1** `main.cpp:185` — `file.close()` called at the end of `main`, long after the data was fully read at line 47.
  - [ ] Move `file.close()` to immediately after `file.read(buffer.data(), fileSize)` on line 47
  - [ ] Or remove it entirely — `ifstream` closes automatically on destruction (RAII), making the explicit close redundant

- [ ] **X2** `CmapTable.cpp:122,130,138` — `break` after `return` is unreachable dead code.
  - [ ] Remove `break;` after the first `return subtable.getGlyphIndex(unicodeValue)` (~line 123)
  - [ ] Remove `break;` after the second `return` (~line 131)
  - [ ] Remove `break;` after the third `return` (~line 138)
  - [ ] Note: after D11, these loops are replaced — this is resolved automatically

- [ ] **X3** `convertEndian8` swaps nibbles of a byte, not bytes — misleading name for a meaningless operation.
  - [ ] Run `grep -r "convertEndian8" .` to confirm no call sites
  - [ ] Remove the `uint8_t convertEndian8(uint8_t value);` declaration from `Helpers.h`
  - [ ] Remove the `convertEndian8` implementation from `Helpers.cpp` (lines 117–119)

- [ ] **X4** `GlyphTable.cpp:130–152` — `isWord`/`isXY` branches in `parseCompoundGlyph` read identical code regardless of the flag, ignoring the distinction between xy-offsets and point-index arguments.
  - [ ] Read the TTF spec section on composite glyph component flags: bit 0 = `ARG_1_AND_2_ARE_WORDS`, bit 1 = `ARGS_ARE_XY_VALUES`
  - [ ] For word-size + `ARGS_ARE_XY_VALUES`: read two `int16_t` signed values as dx, dy offsets
  - [ ] For word-size + point indices: read two `uint16_t` unsigned values as point indices (anchor matching, not offsets)
  - [ ] For byte-size + `ARGS_ARE_XY_VALUES`: read two `int8_t` signed values (use `readByte` + `static_cast<int8_t>`) as dx, dy
  - [ ] For byte-size + point indices: read two `uint8_t` values as point indices
  - [ ] Store `dx`/`dy` in the `ComponentData` struct from D6 (only valid when `ARGS_ARE_XY_VALUES` is set)
  - [ ] For point-index mode, store the two indices and note that full support requires matching points between parent and component glyph (complex — mark as partial if only offset mode is implemented)

- [ ] **X5** `GlyphTable.cpp:320–325` — `addPointsBetween` inserts a spurious off-curve midpoint between consecutive on-curve points; two on-curve points already define a straight line.
  - [ ] Remove the `else if (isCurrentOnCurve && isNextOnCurve)` block in the main loop (lines 320–325)
  - [ ] Remove the corresponding `else if (isLastPointOnCurve && isFirstPointOnCurve)` block in the wrap-around case (lines 285–292)
  - [ ] Render text before and after the change — visually should be identical (collinear Bezier control points produce exact straight lines), but with fewer intermediate points

- [ ] **X6** `main.cpp:77` — `ADVANCEWIDTH = 600 * scalingFactor` hardcodes 600 font units; only works for this one font.
  - [ ] Extract `600` to `const int NOMINAL_ADVANCE_UNITS = 600` near the top of `main.cpp`
  - [ ] Extract `1320` to `const int NOMINAL_LINE_HEIGHT_UNITS = 1320` similarly
  - [ ] Add a comment noting both should come from `hmtx`/`hhea` once PLANS.md Phase 1 is done
  - [ ] This item is fully resolved by PLANS.md Phase 1

## LINTING (clang-tidy)

Warnings captured from: `clang-tidy --checks="bugprone-*,modernize-*,performance-*,readability-*,..." --header-filter="$(pwd)/(src|include)/.*"`

- [x] **LT1** `endl` used instead of `'\n'` across all files — `std::endl` flushes the stream on every call, which is unnecessary and slow.
  - [x] Replace all `<< endl` with `<< '\n'` in `main.cpp` (lines 40, 49, 64, 176)
  - [x] Replace in `src/CmapTable.cpp` (lines 112, 124, 133, 142)
  - [x] Replace in `src/GlyphTable.cpp` (lines 380, 381, 382, 383, 388, 393, 396)
  - [x] Replace in `src/Helpers.cpp` (line 103)
  - [x] Replace in `src/SDLInitializer.cpp` (lines 5, 16, 26, 37)
  - [x] Replace in `src/TTFFile.cpp` (lines 50, 86, 93, 100)
  - [x] Replace in `src/TTFHeader.cpp` (line 81)
  - [x] Replace in `src/TTFTable.cpp` (line 18)

- [x] **LT2** Narrowing conversions in `GlyphTable.cpp` — `uint32_t`/`uint16_t` read results assigned to `int`/`int16_t` without explicit cast; float↔int conversions in coordinate and matrix arithmetic.
  - [x] Add `static_cast<int>` where `uint32_t` offsets are assigned to `int pos` (lines 49, 109, 236)
  - [x] Add `static_cast<int16_t>` where `read2Bytes` results (uint16_t) are assigned to signed fields (lines 237–241)
  - [x] Add explicit casts in the coordinate delta narrowing cases (lines 79, 81, 84, 95, 97, 100)
  - [x] Fix narrowing in `ms`/`ns` push_back — floats stored into `vector<int32_t>` (lines 196, 197); resolved by D6 if done first
  - [x] Add `static_cast<int16_t>` where `size_t pointOffset` is used as endpoint shift (line 210)
  - [x] Add parentheses to matrix multiply lines 224–225 to clarify operator precedence (also resolves LT8)

- [x] **LT3** Narrowing conversions in other files — same pattern of unsigned→signed or float→int without explicit cast.
  - [x] `src/CmapTable.cpp`: add casts on lines 26, 33, 39, 51, 101
  - [x] `src/HeadTable.cpp`: add casts on lines 63, 70, 71, 72–80
  - [x] `src/Helpers.cpp`: add casts on lines 21, 25–26, 112–113
  - [x] `src/LocaTable.cpp`: add cast on line 9 (`size_t` → `int pos`)
  - [x] `src/MovablePoint.cpp`: add casts on lines 26–29, 37–38
  - [x] `src/TTFFile.cpp`: no additional narrowing beyond what's already flagged

- [x] **LT4** Implicit `int → bool` conversions in flag bit-checks — `flag & 1` should be `(flag & 1) != 0` for clarity and to silence the warning.
  - [x] Fix in `parseSimpleGlyph`: lines 64, 76, 78, 83 in `GlyphTable.cpp`
  - [x] Fix in `parseCompoundGlyph`: lines 123, 124, 125, 129, 130, 131 in `GlyphTable.cpp`
  - [x] Fix in `addPointsBetween`: lines 272, 273, 306, 307 in `GlyphTable.cpp`
  - [x] Fix in `drawSimpleGlyph`: line 353 in `GlyphTable.cpp`
  - [x] Fix `bool → uint32_t` conversion in `Helpers.cpp:202`

- [x] **LT5** Constructor vector parameters passed by value — should accept by value and `std::move` into member in initializer list.
  - [x] `Glyph` constructor (`GlyphTable.cpp:18–23`): add `std::move` for `endPtsOfContours`, `instructions`, `flags`, `xCoordinates`, `yCoordinates` in the initializer list
  - [x] `TTFFile` constructor (`TTFFile.cpp:8–10,12`): add `std::move` for `header`, `tables`, `locas`, `cmapTable`
  - [x] `TTFTable` constructor (`TTFTable.cpp:10`): add `std::move` for the `string` tag parameter

- [x] **LT6** Function parameters passed by value when `const&` would avoid the copy.
  - [x] `drawSimpleGlyph`: change `Glyph glyph` → `const Glyph& glyph` (also tracked in D5)
  - [x] `TTFFile::parseGlyphs`: change `std::string letters` → `const std::string& letters` (`TTFFile.cpp:108`)

- [x] **LT7** Missing braces around single-statement `if`/`else` bodies.
  - [x] Add braces to the zoom clamp conditionals in `main.cpp` (lines 95, 99, 103, 107, 115, 123–126)
  - [x] Add braces in `Helpers.cpp` (lines 191, 192, 202, 212, 218, 219)

- [x] **LT8** Manual `<`/`>` comparisons where `std::min`/`std::max` should be used (`main.cpp`).
  - [x] Replace `if (scalingFactor < MIN) scalingFactor = MIN` pattern with `scalingFactor = std::max(scalingFactor, MIN)` on lines 95, 103, 115, 123, 125
  - [x] Replace `if (scalingFactor > MAX) scalingFactor = MAX` with `std::min` equivalents on lines 99, 107, 124, 126
  - [x] Add `#include <algorithm>` to `main.cpp` if not already present

- [x] **LT9** `switch` on SDL key event missing a `default` case (`main.cpp:92`).
  - [x] Add `default: break;` at the end of the `switch (event.key.keysym.sym)` block

- [x] **LT10** Return statements repeat the return type instead of using braced init lists (`modernize-return-braced-init-list`).
  - [x] `TTFHeader.cpp`: convert `return TTFHeader(...)` on lines 40, 43, 46, 49 to `return {...}`
  - [x] `GlyphTable.cpp:105`: convert `return Glyph(...)` to `return {...}`
  - [x] `GlyphTable.cpp:231`: convert `return Glyph(...)` to `return {...}`
  - [x] `MaxpTable.cpp:71`: convert `return MaxpTable(...)` to `return {...}`
  - [x] `CmapTable.cpp:118`: convert `return CmapSubtable(...)` to `return {...}`
  - [x] `TTFFile.cpp:69`: convert `return TTFFile(...)` to `return {...}`
  - [x] `HeadTable.cpp:82`: convert `return HeadTable(...)` to `return {...}`

- [x] **LT11** `else` after `return` — the `else` branch is dead; remove it and unindent the else body.
  - [x] `TTFHeader.cpp:41`: remove `else` after the `return` on line 40
  - [x] `GlyphTable.cpp:245`: remove `else` after `return` in `parseGlyph`
  - [x] `CmapTable.cpp:73,77`: remove both `else` branches in `getGlyphIndex` format dispatch

- [x] **LT12** Missing explicit parentheses in math expressions flagged by `readability-math-missing-parentheses`.
  - [x] `Helpers.cpp:10`: parenthesize the Bezier interpolation terms
  - [x] `Helpers.cpp:25–26`: parenthesize `*` before `+` in x/y lerp expressions
  - [x] `GlyphTable.cpp:224–225`: parenthesize matrix multiply terms `(as[i] * x)` and `(cs[i] * y)`
  - [x] `GlyphTable.cpp:356`: parenthesize `%` before `+` in `wrapIdx`
  - [x] `GlyphTable.cpp:361,365,369`: parenthesize `*` before `+` in SDL coordinate calculations

- [x] **LT13** Inconsistent parameter names between `parseSimpleGlyph` declaration and definition.
  - [x] In `GlyphTable.h:37`, rename the first parameter from `pos` to `offset` to match the definition in `GlyphTable.cpp:48`

- [x] **LT14** Loop variable type too narrow — `uint16_t` used as loop counter where upper bound is `size_type` (`GlyphTable.cpp:395`).
  - [x] Change `for (uint16_t i = 0; i < xCoordinates.size(); i++)` to `for (size_t i = 0; ...)`

- [x] **LT15** High cognitive complexity — functions exceed the threshold of 25.
  - [x] `main()` (complexity 66): extract key-event handling into `handleKeyEvent(SDL_Event&, double& scalingFactor, bool& quit)`, extract render loop body into `renderFrame(...)`
  - [x] `parseCompoundGlyph` (complexity 33): extract the flag/argument reading block into a helper; resolved naturally by D6
  - [x] `CmapTable::getGlyphIndex` (complexity 31): resolved by D11 (single dispatch replaces three scan loops)

- [x] **LT16** Short identifier names flagged by `readability-identifier-length` — many are legitimate math names; suppress or rename case by case.
  - [x] Suppress `a`, `b`, `c`, `d` matrix variable warnings in `parseCompoundGlyph` with `// NOLINT(readability-identifier-length)` — single-letter names are conventional for 2×2 transform matrices; resolved by D6 if struct fields are renamed
  - [x] Rename loop variable `e` in `main.cpp:81` catch block → `err`
  - [x] Rename `t` in `TTFHeader.cpp:56` → `table`
  - [x] Rename `os` parameter in `TTFHeader.cpp:76` → `outStream`
  - [x] Rename `ss` in `Helpers.cpp:87` → `stream`
  - [x] Rename `ci`/`ei` in `GlyphTable.cpp:358–359` → `ctrlIdx`/`endIdx`
  - [x] Rename `m0`/`n0`/`m`/`n` in `GlyphTable.cpp:181–184` → `scaleM0`/`scaleN0`/`scaleM`/`scaleN`; resolved by D6
  - [x] Suppress `p0`/`p1`/`p2` in `Helpers.cpp:9` — conventional Bezier point names; add `// NOLINT`
  - [x] Suppress `x`/`y`/`t` in `Helpers.cpp:20–26` — conventional math param names; add `// NOLINT`

- [x] **LT17** `bugprone-easily-swappable-parameters` — constructors with many same-typed params. These are design issues; most are resolved by D6/D9.
  - [x] `Glyph` constructor: resolved by D6 (fewer params once compound glyph is restructured) or add a `GlyphData` struct
  - [x] `TTFFile` constructor: resolved by D3 (removes 5 redundant params)
  - [x] `HeadTable`/`MaxpTable` constructors: resolved by D9 (converted to structs)
  - [x] `TTFTable` constructor: resolved by M1 (value type, emplace_back)
  - [x] `CmapSubtable` constructor (`CmapTable.cpp:8`): rename params to make intent clear or add a factory function
  - [x] `parseSimpleGlyph`/`parseCompoundGlyph`: the adjacent `uint32_t` params are unavoidable without a context struct — document with a comment for now

- [x] **LT18** `bugprone-branch-clone` — identical `if`/`else` branches in `parseCompoundGlyph` for the isWord/isXY case (`GlyphTable.cpp:138,146`).
  - [x] This is the same as X4 — the branches look identical because the signed/unsigned distinction is not yet implemented
  - [x] Resolve by implementing X4 (correct signed/unsigned argument reading based on `ARGS_ARE_XY_VALUES` flag)

- [x] **LT19** `clang-analyzer-deadcode.DeadStores` — `calculatedCheckSum` computed but never read (`TTFTable.cpp:39,41`).
  - [x] Resolve by uncommenting the checksum verification block (same as DC1)

- [x] **LT20** `performance-inefficient-vector-operation` — `push_back` inside loop in `parseSimpleGlyph` without pre-allocating (`GlyphTable.cpp:52`).
  - [x] Add `endPtsOfContours.reserve(numberOfContours)` before the loop on line 52

- [x] **LT21** Multiple declarations in a single statement (`readability-isolate-declaration`).
  - [x] `Helpers.cpp:211`: split `int s = 0, i = 0;` into two separate declarations
  - [x] `Helpers.cpp:216`: split `int w = 0, r = 0;` (or equivalent) into two declarations
  - [x] `MovablePoint.cpp:22`: split combined declaration
  - [x] `MovablePoint.cpp:35`: split combined declaration

- [x] **LT22** `readability-uppercase-literal-suffix` — float literals use lowercase `f` suffix (`16384.0f` etc). Very minor style nit; consider suppressing the check instead of touching every literal.
  - [x] Either: add `-readability-uppercase-literal-suffix` to the `--checks` suppression list in `.gitlab-ci.yml`
  - [x] Or: change all `16384.0f` → `16384.0F`, `33.0f` → `33.0F`, `65536.0f` → `65536.0F` etc. across `GlyphTable.cpp` and `Helpers.cpp`
