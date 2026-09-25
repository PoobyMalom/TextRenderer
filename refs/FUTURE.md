# TextRenderer — Phase 1 Future Work (Font Engine)

This document covers TTF/OpenType tables **not yet implemented** and not covered
by `PLANS.md`. Each entry describes the table's purpose, its binary layout (as
reference tables, not code), and what it unlocks for the engine.

> Framing note: "the renderer" below now means **any consumer of the engine** —
> the SDL demo, the Phase 2 OpenGL renderer, or the Python layer. The engine's
> job for each of these tables is to expose the parsed data via its public API;
> what unlocks a feature is the *consumer* acting on that data. Binary-layout
> blocks are kept as reference; implementation code has been removed per the
> docs convention (decisions and structure, not code).

**Already implemented:** `head`, `maxp`, `loca`, `cmap`, `glyf`
**Planned in `PLANS.md` (Phase 1 MVP):** `hhea`, `hmtx`
**Everything below:** future work, roughly ordered by value to the engine.

### Cross-cutting decisions for future tables
- **OpenType Layout shared infrastructure** (Coverage tables, ClassDef tables,
  ScriptList/FeatureList/LookupList) is needed by `GDEF`/`GSUB`/`GPOS`/`BASE`/
  `JSTF`. Decide to build that shared mini-parser **once** before tackling any
  of those tables, rather than per-table.
- **Where parsed data lives:** most of these belong on the `Font`/engine object,
  not on `Glyph`. Decide the accessor shape when you add the first one.
- **Shaping vs metrics:** `GSUB`/`GPOS` push the engine from "metrics provider"
  toward a "shaping engine." Decide if/when the engine owns shaping, or whether a
  separate shaping layer sits above it.

---

## Group 1 — Typography Essentials

These three tables are small, straightforward to parse, and immediately improve text quality.
Implement these first after PLANS.md is complete.

---

### `OS/2` — OS/2 and Windows Metrics

**What it gives you:** Better line height values than `hhea`, plus x-height, cap height,
weight class, underline/strikeout metrics, and the Unicode ranges the font covers.
`hhea` is the Apple baseline; `OS/2` is the Windows baseline. Most renderers prefer `OS/2`
when it's present.

**Key fields (version 0, 78 bytes):**

| Offset | Type | Name | Notes |
|---|---|---|---|
| 0 | uint16 | version | 0–5 |
| 2 | int16 | xAvgCharWidth | average width of all non-zero-width glyphs |
| 4 | uint16 | usWeightClass | 100=Thin, 400=Regular, 700=Bold, 900=Black |
| 6 | uint16 | usWidthClass | 1=UltraCondensed … 5=Normal … 9=UltraExpanded |
| 8 | uint16 | fsType | embedding permission flags |
| 28 | int16 | yStrikeoutSize | strikeout bar thickness |
| 30 | int16 | yStrikeoutPosition | strikeout position from baseline |
| 68 | int16 | sTypoAscender | typographic ascender (preferred for line height) |
| 70 | int16 | sTypoDescender | typographic descender (negative) |
| 72 | int16 | sTypoLineGap | additional line spacing |
| 74 | uint16 | usWinAscent | Windows-specific ascender |
| 76 | uint16 | usWinDescent | Windows-specific descender (positive value) |

**Version 2+ adds (at offset 86):**

| Offset | Type | Name | Notes |
|---|---|---|---|
| 86 | int16 | sxHeight | height of lowercase 'x' from baseline |
| 88 | int16 | sCapHeight | height of uppercase 'H' from baseline |
| 90 | uint16 | usDefaultChar | codepoint used when character is missing |
| 92 | uint16 | usBreakChar | word-break character (usually 0x0020 space) |

**Parsing approach:** read the fixed-layout fields in order, skipping the blocks
you don't need — subscript/superscript metrics, family class + PANOSE (18 bytes),
the four `unicodeRange` words (16 bytes), and `achVendID` (4 bytes) — then read
`fsSelection`, first/last char index, and the typographic metrics. The version 2+
fields (`sxHeight`, `sCapHeight`, …) come after an 8-byte `codePageRange` block
and should only be read when `version >= 2`. **Decision:** guard every
version-gated field on the parsed `version`, and clamp reads to the table length
so older 78-byte tables don't over-read.

