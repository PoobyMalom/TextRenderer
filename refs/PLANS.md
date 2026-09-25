# TextRenderer — Phase 1 Implementation Plan (Font Engine)

> This is the **mid-level** breakdown of Phase 1. The high-level vision and EU
> sizing live in `FULL_PLAN.md` §2. This document expands each milestone into
> concrete sub-steps and the **design decisions** you still need to make. No
> code — decisions and structure only.

---

## Goal

Turn the current SDL text viewer into a real, reusable **font engine library**
(`libtr_font`): given a `.ttf` file and a target pixel size, it produces correct
**metrics**, **filled anti-aliased glyph bitmaps**, and a packed **texture
atlas** — with **no SDL or OpenGL in its public headers**. It ships as a CMake
package and a pybind11 Python module. An SDF atlas is *designed* now and built
later (when Phase 3's zoom demands it).

The interactive viewer (camera, HUD, live controls) is **no longer the
deliverable** — it becomes an optional demo consumer under `examples/`. The
real zoomable renderer is Phase 2.

---

## Relationship to the old plan

The previous version of this document planned an interactive SDL2 viewer with a
glyph cache of `SDL_Texture`s, a camera, and an on-screen HUD. That work is
**superseded**: caching/camera/HUD belong to the Phase 2 renderer, and the
engine must not depend on SDL. What carries over is the *core new work* — the
CPU rasterizer (Bézier flattening + scanline fill + AA) — now producing
backend-neutral bitmaps instead of `SDL_Surface`s.

---

## Current state

- TTF parsing works: `head`, `maxp`, `loca`, `cmap` (formats 0/4/12), `glyf`
  (simple + compound).
- Glyphs render as Bézier **outlines only** (stroked), via SDL line drawing.
- No `hhea`/`hmtx` → advance width hardcoded to 600 font units.
- No fill, no anti-aliasing, no atlas, no metrics.
- Engine still coupled to SDL through `Renderer.{h,cpp}`.
- Build is a makefile; CI + clang-tidy in place; gtest scaffold present
  (placeholder test only).
- The large correctness/lint/leak cleanup (`TODO.md`) is essentially complete.

---

## Target architecture

```
TTF File
   │
   ▼
[Parsing Layer]        (exists — freeze once hhea/hmtx land)
   │  TTFFile, Glyph, Cmap/Loca/Head/Maxp + NEW Hhea/Hmtx
   │
   ▼
[Metrics]              (new)  advance widths, bearings, ascender/descender,
   │                          unitsPerEm — one documented font-unit→pixel transform
   ▼
[CPU Rasterizer]       (new — the core work)
   │  flatten quadratic Béziers → nonzero-winding scanline fill →
   │  grayscale coverage bitmap → anti-alias (supersample first)
   ▼
[Atlas Builder]        (new)  pack glyph bitmaps into one grayscale texture +
   │                          per-glyph UV rect + metrics
   ▼
[Public API]  (tr/font.h)  ── no SDL, no GL ──▶  consumers:
                                                  • examples/sdl_demo
                                                  • Phase 2 renderer
                                                  • Python (pybind11)
```

**Keystone rule:** nothing in `libtr_font`'s public headers may include SDL or
OpenGL. The bitmap is plain bytes; the consumer uploads it.

---

## Milestones

Each milestone lists *what*, *why it's ordered here*, the *design decisions* you
own, and a *Definition of Done*.

### P1.0 — Land the cleanup, cut a baseline

- **What:** close remaining `TODO.md` items, merge `memory-leak-fixs`, tag
  `v0.1.0` (the outline-renderer baseline).
- **Why now:** you want a known-good fallback before architectural surgery.
- **Decisions:** none significant.
- **DoD:** `main` builds clean, clang-tidy green, tag exists.

### P1.1 — Horizontal metrics (`hhea` + `hmtx`)

- **What:** parse `hhea` (ascender, descender, lineGap, numberOfHMetrics) and
  `hmtx` (per-glyph advanceWidth + leftSideBearing, including the trailing
  lsb-only run). Replace the hardcoded 600.
- **Why now:** small, unblocks correct spacing everywhere downstream.
- **Decisions to make:**
  - Where do per-glyph metrics live — on `Glyph`, or in a separate `Metrics`
    object keyed by glyph index? (Recommend separate; keeps `Glyph` about
    outlines.)
  - Default line height source now (`hhea`) vs later (`OS/2`, see `FUTURE.md`).
- **DoD:** a proportional font advances correctly; golden test on a known
  advance value.

### P1.2 — `unitsPerEm` + the canonical coordinate transform

#### Background: what is `unitsPerEm`?

A font defines all of its geometry — glyph outlines, advance widths, bearings,
ascender, descender — in an abstract integer grid called **font units**. The
size of that grid is `unitsPerEm`: common values are 1000 (PostScript-origin
fonts) and 2048 (TrueType). It has nothing to do with pixels; it's just the
resolution the type designer chose to draw at.

The **em** is the reference unit — historically the height of the metal block a
letter was cast on, and today the square that nominally contains a capital
letter. If `unitsPerEm` is 2048, a capital H might be ~1400 units tall, and an
advance width might be ~1200 units. None of those numbers mean anything until
you divide by `unitsPerEm` and multiply by the desired pixel size:

```
pixels = fontUnits * pixelsPerEm / unitsPerEm
```

This is why two fonts with different `unitsPerEm` values can render at the same
visual size given the same `pixelsPerEm` — the denominator normalises them.

- **What:** read `unitsPerEm` from `head`; write `docs/coordinates.md`; replace
  ad-hoc scaling with one documented `fontUnits → pixels` transform.
- **Why now:** every later placement bug traces back to this; fix the mental
  model before the rasterizer depends on it.

#### Sub-steps

1. **Confirm `unitsPerEm` is already parsed.** `head` is parsed via
   `HeadTable::parseHeadDirectory` — check that `unitsPerEm` is a field and
   accessible. It almost certainly is; this is a quick verify.

2. **Define the one canonical transform.** The formula is:

   ```
   pixels = fontUnits * pixelsPerEm / unitsPerEm
   ```

   where `pixelsPerEm` is the caller's requested render size in pixels (e.g. 32
   for a 32px font). Write this as a free function or small struct in
   `tr/Metrics.h` (or a new `tr/Transform.h`):

   ```cpp
   struct FontTransform {
       float pixelsPerEm;
       uint16_t unitsPerEm;
       float toPixels(int16_t fontUnits) const {
           return fontUnits * pixelsPerEm / unitsPerEm;
       }
   };
   ```

   Every placement value — advance widths, bearings, ascender, descender,
   glyph coordinates — must go through this and **nothing else**.

