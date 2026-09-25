# TextRenderer — Phase 1 Testing Plan (Font Engine)

> Mid-level testing strategy for the font engine. No code — what to test, how to
> approach it, and the testing decisions you own. Test specs are written as
> intent ("given X, expect Y"), not implementations.

---

## Philosophy

The engine has three layers, each with a different testing strategy:

1. **Parsing layer** — pure bytes→structs. Fully deterministic, no I/O, no GPU.
   Unit-test with hand-crafted byte buffers and known expected values. This is
   where correctness bugs concentrate, so cover it first.
2. **Rasterizer + atlas** — pure CPU math producing grayscale bitmaps. Testable
   without any window via **golden-image** comparison against a reference
   (FreeType is a good oracle) within a tolerance.
3. **Bindings / packaging** — that the CMake package links and the Python wheel
   imports and round-trips an atlas.

The old plan treated rendering as "hard to test, visual only." That was true
when output was live SDL draw calls. Now the engine's output is a **CPU bitmap**,
so rasterization is unit-testable — this is a direct benefit of the
backend-agnostic decision.

---

## Framework decisions

- **C++:** Google Test (gtest), run under CTest once CMake lands (P1.7).
- **Golden images:** store reference PNGs under `tests/golden/`; compare with a
  per-pixel tolerance + a max-different-pixel budget (decide the thresholds).
- **Python:** pytest for the binding layer (P1.8).
- **Decision — reference oracle:** generate golden images once from FreeType at
  matching size/hinting-off, or hand-author tiny known shapes? (Recommend
  FreeType for real glyphs, hand-authored buffers for parser units.)
- **Decision — buffer-crafting helper:** a shared big-endian buffer builder for
  parser tests (push u8/u16/u32/s16, pad-to-4). Build it once under
  `tests/fixtures/`.

---

## Test file structure (target)

```
tests/
├── test_helpers.cpp        endian, readers, utf-8, bezier point, checksum
├── test_ttftable.cpp       table directory parsing
├── test_headtable.cpp      head fields, unitsPerEm, indexToLocFormat
├── test_maxptable.cpp      numGlyphs etc.
├── test_locatable.cpp      16/32-bit loca, *2 rule, sentinel count
├── test_cmaptable.cpp      formats 0/4/12, subtable priority
├── test_glyphtable.cpp     simple/compound parse, addPointsBetween
├── test_metrics.cpp        NEW — hhea/hmtx advance + bearings
├── test_rasterizer.cpp     NEW — flattening, fill, AA (golden images)
├── test_atlas.cpp          NEW — packing, UV rects, no-bleed
├── test_ttffile.cpp        integration on a real .ttf
├── fixtures/               shared buffer builders
└── golden/                 reference PNGs
```

---

## What to test, by module

### Helpers
- Endian conversion (16/32/64): known values and round-trips.
- Byte readers advance the offset correctly; sequential reads chain.
- `stringToUnicode`: ASCII, 2/3/4-byte UTF-8, mixed, empty, invalid byte throws.
- Bézier point: t=0 → first point, t=1 → last, t=0.5 on a symmetric curve at apex.
- Checksum: known sums, modulo-2³² wrap, zero-padded trailing bytes, OOB throws.

### TTFTable (table directory)
- Parse one and many records; tag is exactly 4 bytes; offset/length correct.