**What it unlocks:**
- Correct typographic line height (`sTypoAscender - sTypoDescender + sTypoLineGap`)
- x-height guide line for the debug overlay
- Cap-height guide line for the debug overlay
- Strikeout rendering (size and position)
- Font weight/width metadata for the HUD
- Missing character fallback glyph (`usDefaultChar`)

---

### `name` — Naming Table

**What it gives you:** Every human-readable string in the font — family name, copyright,
designer, version, license, PostScript name, and more. Useful for the HUD and for font
file validation.

**Structure:**
```
uint16 format        (0 = basic, 2 = with lang tags)
uint16 count         number of name records
uint16 stringOffset  byte offset from start of table to string storage
```

Each of the `count` name records:
```
uint16 platformID    (1=Mac, 3=Windows)
uint16 encodingID    (1=Mac Roman, 1=Windows UCS-2)
uint16 languageID    (0=English for Mac, 0x0409=English US for Windows)
uint16 nameID        what this string represents (see list below)
uint16 length        byte length of the string
uint16 offset        byte offset from stringOffset to the string
```

**Important Name IDs:**

| ID | Meaning |
|---|---|
| 0 | Copyright notice |
| 1 | Font family name |
| 2 | Subfamily (Regular / Bold / Italic / Bold Italic) |
| 4 | Full font name |
| 5 | Version string (e.g. "Version 2.301") |
| 6 | PostScript name (e.g. "JetBrainsMono-Bold") |
| 8 | Manufacturer name |
| 9 | Designer name |
| 13 | License description |
| 16 | Typographic family name (preferred family, overrides ID 1) |
| 17 | Typographic subfamily |
| 19 | Sample text (sometimes set to "The quick brown fox…") |

**Parsing approach:**
Windows strings (platformID=3) are encoded as UTF-16 BE. Read pairs of bytes and combine
into `char32_t`, then convert to UTF-8 for storage. Mac strings (platformID=1) are
Mac Roman — a simple 8-bit encoding close to Latin-1.

Priority: prefer platformID=3, encodingID=1, languageID=0x0409. Fall back to platformID=1.

**What it unlocks:**
- HUD can show font family, subfamily, version, designer
- PostScript name for identification
- License text visible in an about screen
- Font file format validation (does the name match the file?)

---

### `post` — PostScript Table

**What it gives you:** Italic angle, underline metrics, whether the font is monospace, and
(version 2.0) glyph names. Underline and italic data are immediately useful for rendering.

**Structure (all versions share this header):**
```
Fixed   version            (0x00010000, 0x00020000, 0x00025000, 0x00030000)
Fixed   italicAngle        degrees of italic lean; 0.0 = upright, negative = italic
FWord   underlinePosition  from baseline; typically negative (below baseline)
FWord   underlineThickness thickness of the underline stroke
uint32  isFixedPitch       0 = proportional; non-zero = monospace (every glyph same width)
uint32  minMemType42       memory usage hints (ignore)
uint32  maxMemType42       memory usage hints (ignore)
uint32  minMemType1        memory usage hints (ignore)
uint32  maxMemType1        memory usage hints (ignore)
```

**Version 2.0 glyph names (after header):**
```
uint16 numGlyphs
uint16 glyphNameIndex[numGlyphs]   indices 0–257 = standard Mac names; 258+ = custom
Pascal strings for custom names (uint8 length + bytes, no null terminator)
```

Standard Mac glyph name list: `.notdef`, `space`, `A`–`Z`, `a`–`z`, `zero`–`nine`, etc.
(258 predefined entries). Index < 258 → look up in the standard table. Index >= 258 →
read the (index - 258)'th Pascal string from the custom name array.

**What it unlocks:**
- Underline rendering: draw a horizontal bar at `underlinePosition` with `underlineThickness`
- Italic angle: could be used to shear the rasterized glyph for synthetic italics
- `isFixedPitch`: confirms the font is monospace; could be shown in HUD
- Glyph names: useful for debugging (e.g. print the glyph name alongside its outline in wireframe mode)

---

### `kern` — Kerning Table

**What it gives you:** Spacing adjustments between specific pairs of glyphs. The classic
example: "VA" looks too wide without kerning; the kern table says "subtract 80 units
between V and A". Modern fonts use GPOS instead, but many TTF files still include `kern`.

**Structure:**
```
uint16 version    (0)
uint16 nTables    number of subtables
```

