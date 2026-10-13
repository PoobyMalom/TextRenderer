# AGENTS.md

Guidance for AI coding agents working in this repository. (See also `CLAUDE.md`, which covers the same ground for Claude Code specifically — keep the two in sync if you update either.)

## What this project is

TextRenderer parses TrueType font files from scratch (no FreeType/stb_truetype) and renders glyph outlines, currently via SDL3. It's a learning/graphics-R&D project, not a shipping product — the debug scaffolding (hotkey visualization modes, a benchmark harness, dashed/colored debug draw modes) is intentional and should be treated as a normal part of the codebase, not dead code to clean up.

## Commands

C++17, built with a single root `makefile` (g++, SDL3 + gtest via `pkg-config`). `CXXFLAGS` already includes `-O2`.

```bash
make && make run                                 # main: production glyph renderer
make demo && ./demo                              # phase-stepping interactive demo
make triangulation && ./triangulation src/fonts/<font>.ttf   # CDT dev/debug harness (hotkeys 1-9)
make benchmark_cdt && ./benchmark_cdt            # times triangulateConstrained() across every font in src/fonts
make test_runner && ./test_runner                # all tests (gtest)
./test_runner --gtest_filter=SuiteName.TestName  # single test
make clean                                       # make does not track flag changes, only mtimes --
                                                  # run this after editing CXXFLAGS or it'll silently relink stale objects
```

Five binaries, five entry points (`main.cpp`, `demo.cpp`, `triangulation.cpp`, `tests/test_helpers.cpp`, `benchmark_cdt.cpp`), sharing the core parsing/rendering objects. Only `triangulation` and `benchmark_cdt` link the `src/cdt/` CDT library.

## Architecture

**Parsing** (`TTFFile::parse()`): header/table directory → `HeadTable` → `MaxpTable` → `Metrics` (hhea+hmtx, defensively clamped — real fonts have been seen declaring `numOfLongHorMetrics > numGlyphs`, which is spec-invalid) → `LocaTable` → `CmapTable`. Order matters; each stage consumes data the previous one extracted. A glyph's outline is three parallel flat arrays (`xCoordinates`/`yCoordinates`/`flags`, on-curve bit packed into `flags`) plus `endPtsOfContours`, matching the wire format directly — not pre-split into richer point objects.

**Coordinates**: font data is Y-up abstract units; screen space is Y-down. All conversion goes through `FontTransform` (`Metrics.h`); the Y-flip must never happen inline anywhere else. Full convention writeup with worked example: `docs/coordinates.md` — read before touching coordinate code.

**Rendering** (`Renderer.cpp`): shared traversal pattern across draw functions — walk each contour, and for each on-curve point check if the next point is on-curve (straight edge) or off-curve (quadratic Bézier, on-off-on). `drawTriangulatedGlyph` is the odd one out: it feeds the glyph into the CDT library and fills with triangles instead of stroking an outline.

**CDT library** (`include/cdt/`, `src/cdt/`, ported from a separate prototype `~/DelaunayTest`): `triangulateConstrained()` in `constrain.cpp` runs Bowyer–Watson insertion → constraint-edge recovery (diagonal flips) → Delaunay legalization (flip worklist) → even-odd point-in-polygon classification (handles holes and self-touching contours, which a dual-graph flood fill can't distinguish from a real boundary) + degenerate-triangle stripping. A second overload adds extra vertices/constraints to force each curve's "control triangle" (on-curve, off-curve control point, on-curve) to survive intact, toward eventually giving curves a real per-pixel test instead of a flat-chord approximation — **this is unfinished**: works on simple glyphs, but a full sweep across `src/fonts` currently surfaces on the order of thousands of problems (missing control triangles, triangle overlaps from undeduplicated control points). Verify before building further on it.

## Hard-won lessons specific to this codebase

- **SDL windows and rebuilds don't mix without a kill first.** The linker replaces the binary file on each build, but an already-running process keeps its *old* binary mapped in memory (check via `readlink -f /proc/<pid>/exe` vs the file's current inode if something looks stale). Always `pkill -x <binary>` before relaunching an interactive SDL target (`triangulation`, `main`, `demo`) after a rebuild, or you'll debug phantom "unfixed" behavior that's actually just stale code.
- **Verify geometry changes with small standalone diagnostics, not just visual inspection.** The most effective pattern this session: write a throwaway `.cpp` that links directly against the relevant `src/*.cpp` files (`g++ -Iinclude -Iinclude/cdt ... src/cdt/*.cpp`), dumps or asserts on the actual data (contour points, triangle lists, overlap/degeneracy checks), and sweep it across every font in `src/fonts` and a broad character set before trusting a fix. A fix that looks right on one glyph in one font has repeatedly turned out to be wrong at scale in this codebase.
- **Profile before optimizing.** The single largest speedup found in this codebase (~870x on the CDT pipeline) was `-O2` simply not being in the default build flags — found only by timing pipeline phases separately, not by guessing which function was slow. Guessing which `cdt/` phase was the bottleneck was wrong more than once; timing it directly was always right.
- **Duplicate points after rounding to integer pixels are a recurring CDT failure mode.** Two distinct font-unit coordinates can round to the same screen pixel at small font sizes. Feeding `triangulateConstrained` a duplicate point produces a triangle with two identical corners (zero area, renders as a stray spike). Always dedupe a contour's points before triangulating.
- **`EdgeAdjacency::build()` must `.clear()` before rebuilding**, or stale entries from the mesh's previous state accumulate on top of current ones and silently corrupt both the "is this edge interior" check and `neighbors()`.
- **Exactly-collinear points break the circumcircle math, not just "nearly degenerate" ones.** `calculateCircumcircle` returns an invalid circle (radius `-1`) for three exactly-collinear points, so `inCircumcircle` can never flag that triangle for a legalizing flip — it silently survives into the final mesh. Detect degenerate triangles by checking triangle area directly, not by relying on the circumcircle test to catch them.

## Known repo-hygiene gap

`.gitignore` ignores a top-level `fonts/*.ttf` that doesn't exist (the real path is `src/fonts/`, which isn't excluded, so the `.ttf` files are tracked) and lists a `test` binary that doesn't match the actual `test_runner` target. `test_runner` and `benchmark_cdt` are untracked but not ignored.