3. **Rip out the ad-hoc `scalingFactor`.** The current `double scalingFactor =
   0.1` in `main.cpp` is an unmotivated constant that approximates the real
   transform for one font at one size. Replace it: `pixelsPerEm` is the user
   control (the +/- keys now adjust `pixelsPerEm`, not a raw scale), and the
   transform does the rest.

4. **Fix the Y-axis.** TTF glyph coordinates are Y-up (positive Y goes toward
   the ascender). SDL and GPU texture rows are Y-down. The transform must flip:

   ```
   screenY = baselineY - toPixels(fontY)
   ```

   Document this flip in `docs/coordinates.md` with a diagram. This is the most
   common source of upside-down glyphs.

5. **Write `docs/coordinates.md`.** Cover:
   - The font-unit grid and what `unitsPerEm` means (the cap-height reference)
   - The baseline, ascender, descender in font units (from `hhea`)
   - The Y-up → Y-down flip at the render boundary
   - The one `fontUnits → pixels` formula and where it lives in code
   - A worked example: advance a cursor for the string "Ag" at 32px on a
     2048-UPM font

6. **Golden test.** Render the same string ("Hello") from two fonts with
   different `unitsPerEm` values at the same `pixelsPerEm`. Assert that the
   pixel-space advance widths are within 1px of each other for comparable
   glyphs. This verifies the transform is actually being applied, not bypassed.