Each subtable:
```
uint16 version    (0)
uint16 length     total byte length of subtable including this header
uint16 coverage   bit field:
    bit 0: horizontal (1) vs vertical (0)
    bit 1: minimum (instead of additive)
    bit 2: cross-stream kerning
    bit 3: override existing kern
    bits 8–15: format (0 = pair list, 2 = class-based)
```

**Format 0 (ordered pair list):**
```
uint16 nPairs
uint16 searchRange
uint16 entrySelector
uint16 rangeShift
// nPairs of:
uint16 left     glyph index of left glyph
uint16 right    glyph index of right glyph
int16  value    kern value in font units (negative = tighter, positive = looser)
```

Pairs are sorted by (left << 16 | right) so binary search works.

**Format 2 (class-based):** Maps glyphs to classes, then indexes a 2D class×class table.
More compact. Parse class arrays and look up `table[leftClass][rightClass]`.

**Parsing approach:**
Build a `std::unordered_map<uint32_t, int16_t>` keyed by `(leftGlyph << 16) | rightGlyph`.
Call it in the layout engine: after placing glyph N, before advancing pen by advanceWidth,
look up `kern[glyphN][glyphN+1]` and add the value to the advance.

**What it unlocks:**
- Correct pair spacing for professional-quality text rendering
- Noticeable improvement especially for capital letters (AV, WA, To, etc.)

---

## Group 2 — OpenType Layout Engine (GSUB / GPOS / GDEF)

These three tables form the OpenType Layout engine. They are significantly more complex
than anything parsed so far — each uses a shared set of lookup table structures that
require their own mini-parser. Implement them as a unit since GDEF is needed to use
GPOS, and GPOS references GDEF.

---

### `GDEF` — Glyph Definition Table

**What it gives you:** Classifies every glyph as Base (1), Ligature (2), Mark (3), or
Component (4). Required by GPOS to know which positioning lookups apply to which glyphs.
Also provides ligature caret positions (for cursor placement inside ligatures).

**Structure:**
```
uint16 majorVersion (1)
uint16 minorVersion (0, 2, or 3)
uint16 glyphClassDefOffset    offset to ClassDef for glyph categories
uint16 attachListOffset       offset to attachment point list (may be 0)
uint16 ligCaretListOffset     offset to ligature caret list (may be 0)
uint16 markGlyphSetsDefOffset offset to mark glyph sets (v1.2+, may be 0)
```

**ClassDef table** (used by many OpenType tables):
- Format 1: start glyph + array of class values (dense range)
- Format 2: array of (startGlyph, endGlyph, class) ranges (sparse)

Build a `std::vector<uint16_t> glyphClass(numGlyphs)` from the ClassDef.

**Glyph classes:**
- 0 = not assigned (treated as Base)
- 1 = Base glyph
- 2 = Ligature glyph
- 3 = Mark glyph (diacritics, combining characters)
- 4 = Component (part of a compound glyph at the base level)

**What it unlocks:**
- Enables GPOS mark positioning (diacritics over base glyphs)
- Enables correct ligature caret placement for text editors
- Prerequisite for GPOS and GSUB processing

---

### `GSUB` — Glyph Substitution Table

**What it gives you:** Glyph substitution rules. The most immediately useful feature is
**ligature substitution** — replacing sequences like `f`+`i` with the `fi` ligature glyph,
or `f`+`f`+`l` with `ffl`. Also handles alternate forms, stylistic sets, small caps, etc.

**Top-level structure:**
```
uint16 majorVersion (1)
uint16 minorVersion
uint16 scriptListOffset
uint16 featureListOffset
uint16 lookupListOffset
```

All three of these (ScriptList, FeatureList, LookupList) use the same shared OpenType
structure. The LookupList is the most important:

```
LookupList:
  uint16 lookupCount
  uint16 lookupOffsets[lookupCount]

Lookup:
  uint16 lookupType      (1–8 for GSUB; see below)
  uint16 lookupFlag      bit field (process right-to-left, skip marks, etc.)
  uint16 subTableCount
  uint16 subTableOffsets[subTableCount]
```

**GSUB Lookup Types:**

| Type | Name | Description |
|---|---|---|
| 1 | Single | One glyph → one glyph (e.g. smcp: 'a' → small-cap 'a') |
| 2 | Multiple | One glyph → sequence (decompose ligature) |
| 3 | Alternate | One glyph → one of several alternates |
| 4 | Ligature | Sequence → one glyph (fi, fl, ffi, ffl, etc.) |
| 5 | Context | Sequence-dependent substitution |
| 6 | Chained Context | Context with lookahead and lookbehind |
| 7 | Extension | Points to another lookup type (allows table > 64KB) |
| 8 | Reverse Chained | Used for RTL scripts |

