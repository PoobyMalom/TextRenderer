# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Commands

Build system is a single `makefile` at the repo root (g++, C++17, SDL3 + gtest via `pkg-config`). `CXXFLAGS` already includes `-O2`, so there is no meaningfully slower default build to avoid.

```bash
make                      # builds `main` (the production glyph renderer entry point)
make run                  # build + run main

make demo && ./demo       # phase-stepping interactive demo (src/DemoPhases.cpp)
make run-demo

make triangulation && ./triangulation src/fonts/<font>.ttf   # CDT dev/debug harness, see below
make run-triangulation     # same, defaults to src/fonts/JetBrainsMono-Regular.ttf

make benchmark_cdt && ./benchmark_cdt   # times triangulateConstrained() across every font in src/fonts
make run-benchmark

make test_runner && ./test_runner                       # run all tests (gtest)
./test_runner --gtest_filter=SuiteName.TestName          # run a single test
./test_runner --gtest_filter=SuiteName.*                 # run one suite

make clean                 # removes build/ and all binaries
make release                # adds -DNDEBUG on top (CXXFLAGS already has -O2)
```

Each of `main`, `demo`, `triangulation`, `test_runner`, and `benchmark_cdt` is its own binary with its own entry point (`main.cpp`, `demo.cpp`, `triangulation.cpp`, `tests/test_helpers.cpp`, `benchmark_cdt.cpp`) but shares the core parsing/rendering object files (`$(OBJS)` in the makefile). Only `triangulation` and `benchmark_cdt` additionally link the `src/cdt/` constrained-Delaunay library (`$(CDT_OBJS)`) — `main`/`demo`/`test_runner` do not depend on it.

`make` does not track flag changes, only file mtimes — if you change `CXXFLAGS` or switch compilers, run `make clean` first or stale objects will silently relink.

## Architecture

### TTF parsing pipeline
`TTFFile::parse()` is the entry point and must run its sub-parses in this order (each depends on data the previous one extracted): `TTFHeader::parse()` + `parseTables()` build a `TableMap` (table tag → offset/length, checksum-verified against the file) → `HeadTable` (`unitsPerEm`, `indexToLocFormat`) → `MaxpTable` (`numGlyphs`) → `Metrics` (`hhea` + `hmtx`; defensively clamps `numOfLongHorMetrics` against both `numGlyphs` and the actual buffer length, since real-world fonts have been observed to declare it larger than `numGlyphs`, which is a spec violation) → `LocaTable` (per-glyph byte offsets into `glyf`) → `CmapTable` (codepoint → glyph index).

Glyph lookup (`TTFFile::parseGlyph(data, unicode)`): cmap maps codepoint → glyph index → `loca` gives the byte offset → `Glyph::parseGlyph` parses it. A glyph's outline is **not** pre-split into points-with-metadata; it's three parallel flat arrays (`xCoordinates`, `yCoordinates`, `flags`) plus `endPtsOfContours` marking contour boundaries, matching TrueType's on-wire encoding (on-curve bit packed into `flags`). Compound glyphs recurse through component glyphs via the same `loca`/`glyf` offsets. `addPointsBetween()` (gated by the `insertBezierMidpoints` parameter on `parseGlyph`) can synthesize implied on-curve midpoints between two consecutive off-curve points.

### Coordinate system
Font data is Y-up in abstract "font units" (`unitsPerEm`-scaled); screen/pixel space is Y-down. The conversion is `FontTransform` in `Metrics.h` (`toPixels`, `toScreenY`) — the Y-flip must only ever happen there, never inline elsewhere. Full writeup with a worked example: `docs/coordinates.md`. Read it before touching any coordinate-handling code.

### Rendering (`Renderer.cpp`)
Several draw functions share the same traversal pattern: walk each contour's points, and for every on-curve point check whether the *next* point is on-curve (straight edge) or off-curve (on, off, on = one quadratic Bézier segment, drawn via `DrawBezier`/`DrawBezierDashed` in `Helpers.cpp`). `drawSimpleGlyph` is the real outline; `drawSimpleGlyphDashed` is a debug view that also colors each segment by the contour's winding direction; `drawSimpleGlyphLines` draws straight chords between points for comparison; `drawTriangulatedGlyph` feeds the glyph through the CDT library (below) and fills it with triangles instead of outlining it.

### Constrained Delaunay triangulation (`include/cdt/`, `src/cdt/`)
A self-contained CDT implementation (ported in from a separate prototype, `~/DelaunayTest`) used to fill a glyph's outline with triangles. Only `triangulation.cpp` and `benchmark_cdt.cpp` link against it. Entry point is `triangulateConstrained()` in `constrain.cpp`:

1. **Insertion** (`triangulate.cpp`): incremental Bowyer–Watson insertion of all contour points into a padded super-triangle.
2. **Constraint recovery** (`recoverConstraintEdge`): forces each contour edge into the mesh via diagonal flips. Uses an `EdgeAdjacency` edge→triangle map (`adjacency.h`) that's updated incrementally per flip, not rebuilt from scratch — rebuilding per-flip was the original design and was a major performance bottleneck (see `git log` around the CDT benchmark work for the before/after numbers).
3. **Legalization** (`legalizeNonConstraintEdges`): standard Delaunay edge-flip legalization, implemented as a flip worklist (not a repeated full-mesh rescan, same rebuild-cost lesson as step 2).
4. **Classification** (`classifyAndStripExterior`): even-odd point-in-polygon against the on-curve contour decides inside/outside (handles holes, and handles a contour that pinches inward and briefly touches itself — a dual-graph flood fill cannot distinguish that from a real boundary crossing, which is why this uses point-in-polygon instead). Also strips zero-area degenerate triangles (exactly-collinear points, which make `calculateCircumcircle` return an invalid radius that can never trigger a legalizing flip).

A second `triangulateConstrained()` overload takes extra vertices/constraints/expected-triangles, used to force each quadratic curve's "control triangle" (on-curve point, off-curve control point, on-curve point) to survive into the mesh intact, so it can eventually get its own per-pixel curve test instead of being approximated by a flat chord. **This is active, not-yet-correct work**: it works for simple cases but a full sweep across every glyph in `src/fonts` currently finds on the order of thousands of problems (missing control triangles from constraint-recovery failures under the added constraint load, plus triangle overlaps from off-curve control points that aren't deduplicated against existing points the way on-curve points are). Don't assume this path is verified before building on it further.

### Known gitignore gap
`.gitignore` ignores a top-level `fonts/*.ttf` that doesn't exist (fonts actually live in `src/fonts/`, which is not excluded — hence the `.ttf` files are tracked) and lists a `test` binary that doesn't match the actual `test_runner` target name. `test_runner` and `benchmark_cdt` binaries are currently untracked-but-not-ignored; `make clean` removes them either way.