#### Decisions to make

- **Y-axis convention for engine output bitmaps:** Y-down (atlas rows top-to-
  bottom) — conventional for GPU upload. Pick this, document it, don't revisit.
- **Pixel-size input unit:** pixels-per-em as the engine's native unit. Let
  callers convert from point size: `ppem = pointSize * dpi / 72`. Keep the
  conversion out of the engine.
- **Where the transform lives:** recommend a single `FontTransform` struct in
  the metrics layer, not scattered casts. All coordinate math imports it.

#### DoD

- `docs/coordinates.md` exists and is linked from a comment at the top of
  `Metrics.h` (or wherever `FontTransform` lives).
- `scalingFactor` is gone from `main.cpp`; +/- keys adjust `pixelsPerEm`.
- Two fonts with different `unitsPerEm` render at the same visual size at the
  same `pixelsPerEm` — verified by eye and by the golden test.

### P1.3 — Decouple the engine core from SDL

- **What:** move `Renderer.{h,cpp}` and the SDL `main` into
  `examples/sdl_outline_demo/`. Introduce a thin public facade header
  (`tr/font.h`) that does not expose raw table classes.
- **Why now:** do this *before* writing the rasterizer so new code is born
  backend-neutral.
- **Decisions to make:**
  - Facade shape: free functions vs a `Font` class with methods? (Recommend a
    small `Font` class — maps cleanly to the Python binding later.)
  - How much of the parser is public vs internal (pimpl/`detail` namespace)?
  - What's the canonical bitmap type the engine returns (e.g. a `Bitmap` struct:
    width, height, stride, `vector<uint8_t>` coverage)?
- **DoD:** a test translation unit including only `tr/font.h` compiles with **no
  SDL on the include path**.

### P1.4 — Contour flattening + scanline fill rasterizer

- **What:** flatten quadratic Béziers to line segments at a pixel tolerance;
  implement a **nonzero-winding** scanline fill into a grayscale coverage
  buffer.
- **Why now:** this is the core capability the whole phase exists for.
- **Decisions to make:**
  - Flattening strategy: adaptive subdivision (flatness threshold) vs fixed step
    count. (Recommend adaptive with a ~0.3–0.5px deviation threshold.)
  - Winding rule confirmation: TrueType is **nonzero**, not even-odd — overlaps
    (counters, bars) depend on it. Lock this in a test.
  - Coverage representation: 8-bit now, or accumulate in float then quantize?
  - How to treat empty glyphs (space) — zero-size bitmap, not an error.
- **DoD:** 'O', 'B', 'g', 'A', '@' fill with correct holes; visual diff against a
  FreeType render of the same glyph/size is within tolerance.

### P1.5 — Anti-aliasing

- **What:** start with **supersampling** (rasterize 3×–4×, box-downsample to
  coverage). Document the path to analytic/signed-area AA as a later upgrade.
- **Why now:** AA only makes sense once fill is correct.
- **Decisions to make:**
  - Supersample factor (memory vs quality) and whether it's configurable.
  - Gamma: blend coverage in linear vs sRGB? (Note it; correct gamma matters for
    perceived weight — can defer but document.)
- **DoD:** edges smooth; factor configurable; atlas-build time acceptable.

### P1.6 — Glyph atlas + packer

- **What:** rasterize a requested charset at a target px size; pack into one
  grayscale texture with a skyline/shelf packer; emit per-glyph UV rect +
  metrics; 1px padding to avoid bleed.
- **Why now:** the atlas is what Phase 2/3 actually consume.
- **Decisions to make:**
  - Atlas charset policy: fixed ASCII/Latin set, or on-demand glyph insertion
    with atlas growth? (Recommend fixed set first; design for growth.)
  - Atlas data exchange format for Python/file (raw bytes + struct vs PNG +
    JSON sidecar).
  - One atlas per (font, size), or multiple sizes in one atlas?
  - Packer choice (skyline is the sweet spot of simple + tight enough).
- **DoD:** one atlas image + metrics dump; the SDL demo blits a string from the
  atlas instead of stroking outlines.