**Ligature Substitution (Type 4) — parse this first:**
```
uint16 substFormat  (1)
uint16 coverageOffset
uint16 ligSetCount
uint16 ligSetOffsets[ligSetCount]

LigatureSet (one per first-glyph):
  uint16 ligatureCount
  uint16 ligatureOffsets[ligatureCount]

Ligature:
  uint16 ligGlyph            the replacement glyph
  uint16 componentCount      number of glyphs in the sequence (including first)
  uint16 componentGlyphIDs[componentCount - 1]  rest of sequence (first is implicit)
```

**Coverage tables** (shared by all lookup types):
```
Format 1: explicit list of glyph IDs
Format 2: ranges of glyph IDs
```
A coverage table answers: "is glyph G covered by this lookup, and if so, what is its coverage index?"

**Applying GSUB in the layout engine:** run the glyph sequence through each
active lookup in feature order. For ligatures, scan the sequence; wherever a
glyph matches the first component of a ligature and the following glyphs match
the remaining components, replace that run with the single ligature glyph and
re-check from the same position (a new ligature may now start there). **Decision:**
this is the point where the engine grows a "shaping" step that mutates the glyph
run before metrics/positioning — decide whether that lives in the engine or a
layer above it (see the cross-cutting note at the top).

Active features are selected by the script/language system. Common features:
- `liga` — standard ligatures (fi, fl, ff, ffi, ffl) — on by default
- `clig` — contextual ligatures — on by default
- `dlig` — discretionary ligatures — off by default
- `smcp` — small capitals — off by default
- `onum` — oldstyle numerals — off by default
- `ss01`–`ss20` — stylistic sets — off by default

**What it unlocks:**
- Ligature rendering (fi, fl, ffi, ffl — the most visible improvement)
- Alternate glyph support
- Small caps, oldstyle numerals, stylistic alternates
- Correct rendering of complex scripts (Arabic, Devanagari) when combined with GPOS

---

### `GPOS` — Glyph Positioning Table

**What it gives you:** Advanced glyph positioning — the modern replacement for `kern`.
Much more powerful: supports class-based kerning, mark-to-base diacritic positioning,
cursive attachment, and more. JetBrainsMono uses GPOS for kerning.

**Top-level structure:** Same as GSUB (ScriptList, FeatureList, LookupList).

**GPOS Lookup Types:**

| Type | Name | Description |
|---|---|---|
| 1 | Single | Adjust position of one glyph (e.g. shift punctuation) |
| 2 | Pair | Kerning between pairs — **implement this first** |
| 3 | Cursive | Attach cursive glyphs (Arabic) |
| 4 | Mark-to-Base | Position diacritics over base glyphs |
| 5 | Mark-to-Ligature | Position diacritics over ligatures |
| 6 | Mark-to-Mark | Stack combining marks |
| 7 | Context | Context-sensitive positioning |
| 8 | Chained Context | Lookahead/lookbehind positioning |
| 9 | Extension | Points to another lookup type |

**ValueRecord** (used by most lookup types):
```
int16 xPlacement    shift glyph horizontally (does not change advance)
int16 yPlacement    shift glyph vertically
int16 xAdvance      adjust advance width (like kerning)
int16 yAdvance      adjust advance height
// + optional device tables for pixel-grid correction
```
A `valueFormat` bitfield (uint16) says which of these fields are present.

**Pair Adjustment — Type 2, Format 1 (glyph pairs):**
```
uint16 posFormat    (1)
uint16 coverageOffset
uint16 valueFormat1  (for first glyph)
uint16 valueFormat2  (for second glyph)
uint16 pairSetCount
uint16 pairSetOffsets[pairSetCount]

PairSet:
  uint16 pairValueCount
  PairValueRecord:
    uint16      secondGlyph
    ValueRecord value1   (applied to first glyph's advance)
    ValueRecord value2   (applied to second glyph's advance — usually empty)
```

**Pair Adjustment — Type 2, Format 2 (class-based, more common):**
```
uint16 posFormat    (2)
uint16 coverageOffset
uint16 valueFormat1
uint16 valueFormat2
uint16 classDef1Offset   classes for first glyph
uint16 classDef2Offset   classes for second glyph
uint16 class1Count
uint16 class2Count
Class1Record[class1Count]:
  Class2Record[class2Count]:
    ValueRecord value1
    ValueRecord value2
```