### HeadTable
- All fields at offset 0 and at a non-zero offset.
- `indexToLocFormat` 0 vs 1; signed fields (negative xMin) decode correctly.
- `unitsPerEm` read (don't assume 1000/2048).

### MaxpTable
- All fields; `numGlyphs` correct; parse at non-zero offset.

### LocaTable
- 16-bit format: values multiplied by 2 (spec rule); entry count = numGlyphs+1.
- 32-bit format: raw offsets; entry count = numGlyphs+1.
- Offsets are non-decreasing.

### CmapTable / CmapSubtable
- **Format 0:** direct index lookup.
- **Format 4:** single segment with idRangeOffset=0 and a known delta; the
  indirect (idRangeOffset≠0) path; the 0xFFFF terminator; out-of-range codepoint.
  (This format had the historical offset bug — lock it down hard.)
- **Format 12:** single and multiple groups; gaps return missing.
- **Subtable priority:** (3,10) > (3,1) > (1,0); throws when none supported.

### Glyph
- **Simple glyph:** decode delta-encoded coords (positive/negative short-vector
  and long-vector cases); flags including the repeat flag; endPtsOfContours.
- **Dispatch:** positive/zero contours → simple; negative → compound.
- **addPointsBetween:** insert on-curve midpoint between two off-curve points;
  wrap-around (first & last off-curve); no insertion when alternating or all
  on-curve; endPtsOfContours updated; multi-contour handled independently.
- **Compound:** with B3 fixed, test component transform + coordinate merge with a
  crafted compound + stub sub-glyph; verify contour-offset shifting across
  components.

### Metrics (NEW — P1.1)
- `hmtx` advance width for a known glyph matches ground truth (cross-check with
  `ttx`/fonttools dump).
- Glyphs beyond `numberOfHMetrics` inherit the last advance but keep their own
  lsb.
- `hhea` ascender/descender/lineGap parsed; derived line height matches expected.

### Rasterizer (NEW — P1.4/P1.5) — golden images
- Flattening: a known quadratic flattens within the deviation tolerance
  (segment count bounded; endpoints exact).
- Fill: 'O'/'B' have correct interior holes (nonzero winding); a filled triangle
  matches a hand-computed coverage mask.
- AA: supersampled edges produce intermediate coverage values (not just 0/255);
  output dimensions and bearing offsets are correct.
- Empty glyph (space): zero-area bitmap, no crash.
- **Decision:** tolerance budget (max per-pixel delta, max % differing pixels).

### Atlas (NEW — P1.6)
- All requested glyphs present with non-overlapping rects.
- 1px padding respected; UV rects map back to the correct glyph.
- Metrics (advance/bearing) stored per entry match the rasterized glyph.
- Deterministic packing for a fixed input (so goldens are stable).

### Integration (`test_ttffile.cpp`) — real font
- Parse succeeds; required tables present (`head`,`maxp`,`loca`,`cmap`,`glyf`,
  `hmtx`); all offsets/lengths within file bounds.
- `magicNumber` == 0x5F0F3CF5; `unitsPerEm` a power of two; `numGlyphs` matches
  loca count − 1.
- Common characters map to distinct non-zero glyphs.
- Parse a short string into glyphs; coordinate/flag vector sizes are consistent.
- **Stress (slow, separate filter):** parse every glyph in the font without
  crashing; catches loca off-by-ones and coordinate over-reads.
- **CFF rejection:** an OTTO/CFF font is detected and rejected cleanly (scope
  boundary), not mis-parsed.

---

## End-to-end flows to validate

1. **Load pipeline:** file → bytes → parse → tables indexed → head/maxp/loca/
   cmap/hhea/hmtx consistent (numGlyphs vs loca count).
2. **Character → atlas entry:** string → unicode → glyph index → loca offset →
   glyph parse → addPointsBetween → flatten → fill → atlas rect + metrics.
3. **Space handling:** space yields an empty glyph and a zero-area atlas entry
   with a valid advance, no crash.
4. **Compound resolution:** a compound glyph resolves to merged contours with
   consistent coordinate/flag sizes.
5. **Round-trip (Python, P1.8):** load font → request atlas → image buffer +
   metrics arrive intact and match the C++ output.

---

## What's still hard to test (and the workaround)

| Area | Problem | Workaround |
|---|---|---|
| Exact pixel rendering | No perfect oracle | Golden images vs FreeType within tolerance |
| Gamma/AA "look" | Subjective | Lock a reference image; review diffs on change |
| Window/context (demo) | Opens an OS window | Keep in `examples/`; exclude from unit tests |
| GPU upload (Phase 2) | Needs a GL context | Tested in Phase 2, not here |

---

## Suggested order

1. Helpers → 2. TTFTable → 3. Head/Maxp → 4. Loca → 5. Cmap → 6. Glyph →
7. Metrics → 8. Rasterizer (goldens) → 9. Atlas → 10. Integration → 11. Python.

Write the failing test first where a behavior is being added or a bug fixed,
watch it fail, then make it pass.

---

## Testing decisions to make

- Golden-image tolerance thresholds and how goldens are regenerated/reviewed.
- Whether to vendor a tiny test font or keep using `src/fonts/*` (the existing
  `.ttx` files are useful ground truth via fonttools).
- CTest layout: one binary with filters, or per-module binaries.
- CI: run goldens headless (pure CPU — no GL needed, which is the point).
- Coverage target for the parsing layer before declaring it "frozen".