### P1.7 — CMake + install/export, real tests

- **What:** migrate the build to CMake; `add_library(tr_font)`, install/export
  so `find_package(tr_font)` works; relocate the engine under `/font` per the
  monorepo layout (`FULL_PLAN.md` D-A3); extend gtest beyond the placeholder.
- **Why now:** packaging + Python both need CMake; do it once the surface is
  stable.
- **Decisions to make:**
  - Do the `/font` directory move now or defer? (Recommend now, with CMake, to
    avoid a second churn.)
  - Keep the makefile during transition, or hard cut? (Recommend keep until
    CMake reaches parity, then delete.)
  - Library type: static, shared, or both?
- **DoD:** a scratch CMake project links `tr_font` via `find_package`; tests run
  via CTest.

### P1.8 — pybind11 bindings + wheel

- **What:** a `tr_font` Python module: load a font, shape a string into
  positioned glyphs, get an atlas (image buffer + metrics). Build the wheel via
  `scikit-build-core`.
- **Why now:** Phase 3 consumes this; lock the API early.
- **Decisions to make:**
  - Buffer exchange: numpy array vs raw `bytes` for the atlas image.
  - API granularity (per `FULL_PLAN.md` D-2B thinking): coarse calls only — no
    per-glyph FFI in hot paths.
  - Packaging name and module layout.
- **DoD:** `pip install .`, `import tr_font`, dump an atlas PNG from Python.

### P1.9 — SDF atlas design (build deferred)

- **What:** write `docs/sdf.md` — approach (distance transform from coverage, or
  MSDF for sharp corners), atlas format deltas, and the shader-side sampling
  Phase 2 will need. No code yet.
- **Why now:** Phase 3's zoomable world needs crisp text; capture the design
  while the rasterizer is fresh in your head.
- **Decisions to make:**
  - Single-channel SDF (simple, rounds corners) vs MSDF (sharp, more complex).
  - Generation: in-house distance transform vs lean on `msdfgen` as a tool.
- **DoD:** `docs/sdf.md` exists; tagged as the v0.2 feature.

---

## Cross-cutting decisions (recap from FULL_PLAN)

- 🚩 Engine core is backend-agnostic (no SDL/GL in public headers) — P1.3.
- 🚩 Build migrates to CMake; engine moves under `/font` — P1.7.
- 🚩 SDF is designed in P1.9, built when Phase 3 needs zoom.
- ⚠️ Scope boundaries: **no CFF/PostScript (OTTO) outlines**, **no hinting
  bytecode execution**. Detect CFF fonts and reject with a clear error.

---

## Milestone summary

| Milestone | Done when |
|---|---|
| **P1.0** | Cleanup merged; `v0.1.0` tagged |
| **P1.1** | `hhea`/`hmtx` drive real advance widths |
| **P1.2** | One documented font-unit→pixel transform; `unitsPerEm` honored |
| **P1.3** | Engine compiles with no SDL on the include path |
| **P1.4** | Glyphs fill correctly with proper holes (nonzero winding) |
| **P1.5** | Anti-aliased edges via supersampling |
| **P1.6** | Packed atlas + metrics; demo blits from atlas |
| **P1.7** | CMake package; `find_package` works; CTest runs |
| **P1.8** | `import tr_font` works from a wheel |
| **P1.9** | SDF design doc written |

---

## Open questions to resolve before the milestone that needs them

- **Atlas charset** — ASCII/Latin only, or box-drawing + symbols for the game's
  "ASCII art" look? (Needed by P1.6; also in `FULL_PLAN.md` §9.)
- **Kerning** — legacy `kern` first, GPOS later, or skip for Phase 1? (See
  `FUTURE.md`; not required for the engine MVP.)
- **OS/2 line metrics** — adopt now for better line height, or stay on `hhea`?
- **Dynamic font path** — keep hardcoded in the demo, or add a CLI arg /
  Python-only loading? (Engine takes a path/buffer regardless.)
- **Gamma-correct blending** — commit to linear-space coverage now or later?