Lookup: `table[classDef1[glyph1]][classDef2[glyph2]].value1.xAdvance`

**Mark-to-Base (Type 4) — for diacritics:**
Requires GDEF to identify which glyphs are Marks vs Base.
Provides anchor points: "the top-center of base glyph B has anchor at (x, y); the bottom-center of mark M has anchor at (x, y); align those anchors."

```
MarkArray: anchor point for each mark glyph (with mark class)
BaseArray: anchor point per mark class for each base glyph
```

**What it unlocks:**
- Correct kerning for modern fonts that rely on GPOS instead of `kern`
- Diacritic positioning (é, ü, ñ etc. render correctly over their base glyphs)
- Cursive script support
- Much more typographically correct output overall

---

## Group 3 — Variable Fonts

Variable fonts store a single font file that covers a continuous range of styles
(weight, width, slant, etc.). The `fvar` table defines the axes; `gvar`, `avar`, and `cvar`
provide the interpolation data.

---

### `fvar` — Font Variations

**What it gives you:** The list of variation axes and their ranges. After parsing this,
the user can pick a point in the design space and get a unique font style.

**Structure:**
```
uint16 majorVersion  (1)
uint16 minorVersion  (0)
uint16 axesArrayOffset
uint16 reserved      (2)
uint16 axisCount
uint16 axisSize      (20)
uint16 instanceCount
uint16 instanceSize
```

Each VariationAxis (20 bytes):
```
uint32 axisTag      4-char tag (see below)
Fixed  minValue     minimum axis value (Fixed = 16.16 fixed-point)
Fixed  defaultValue
Fixed  maxValue
uint16 flags        bit 0: hidden axis (don't show to user)
uint16 axisNameID   name table ID for the axis label
```

**Well-known axes:**

| Tag | Name | Typical Range | Notes |
|---|---|---|---|
| `wght` | Weight | 100–900 | 400=Regular, 700=Bold |
| `wdth` | Width | 75–125 | percent of normal width |
| `ital` | Italic | 0–1 | discrete (0=upright, 1=italic) |
| `slnt` | Slant | -90–90 | degrees of slant |
| `opsz` | Optical Size | 6–72 | point size for optical adjustments |
| `GRAD` | Grade | 0–150 | weight without changing width |
| `XHGT` | X-Height | varies | x-height scaling |

Each NamedInstance:
```
uint16 subfamilyNameID
uint16 flags
Fixed  coordinates[axisCount]  one value per axis
```

**What it unlocks:**
- UI sliders for each axis in the HUD
- Real-time interpolation between font styles
- Single font file that covers Regular, Bold, Light, Condensed, etc.

---

### `gvar` — Glyph Variations

**What it gives you:** Per-glyph deltas for each point as axes are adjusted. This is
the core data for variable font interpolation.

**Overview of approach:**
1. Start with the glyph's default point coordinates from `glyf`
2. Normalize the axis values to [-1, 0, 1] range
3. For each tuple variation record in `gvar` for this glyph:
   - Compute the scalar from the axis values and this tuple's region
   - Multiply each point delta by the scalar
   - Add to the glyph's coordinates
4. Render the interpolated glyph

**Structure:** Complex. Each glyph has a GlyphVariationData block containing:
- Shared tuples (global) and per-glyph tuple variation headers
- Each header: axis region (peak/start/end per axis), flags, point deltas

Point deltas are packed using a run-length encoding similar to glyph coordinate packing.
Phantom points (4 extra points representing metrics) must also be varied.

**Implementation note:** This is the most complex table in variable fonts. Tackle `fvar`
and `avar` first to understand the axis normalization, then `gvar`.

---

### `avar` — Axis Variations

**What it gives you:** Normalizes axis coordinates through a piecewise linear mapping.
Raw user values (e.g. weight=600) are mapped to normalized [-1, 1] space via `avar`.

**Structure:**
```
uint16 majorVersion  (1)
uint16 minorVersion  (0)
uint16 reserved
uint16 axisCount
```

Per-axis SegmentMap:
```
uint16 positionMapCount
AxisValueMap[positionMapCount]:
  F2Dot14 fromCoordinate  input value (normalized: -1.0, 0.0, +1.0)
  F2Dot14 toCoordinate    output value
```

The default normalization maps [minValue, defaultValue, maxValue] → [-1, 0, +1] linearly.
`avar` adds extra kinks to that curve. If there's no `avar` entry for an axis, use the
linear default.

---

### `cvar` — CVT Variations

**What it gives you:** Variations on the control value table (`cvt`) entries for hinting.
Only matters if you implement TrueType hinting. Can be deferred indefinitely.

---

### `STAT` — Style Attributes Table

**What it gives you:** A machine-readable description of the design space for variable
fonts. Lets the font declare things like "weight 700 = Bold" with a name ID, so
applications can label axes properly.

Structure: axis records (similar to `fvar`) + axis value records that map specific
design-space positions to named styles. Parse after `fvar` — use `STAT` to display
named stops ("Light", "Regular", "Bold") on each axis slider in the UI.

---

## Group 4 — Color Fonts

Color fonts embed colored artwork directly in the font file. There are three competing
standards; many fonts ship multiple formats.

---

### `COLR` + `CPAL` — Layered Color Glyphs

**What it gives you:** Simple layered color glyphs. Each colored glyph is a stack of
monochrome glyph outlines, each filled with a color from a palette. The simplest color
format — parse these two tables as a unit.

**`CPAL` structure (version 0):**
```
uint16 version            (0)
uint16 numPaletteEntries  colors per palette
uint16 numPalettes
uint16 numColorRecords
uint32 colorRecordsArrayOffset
uint16 colorRecordIndices[numPalettes]  starting index for each palette
```

ColorRecord (4 bytes, little-endian BGRA):
```
uint8 blue
uint8 green
uint8 red
uint8 alpha
```

**`COLR` structure (version 0):**
```
uint16 version
uint16 numBaseGlyphRecords
uint32 baseGlyphRecordsOffset
uint32 layerRecordsOffset
uint16 numLayerRecords
```

BaseGlyphRecord:
```
uint16 glyphID          the colored glyph (as looked up by cmap)
uint16 firstLayerIndex  index into LayerRecord array
uint16 numLayers
```

LayerRecord:
```
uint16 glyphID        outline glyph to draw for this layer
uint16 paletteIndex   color index in CPAL (0xFFFF = foreground color)
```

**Rendering approach:**
When looking up a glyph to render, check if it appears in the COLR base glyph list.
If so, for each layer: rasterize `layer.glyphID` as a monochrome filled glyph, then
colorize it with `palette[layer.paletteIndex]` and composite over the previous layers.

COLR version 1 (draft/standard) adds paint graphs, gradients, compositing — much more
complex. Tackle version 0 first.

**What it unlocks:**
- Emoji fonts (Twemoji, Noto Color Emoji use COLR/CPAL)
- Colored icon fonts
- Multi-color decorative fonts

---

### `SVG ` — SVG Color Glyphs

**What it gives you:** Full SVG documents embedded per glyph. The highest-quality color
format but requires an SVG renderer.

**Structure:**
```
uint16 version  (0)
uint32 svgDocumentListOffset
uint32 reserved (0)
```

SVGDocumentList:
```
uint16 numEntries
SVGDocumentRecord[numEntries]:
  uint16 startGlyphID
  uint16 endGlyphID    one SVG doc can cover a range of glyphs
  uint32 svgDocOffset  offset from SVGDocumentList start
  uint32 svgDocLength  byte length; data may be gzip-compressed
```

SVG data is raw UTF-8 SVG (or gzip-compressed). Glyphs within the range are identified
by `id="glyph{N}"` within the SVG.

**Implementation note:** Requires a full SVG parser and renderer. The simplest approach
is to link against `librsvg` or `nanosvg`. Out of scope until the core renderer is solid.

---

### `CBDT` / `CBLC` — Color Bitmap Glyphs

**What it gives you:** Pre-rasterized RGBA bitmap images for glyphs at specific sizes.
Used by Google's Noto Color Emoji. Not scalable — each bitmap is valid at one PPEM.

`CBLC` (Color Bitmap Location Table): index of which sizes and glyphs have bitmaps.
Same structure as `EBLC` but with color-aware strike headers.

`CBDT` (Color Bitmap Data Table): the actual image data. Format 17 is 32-bit RGBA
raw pixels. Format 18/19 is PNG-compressed.

For PNG-compressed bitmaps: link against `libpng` or `stb_image` to decode.

**What it unlocks:** Pre-rendered emoji at common sizes without needing a rasterizer.

---

## Group 5 — Hinting Tables

TrueType hinting is a bytecode interpreter that adjusts glyph point positions at specific
pixel sizes to align to the pixel grid. It significantly improves legibility at small
sizes (8–14px). At large sizes (40px+) hinting is largely irrelevant.

Hinting is the most complex subsystem in the entire TTF spec. Implementing a full hinting
engine is a multi-month project in itself. The tables are listed here for completeness.

---

### `cvt ` — Control Value Table

A simple array of `int16_t` values in font units used as reference measurements by the
hinting bytecode. Glyph programs read and write CVT entries to coordinate consistent
heights across the font (e.g. "all lowercase letters must have the same x-height").

```
// numEntries = table.length / 2
int16_t cvt[numEntries];
```

---

### `fpgm` — Font Program

A block of TrueType bytecode that runs once when the font is loaded. Typically defines
function definitions (FDEFs) that glyph programs call. The TrueType instruction set
is a stack machine with ~200 opcodes.

```
uint8_t program[table.length];   // bytecode to execute
```

---

### `prep` — Control Value Program (Pre-Program)

Bytecode that runs each time the rendering size or transform changes. Sets up CVT values
for the current size. Typically handles things like "at 11px, round stems to exactly 1
pixel; at 12px, round to exactly 1 pixel plus half a pixel."

```
uint8_t program[table.length];   // bytecode to execute
```

---

### `gasp` — Grid-fitting and Scan-conversion Procedure

**What it gives you:** Advice on which rendering behaviors to use at each size. This is
the only hinting-related table you might want to parse without implementing the full
hinting engine — it tells you whether to apply grayscale AA at the current PPEM.

**Structure:**
```
uint16 version    (0 or 1)
uint16 numRanges
```

Each GaspRange:
```
uint16 rangeMaxPPEM    upper bound (inclusive) of this size range
uint16 rangeGaspBehavior   bit field:
    bit 0: GASP_GRIDFIT         apply grid-fitting (hinting)
    bit 1: GASP_DOGRAY          apply grayscale anti-aliasing
    bit 2: GASP_SYMMETRIC_GRIDFIT   ClearType symmetric grid-fit (v1+)
    bit 3: GASP_SYMMETRIC_SMOOTHING ClearType AA (v1+)
```

**Usage:** Before rasterizing at a given PPEM, look up which `rangeMaxPPEM >= currentPPEM`
applies. If `GASP_DOGRAY` is set, use anti-aliasing. If not, use black-and-white threshold
rendering. This is useful even without full hinting.

---

## Group 6 — Vertical Text

These tables enable correct typesetting of vertical CJK text (Japanese, Chinese, Korean
written top-to-bottom).

---

### `vhea` — Vertical Header

Analogous to `hhea` but for vertical layout. Contains the vertical ascender, descender,
line gap, and number of vertical metrics entries.

```
Fixed   version          (1.0 or 1.1)
int16   ascent           units above center of em square in vertical layout
int16   descent          units below center (negative)
int16   lineGap
uint16  advanceHeightMax
int16   minTopSideBearing
int16   minBottomSideBearing
int16   yMaxExtent
int16   caretSlopeRise
int16   caretSlopeRun
int16   caretOffset
int16   reserved[4]
int16   metricDataFormat (0)
uint16  numberOfVMetrics
```

---

### `vmtx` — Vertical Metrics

Analogous to `hmtx`. For each glyph (up to `numberOfVMetrics` from `vhea`), stores the
vertical advance height and top side bearing. Glyphs beyond `numberOfVMetrics` share the
last `advanceHeight`.

```
// numberOfVMetrics entries:
uint16 advanceHeight
int16  topSideBearing

// remaining glyphs (numGlyphs - numberOfVMetrics):
int16  topSideBearing
```

---

### `VORG` — Vertical Origin

For CJK fonts in vertical layout, specifies the y-coordinate of the vertical origin point
for each glyph (the point used as the baseline in vertical text). Most glyphs use the
`defaultVertOriginY` but individual overrides are common.

```
uint16 majorVersion          (1)
uint16 minorVersion          (0)
int16  defaultVertOriginY    y-coordinate of vertical origin for most glyphs
uint16 numVertOriginYMetrics
// per-glyph overrides:
uint16 glyphIndex
int16  vertOriginY
```

---

## Group 7 — Advanced Typography

---

### `BASE` — Baseline Table

**What it gives you:** Baseline offsets for different writing systems when mixing scripts
on the same line. Defines where the alphabetic, hanging, ideographic, and math baselines
are relative to each other. Needed for correct mixed-script layout (Latin + Devanagari,
Latin + CJK, etc.).

Structure uses script list and language system tables (same pattern as GSUB/GPOS).
Per script, per baseline tag (e.g. `ideo`, `romn`, `hang`): an offset from the default
baseline in font units.

---

### `JSTF` — Justification Table

**What it gives you:** Rules for full-justification (stretching text to fill a line).
Defines which GSUB/GPOS lookups to enable/disable and what spacing adjustments to make
to achieve justification. Required for publication-quality justified text.

Structure is complex — uses the same lookup infrastructure as GSUB/GPOS. Contains
`JstfScript` records per script, each with `JstfPriority` levels (try lower priorities
first before more aggressive stretching).

---

### `MATH` — Mathematical Typesetting

**What it gives you:** Everything needed to render mathematical formulas correctly.
Only relevant for math fonts (XITS Math, Libertinus Math, Cambria Math, etc.).

Contains ~60 constants like:
- `scriptPercentScaleDown` — how much to shrink superscripts
- `superscriptShiftUp` — how high to raise superscripts
- `fractionNumeratorShiftUp` — numerator position in a fraction
- `radicalVerticalGap` — space above the radicand under a radical sign

Also: italic corrections per glyph, math kerning (different from regular kerning),
and top accent attachments (how to center accents over math glyphs).

---

## Group 8 — Rarely Needed

These tables exist in many fonts but provide information of limited value for a renderer.
Parse them only if specifically needed.

---

### `hdmx` — Horizontal Device Metrics

Pre-computed advance widths at specific pixel sizes, optimized for specific device
resolutions. Essentially a cache of hinted advance widths. Rarely used in modern
high-DPI rendering environments. Only useful if implementing pixel-grid-aligned advance
widths without a hinting engine.

---

### `LTSH` — Linear Threshold Table

For each glyph, the PPEM at or above which the glyph's advance width scales linearly
(i.e. hinting stops changing it). Used to know when you can skip looking up `hdmx`
values. Not useful without `hdmx`.

---

### `DSIG` — Digital Signature

A cryptographic signature over the font file. Parse only if you want to verify font
authenticity. The signature is a PKCS#7 object and requires a crypto library to verify.
Completely irrelevant to rendering.

---

### `meta` — Metadata

Key-value pairs of font metadata. Common tags:
- `dlng` — design language(s) (e.g. "Latn" for Latin)
- `slng` — supported language(s)

Rarely populated; mainly informational.

---

### `PCLT` — PCL 5 Data

Legacy Hewlett-Packard PCL 5 printer metrics. Only relevant for driving PCL printers.
Not useful for screen rendering.

---

## Implementation Priority Summary

| Priority | Table(s) | Effort | Payoff |
|---|---|---|---|
| 1 | `OS/2` | Low | Better line metrics, x-height/cap-height guides |
| 1 | `name` | Low | Font identity in HUD |
| 1 | `post` | Low | Underline rendering, italic angle, monospace flag |
| 2 | `kern` | Medium | Correct pair spacing for fonts without GPOS |
| 3 | `GDEF` | Medium | Prerequisite for GPOS/GSUB |
| 3 | `GSUB` (Type 4) | High | Ligatures (fi, fl, ffi, ffl) |
| 3 | `GPOS` (Type 2) | High | Modern kerning, mark positioning |
| 4 | `fvar` + `avar` | Medium | Variable font axis UI |
| 4 | `gvar` | Very High | Variable font interpolation |
| 5 | `COLR` + `CPAL` | Medium | Color emoji, layered color fonts |
| 5 | `gasp` | Low | AA behavior hints by size |
| 6 | `cvt ` + `fpgm` + `prep` | Very High | TrueType hinting engine |
| 7 | `vhea` + `vmtx` + `VORG` | Medium | Vertical CJK text |
| 7 | `SVG ` | High | SVG color glyphs (needs SVG renderer) |
| 7 | `CBDT` + `CBLC` | Medium | Color bitmap emoji (needs PNG decoder) |
| 8 | `BASE` + `JSTF` | Very High | Mixed-script & justified text |
| 8 | `MATH` | High | Math formula rendering |
| 9 | `hdmx`, `LTSH`, `DSIG`, `meta` | Low | Rarely needed |
