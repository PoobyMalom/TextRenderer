Phase one: 2D font engine in c++, want this to become a package used in the 2D rendering engine and integrated also into a python package for the game phase

Phase two: 2D rendering engine, I want to make a c++ 2D renderer using opengl as the base so I can render text and create other library functions for a python layer to use

Phase three: I want to make a python based ascii art inspired factorio style factory automation game that runs on top of my font engine and renderer. I want this to have network support so I can host a server on a dedicated server (this is a seperate project that doesnt have to be worried about right now)

Phase four: I want to make a small portable lcd display with buttons that just displays an active version of that game that is basically just a view only with movement controls to view the factory in progress

Phase five: This is the most tentative phase but I want to write the portable layer on a fpga board running my custom riscV processor

---
---

# Detailed Roadmap & Project Plan

> Authored 2026-06-21 as an expansion of the five-phase vision above. This is a
> living document — revise milestone sizing as you learn. Nothing below changes
> the vision; it adds sequencing, goals, resources, and hard-won pitfalls.

---

## 0. How to read this document

### Effort Units (EU)

There are no calendar dates here by design. Instead, work is sized in **Effort
Units**:

> **1 EU ≈ one focused weekend of deep work (~8–12 hours), uninterrupted.**

EU is a *planning* estimate, not a promise. A milestone marked "3 EU" means
"about three good weekends of real progress if nothing surprises you." Hobby
time is bursty; the value of EU is **relative sizing and ordering**, not
prediction. When an estimate has a research-heavy unknown, it's marked with a
`?` (e.g. `4 EU?`) — expect it to be the one that blows up.

### Conventions used below

- **DoD** = Definition of Done. A milestone isn't finished until its DoD holds.
- **Blocks / Blocked-by** = hard dependencies between milestones or phases.
- **Forward dependency** = a decision you must make *now* because a much later
  phase depends on it (e.g. GLES-compatibility in Phase 2 because of Phase 4).
- 🚩 = a non-obvious architectural decision worth revisiting deliberately.
- ⚠️ = a known trap / common source of pain.

### The one principle that ties all five phases together

> **Each phase is a consumer of the phase below it, through a narrow, stable,
> rendering-backend-agnostic interface.**

The font engine knows nothing about OpenGL or SDL. The renderer knows nothing
about the game. The game knows nothing about the LCD or FPGA. If you keep these
seams clean, Phases 4 and 5 become "swap the bottom of the stack" instead of
"rewrite everything." If you let SDL/GL/game concepts leak downward, every later
phase pays interest. **Guard the seams.**

---

## 1. The North Star & guiding architecture

### 1.1 Target stack (top to bottom)

```
  ┌─────────────────────────────────────────────────────────┐
  │ Phase 5 (tentative): FPGA + custom RISC-V                 │
  │   runs a stripped view-only client; glyph ROM from engine │
  └─────────────────────────────────────────────────────────┘
  ┌─────────────────────────────────────────────────────────┐
  │ Phase 4: Portable LCD viewer (Raspberry Pi class)         │
  │   spectator client; OpenGL ES; buttons = camera           │
  └─────────────────────────────────────────────────────────┘
  ┌─────────────────────────────────────────────────────────┐
  │ Phase 3: Python factory game (client + authoritative srv) │
  │   sim in Python; networking; grid + freeform render styles│
  └─────────────────────────────────────────────────────────┘
  ┌─────────────────────────────────────────────────────────┐
  │ Phase 2: C++ OpenGL 2D renderer (libtr_render)            │
  │   quad-batch + text-grid + camera; pybind11 bindings      │
  └─────────────────────────────────────────────────────────┘
  ┌─────────────────────────────────────────────────────────┐
  │ Phase 1: C++ font engine (libtr_font)  ◀── YOU ARE HERE   │
  │   parse TTF → metrics + filled raster + atlas (no GL/SDL)  │
  └─────────────────────────────────────────────────────────┘
```

Library names `libtr_font` / `libtr_render` are placeholders ("tr" = TextRenderer);
rename once, early, then never again.

### 1.2 Cross-cutting architectural decisions (decide once, here)

🚩 **D-A1 — The engine core is backend-agnostic.**
`libtr_font` must not `#include` SDL or OpenGL. Its public output is plain data:
grayscale coverage bitmaps (`std::vector<uint8_t>`), glyph metrics, and atlas
layout metadata. The current `Renderer.{h,cpp}` (SDL `drawSimpleGlyph`) is a
*demo/example consumer*, not part of the library. **Move it to `examples/`.**
This is the keystone decision; everything else leans on it.

🚩 **D-A2 — Migrate the build to CMake.** The makefile is fine for the current
single binary, but cross-platform builds, pybind11 modules, install/export
targets (`find_package(tr_font)`), and Python wheels all want CMake. Do this at
the Phase 1 → packaging boundary, not before (don't disrupt the in-flight
cleanup). Keep the makefile working until CMake reaches parity, then delete it.

🚩 **D-A3 — Monorepo with hard internal boundaries.** One git repo:
`/font`, `/render`, `/python`, `/game`, `/examples`, `/docs`, `/tests`. Each
C++ library has its own public `include/` directory and is independently
buildable/testable. Split into separate repos only if/when external consumers
appear. Rationale: a hobby project shouldn't pay multi-repo coordination tax,
but the *directory* boundaries enforce the same discipline.

🚩 **D-A4 — SDL2 remains the platform layer through Phase 2.** Use SDL2 for
window creation, GL context, and input; use raw OpenGL for all drawing. You
already know SDL2; adding GLFW buys little. (Re-evaluate only if SDL2's GL
context handling fights you on the Pi in Phase 4.)

🚩 **D-A5 — One canonical coordinate-system + font-units document.** Before
writing the rasterizer, write `docs/coordinates.md`: font units vs pixels,
`unitsPerEm` from `head`, em square, Y-up (font) vs Y-down (screen/atlas), and
the single transform that converts between them. Every later bug in glyph
placement traces back to this; make it canonical and link to it from code
comments.

### 1.3 Semantic versioning of the libraries

Treat `libtr_font` and `libtr_render` as real dependencies with semver. Tag
`v0.1.0` = "outline renderer baseline" (where Phase 1 stands today) before you
start the rasterizer work, so you always have a known-good fallback. The game
pins to a renderer version; you bump deliberately.

---

## 2. Phase 1 — Font Engine

**Library:** `libtr_font` · **Language:** C++17 (consider C++20 later) ·
**Output contract:** metrics + filled coverage bitmaps + atlas metadata, no
GPU/windowing types.

### 2.1 Where it stands today (honest assessment)

**Done and working:**
- TTF container parsing: `head`, `maxp`, `loca`, `cmap` (formats 0/4/12), `glyf`
  (simple + compound).
- Quadratic-Bézier *outline* rendering via SDL line drawing (stroke, not fill).
- Clean public-ish API: `TTFFile::parse`, `parseGlyph`, `parseGlyphs`.
- A large correctness + lint + memory-leak cleanup pass is essentially complete
  (see `refs/TODO.md` — bugs B1–B12, leaks M1–M2, design D1–D11, linting
  LT1–LT22 nearly all checked off).
- CI/CD with clang-tidy; gtest scaffold present (placeholder test only).

**Not done / scope gaps to close in this phase:**
- No `hmtx` / `hhea` → advance widths hardcoded to 600 font units (TODO P4/X6).
- No kerning.
- **No fill rasterization** — glyphs are stroked outlines, not filled shapes.
- **No anti-aliasing.**
- **No glyph atlas.**
- Engine core still coupled to SDL via `Renderer` (needs the D-A1 separation).
- API exposes parser internals (`Glyph`, table classes) rather than a small
  facade.

### 2.2 Definition of Done for Phase 1

A C++ library that, given a `.ttf` file and a target pixel size, produces:
1. Correct **metrics** per glyph (advance width, bearings) and per font
   (ascender/descender/line gap, `unitsPerEm`).
2. **Filled, anti-aliased** grayscale coverage bitmaps for requested glyphs.
3. A packed **texture atlas** (one grayscale image + per-glyph UV rect +
   metrics) for a requested character set at a requested size.
4. A small, documented **public API** with no SDL/GL in its headers.
5. **pybind11** Python bindings exposing load → shape string → get atlas/metrics.
6. Installable via CMake (`find_package`) and as a Python wheel.
7. A documented (but not yet built) **SDF atlas** design, since Phase 3 needs
   crisp text at arbitrary zoom.

### 2.3 Milestones

**P1.0 — Land the cleanup & cut a baseline. (0.5 EU)**
- Finish/merge the `memory-leak-fixs` branch; close out remaining TODO items
  (the few unchecked sub-items in P4/X6/BLD are mostly "blocked on hmtx").
- Tag `v0.1.0` baseline.
- *DoD:* `main` builds clean, clang-tidy passes, tagged release exists.

**P1.1 — Horizontal metrics: `hhea` + `hmtx`. (1 EU)**
- Parse `hhea` (numberOfHMetrics, ascender, descender, lineGap).
- Parse `hmtx` (per-glyph advanceWidth, leftSideBearing; note the trailing
  run of monospaced lsb-only entries).
- Wire real advance widths into the demo; delete the `NOMINAL_*` constants
  (closes TODO P4/X6).
- *DoD:* proportional fonts (not just JetBrains Mono) advance correctly;
  golden test on a known glyph's advance.

**P1.2 — Read `unitsPerEm` and formalize the coordinate transform. (0.5 EU)**
- Pull `unitsPerEm` from `head` (don't assume 1000/2048).
- Write `docs/coordinates.md` (D-A5). Replace ad-hoc scaling math with one
  documented `fontUnitsToPixels(size)` transform.
- *DoD:* doc exists; scaling code references it; two fonts with different
  `unitsPerEm` render at the same visual size.

**P1.3 — Decouple the engine core from SDL (D-A1). (1 EU)**
- Move `Renderer.{h,cpp}` and the SDL `main` into `examples/sdl_outline_demo/`.
- Ensure `libtr_font` headers compile with no SDL on the include path.
- Introduce a thin public facade header (e.g. `tr/font.h`) that does not expose
  raw table classes.
- *DoD:* a tiny test TU that includes only `tr/font.h` builds without SDL.

**P1.4 — Contour flattening + scanline fill rasterizer. (3 EU?)**
- Flatten quadratic Béziers to line segments at a pixel-tolerance (adaptive
  subdivision or fixed steps tuned to size).
- Implement a **nonzero-winding** scanline polygon fill into a grayscale buffer.
  ⚠️ TrueType uses nonzero winding, *not* even-odd — overlapping contours
  (e.g. the bar of an 'A', counters in 'O'/'B') depend on this.
- *DoD:* 'O', 'B', 'g', 'A', '@' fill with correct holes; visual compare to a
  reference rasterizer (FreeType render of the same glyph/size).

**P1.5 — Anti-aliasing. (2 EU)**
- Start with **supersampling**: rasterize at 3×–4× then box-downsample to
  coverage. Simplest correct AA.
- Document a path to analytic/coverage AA (signed-area accumulation) as a later
  optimization — don't build it yet.
- *DoD:* edges are smooth; downsample factor configurable; perf acceptable for
  atlas generation (it's offline-ish, not per-frame).

**P1.6 — Glyph atlas + packer. (2 EU)**
- Rasterize a requested character set at a target px size; pack rects into a
  single grayscale texture using a **skyline/shelf** packer (simple, good
  enough). Leave 1px padding to prevent bleed.
- Output: `Atlas { image bytes, width, height, map<codepoint, GlyphEntry{uvRect,
  size, bearing, advance}> }`.
- *DoD:* one atlas image + JSON/struct dump; the SDL demo can blit a string from
  the atlas instead of stroking outlines.

**P1.7 — CMake + install/export (D-A2). (1.5 EU)**
- `add_library(tr_font ...)`, public/private include dirs, `install(TARGETS ...)`
  + an exported `tr_fontConfig.cmake` so downstream `find_package(tr_font)` works.
- Keep gtest target; extend tests beyond the placeholder (a real `test_helpers`,
  `test_cmap`, `test_glyf`, `test_metrics`).
- *DoD:* a separate scratch CMake project links `tr_font` via `find_package`.

**P1.8 — pybind11 bindings. (2 EU)**
- Module `tr_font` exposing: `Font(path)`, `font.shape("text", size_px) ->
  [PositionedGlyph]`, `font.atlas(charset, size_px) -> Atlas` (atlas image as a
  `numpy`/`bytes` buffer + metrics).
- Build the wheel via `scikit-build-core` (CMake-backed) so the same CMake drives
  C++ and Python.
- *DoD:* `pip install .` then `import tr_font` and dump an atlas PNG from Python.

**P1.9 — SDF atlas design doc (build deferred to when Phase 3 needs zoom). (0.5 EU)**
- Document approach (e.g. brute-force or 8SSEDT distance transform from the
  coverage bitmap; or `msdfgen`-style multi-channel for sharp corners). Note the
  shader-side sampling Phase 2 will need.
- *DoD:* `docs/sdf.md` exists; no code yet. Tagged as the v0.2 feature.

**Phase 1 total: ~14 EU** (P1.4/P1.8 are the swingers).

### 2.4 Resources for Phase 1 (font parsing & rasterization)

**Specs (authoritative — keep open while coding):**
- Microsoft OpenType spec — `learn.microsoft.com/typography/opentype/spec`.
  Bookmark `glyf`, `hmtx`, `hhea`, `cmap`, `head`, `maxp`, `loca`.
- Apple TrueType Reference Manual —
  `developer.apple.com/fonts/TrueType-Reference-Manual`. Clearest on outline
  geometry and compound-glyph component flags.

**Reference implementations (read, don't depend on):**
- `stb_truetype.h` (Sean Barrett) — single-file; its rasterizer is the model for
  your fill + AA (P1.4–P1.5). The most directly relevant code to read.
- FreeType — the production reference; use purely as an **oracle** to diff your
  raster output against (P1.4 DoD).
- `fontdue` and `ab_glyph` (Rust) — small, modern, very readable rasterizers if
  you want a cleaner read than FreeType's C.
- `msdfgen` (Viktor Chlumský) — the reference for the SDF/MSDF atlas (P1.9).

**Articles & talks:**
- Sebastian Lague, *"Coding Adventure: Rendering Text"* (YouTube) — builds a
  TTF Bézier-outline renderer from scratch; closest single thing to your project.
- Raph Levien, *font-rs* / "inside the fastest font renderer" posts — for the
  analytic / signed-area AA upgrade after supersampling works.
- Chris Green (Valve, 2007), *"Improved Alpha-Tested Magnification for Vector
  Textures and Special Effects"* — the original SDF text paper (P1.9 / P2.9).
- Any "scanline polygon fill + nonzero winding" tutorial — to anchor P1.4.

**Book:**
- *Fonts & Encodings*, Yannis Haralambous — encyclopedic on font formats; a
  reference to dip into, not read cover-to-cover.

**Tooling:**
- `fonttools` / `ttx` — dump any font to XML to verify your parser against ground
  truth (you already have `.ttx` files in `src/fonts/`).
- pybind11 docs + `scikit-build-core` docs — for P1.8 bindings and the wheel.

### 2.5 Notes, tips, pitfalls

- ⚠️ **CFF/PostScript outlines are out of scope.** OTTO/OpenType-CFF fonts store
  outlines in a `CFF ` table, not `glyf`. Your engine handles TrueType outlines
  only. Detect and *reject CFF fonts with a clear error* rather than mis-parsing.
- ⚠️ **TrueType hinting (the `instructions`/bytecode) is out of scope.** You read
  the bytes but won't execute them. At small pixel sizes unhinted glyphs look
  slightly fuzzy — acceptable, and AA hides most of it. Document this.
- ⚠️ Compound-glyph **point-index** argument mode (vs xy-offset mode) is only
  partially handled (TODO X4). Most fonts use xy-offsets; if a glyph renders
  garbled, suspect point-index anchoring.
- Keep rasterization **offline-friendly**: you build atlases occasionally, not
  per frame. Don't over-optimize the rasterizer before it's correct.
- Test corpus: keep `JetBrainsMono` (monospace, `cmap` format 12), plus at least
  one proportional font and one older font that uses `cmap` format 4, in
  `src/fonts/` (you already have several). Add one font with heavy compound
  glyphs (accented Latin) and verify.

---

## 3. Phase 2 — OpenGL 2D Renderer

**Library:** `libtr_render` · **Consumes:** `libtr_font` atlases + metrics ·
**Exposes:** a C++ API and a pybind11 module for Phase 3.

### 3.1 Goals

A reusable 2D renderer whose hot path is "draw a grid of glyph cells" (the game
model you chose), with a freeform textured-quad layer underneath for the
freeform render style and for UI. Text is just textured quads sampled from the
font atlas.

### 3.2 Key decisions

🚩 **D-2A — Target OpenGL 3.3 Core.** Widely supported, has instancing and VAOs.
**Forward dependency:** Phase 4 (Pi) uses **OpenGL ES**. So restrict yourself to
features with a clean GLES 3.0 analogue (instanced arrays exist in GLES 3.0;
avoid desktop-only niceties). Keep a `docs/gl_portability.md` listing anything
desktop-specific you use, so the Pi port is a checklist, not an archaeology dig.

🚩 **D-2B — Data-oriented scene submission.** Python (Phase 3) builds a *scene
description* (arrays of cells / quads); C++ consumes it and submits to the GPU.
Don't expose per-draw-call C++ methods to Python (the FFI overhead per cell would
murder performance). One call hands over the whole visible grid.

🚩 **D-2C — Single atlas texture, instanced rendering.** The text grid renders as
one instanced draw call: per-instance = {cell x, cell y, glyph index → uv,
fg color, bg color}. This is the performance keystone for the game.

### 3.3 Milestones

**P2.1 — Platform + context + clear. (0.5 EU)** SDL2 window, GL 3.3 core context,
glClear loop, frame timing. *DoD:* a colored window at stable FPS.

**P2.2 — GL resource abstractions. (1.5 EU)** Thin RAII wrappers: Shader/Program,
VertexBuffer, VertexArray, Texture. Error-check + debug output. *DoD:* a single
hardcoded triangle/quad renders through the abstractions.

**P2.3 — Textured quad batch. (2 EU)** Dynamic batch of textured, colored quads;
one draw call per texture; orthographic projection. *DoD:* draw N sprites from a
test texture at stable FPS with N in the thousands.

**P2.4 — Consume the font atlas; render a string. (1.5 EU)** Upload a
`libtr_font` atlas as a GL texture; turn `shape("text")` output into quads; blend
correctly. ⚠️ Use **premultiplied alpha** for text to avoid dark fringes. *DoD:*
crisp text string on screen, matching the offline atlas.

**P2.5 — TextGrid (instanced) — the grid render style. (2.5 EU)** An `MxN` grid of
cells, each cell {glyphIndex, fg, bg}, rendered in **one instanced draw call**.
Dirty-cell or full-buffer upload per frame. *DoD:* a 200×80 grid updates at 60
FPS with per-cell color.

**P2.6 — 2D camera: pan + smooth zoom. (1 EU)** Orthographic camera with
world→screen transform; pan and continuous zoom. ⚠️ Continuous zoom is exactly
why SDF (P1.9) eventually matters — note where text starts to look bad and feed
that back to Phase 1 prioritization. *DoD:* smooth pan/zoom over a large grid.

**P2.7 — Freeform sprite/quad API — the freeform render style. (1.5 EU)** Free
world-space placement of textured quads + text labels; z-order/layering;
alpha blending modes. *DoD:* sprites and text coexist with correct layering;
a flag swaps grid-style vs freeform-style rendering of the same scene.

**P2.8 — pybind11 renderer bindings (D-2B). (2 EU)** Expose: `create_window`,
`load_atlas`, `set_camera(x,y,zoom)`, `submit_grid(buffer)`,
`submit_sprites(buffer)`, `present`, `poll_input`. Bulk buffers as numpy arrays.
*DoD:* a Python script opens a window and animates a grid with zero C++ edits.

**P2.9 — SDF integration (pull in P1.9 when zoom quality demands it). (2 EU?)**
Build the SDF atlas in `libtr_font`; add the SDF sampling shader; switch text to
SDF for crisp arbitrary-zoom. *DoD:* text stays sharp across the full zoom range.

**Phase 2 total: ~16 EU.**

### 3.4 Resources for Phase 2 (OpenGL 2D renderer)

**Tutorials (modern core-profile GL):**
- LearnOpenGL.com (Joey de Vries) — the canonical modern-GL course; the
  *Instancing*, *Text Rendering*, and *Blending* chapters map straight onto
  P2.5/P2.4/P2.7. Also exists as a printable book.
- *The Cherno* OpenGL series (YouTube) — strong on building RAII abstractions
  and a quad batch renderer (P2.2–P2.3).
- open.gl and ogldev (Etay Meiri) — alternate explanations when one doesn't click.

**Reference projects (architecture to study):**
- `sokol_gfx.h` (Andre Weissflog) — minimal, beautifully designed GL/GLES
  abstraction; great model for keeping `libtr_render` small and portable.
- `raylib` — simple, readable 2D/3D API; excellent reference for a *friendly*
  public surface (relevant to your pybind11 API, D-2B).
- `bgfx` — heavier cross-platform render abstraction; read for how it isolates
  backends (useful thinking for the GLES forward-dep).

**Books:**
- *OpenGL Programming Guide* ("Red Book") — reference.
- *Real-Time Rendering* (Akenine-Möller, Haines, Hoffman) — deep reference; the
  blending/AA/text sections.
- *Game Engine Architecture* (Jason Gregory) — the renderer/resource-management
  chapters for structuring `libtr_render`.

**Articles & specs:**
- Chris Green's Valve SDF paper (see §2.4) — for the P2.9 SDF text shader.
- Khronos OpenGL ES 3.0 spec + any "porting desktop GL to GLES" write-up — feed
  these into `docs/gl_portability.md` for the Phase 4 forward-dependency (D-2C).

### 3.5 Pitfalls

- ⚠️ Don't cross the FFI boundary per cell/sprite. Batch everything; one Python
  call per layer per frame.
- ⚠️ Texture filtering: bitmap atlas wants `NEAREST` for a crisp "ASCII" look at
  1× and `LINEAR` only when scaling; SDF wants `LINEAR` always. Make it explicit.
- ⚠️ Avoid GL state thrash; sort by texture/shader, minimize binds.
- Keep the public renderer API *small and stable* — Phase 3 and Phase 4 both
  depend on it. Adding is cheap; changing signatures later is expensive.

---

## 4. Phase 3 — Python Factory Game

**Language:** Python (sim + glue) on top of `tr_font` + `tr_render`. ASCII-art
inspired, Factorio-style, networked.

### 4.1 Goals

A factory automation game: a grid world of tiles and entities (belts, inserters,
machines, resources) that you build to automate production, rendered as a grid of
glyph cells (with an optional freeform style), running against an authoritative
server so it can be hosted on a dedicated box.

### 4.2 Key decisions

🚩 **D-3A — Authoritative server, fixed-tick sim, decoupled render.** The
simulation advances at a fixed update rate (start 20 UPS); rendering runs at
display FPS and interpolates/snapshots. The server owns truth; clients send
*commands* (place/remove) and receive *state*. This avoids lockstep-determinism
pain (float nondeterminism across machines is a nightmare you don't need).

🚩 **D-3B — Build single-player first, but behind the network seam from day one.**
Phase 3 starts with an *in-process* server (same process, function calls). The
client only ever talks to it through the command/state interface. "Going
networked" then becomes "put a socket where the function call was," not a
rewrite.

🚩 **D-3C — Sim and rendering are fully separated.** The sim never imports the
renderer. The client reads sim state and builds a `tr_render` grid buffer. This
is what lets Phase 4 reuse the *exact same* client-render code as a spectator.

⚠️ **D-3D — Python sim performance is the project's real risk.** Thousands of
belts/inserters ticking in pure Python will eventually choke. Mitigations, in
order: (1) keep hot loops flat/array-based; (2) profile early; (3) be ready to
push the belt/inserter tick into a C++ module (you already have the C++/pybind11
toolchain — this is a *planned escape hatch*, designed for, not bolted on). Keep
the sim's hot data in structure-of-arrays form so a C++ port is mechanical.

### 4.3 Milestones

**P3.1 — Python scaffold + import both modules. (0.5 EU)** Project layout, deps,
`import tr_font, tr_render`; open a window from Python. *DoD:* window + a static
"hello" grid driven from Python.

**P3.2 — Render a static world. (1 EU)** Tile map data → grid cell buffer →
`tr_render.submit_grid`. *DoD:* a hand-authored map renders as glyph cells with
colors.

**P3.3 — Input, camera, cursor. (1 EU)** Pan/zoom via input from the renderer;
a tile cursor; screen↔world mapping. *DoD:* pan/zoom + cursor highlight on a tile.

**P3.4 — Core sim: map + place/remove entity. (1.5 EU)** Tile/entity model;
command handling; in-process server (D-3B). *DoD:* place and remove entities;
state survives a tick loop.

**P3.5 — Belts + item transport. (3 EU?)** The heart of the genre and the hardest
sub-system. Model belts as transport-line *segments* (not per-item objects) for
performance. Items flow, merge at junctions, back up when blocked. *DoD:* items
ride belts, turn corners, and congest realistically; perf holds at hundreds of
belt tiles.

**P3.6 — Inserters, machines, recipes, resources. (2.5 EU)** Inserters move items
between belts/machines; machines consume inputs → produce outputs on a recipe
timer; resource patches deplete. *DoD:* a closed loop (mine → belt → machine →
belt → output) runs unattended.

**P3.7 — Save / load. (1 EU)** Serialize world + entity state; load it back
deterministically. *DoD:* round-trip a built factory.

**P3.8 — Extract the server module (still in-process). (1 EU)** Formalize the
command/state protocol as data structures; server is a separate module the client
drives through that interface only. *DoD:* client has zero direct access to sim
internals.

**P3.9 — Network transport. (2.5 EU?)** Put the protocol over TCP (and/or
WebSocket). Client sends commands; server broadcasts state snapshots/deltas at
the tick rate. Handle join (full-state sync) then deltas. asyncio on the server.
*DoD:* two clients on the LAN see the same factory; one builds, the other sees it.

**P3.10 — Dedicated server deployment. (1 EU)** Headless server process,
config, restart-on-crash, basic auth/allowlist. *DoD:* server runs on the
dedicated box; clients connect over the network.

**P3.11 — Freeform render style option. (1.5 EU)** Player-selectable: grid style
(default) vs freeform (entities as freely-placed sprites/quads + text). Both read
the same sim state. *DoD:* toggle styles at runtime; both render the same factory.

**Phase 3 total: ~17 EU** (belts + networking are the swingers).

### 4.4 Resources for Phase 3 (factory game + networking)

**Articles & dev blogs (the canonical reading for the hard parts):**
- *Factorio Friday Facts* (FFF) — search the archive for belt **transport-line**
  optimization, the update/render decoupling, and entity-update threading posts.
  This is the single best source on the belt model (R1) and sim performance (R2).
- *Gaffer On Games* (Glenn Fiedler) — *"Fix Your Timestep"* (the fixed-tick loop,
  D-3A), plus *"Snapshot Interpolation"* and *"State Synchronization"* for the
  authoritative-server netcode (P3.9).
- Valve, *"Source Multiplayer Networking"* — clear intro to authoritative server,
  client prediction, and interpolation.
- Fabien Sanglard's *Quake 3 networking* write-up — a compact, classic model.

**Books:**
- *Game Programming Patterns* (Robert Nystrom) — free at gameprogrammingpatterns.com;
  the *Game Loop*, *Update Method*, *Component*, and *Spatial Partition* chapters
  are directly applicable.
- *Multiplayer Game Programming* (Glazer & Madhav) — end-to-end netcode.
- *Game Engine Architecture* (Gregory) — subsystem structure (shared with §3.4).

**Reference projects (open source to read):**
- **Mindustry** (Java) — open-source factory/automation game; readable systems
  for belts, production, and UI.
- **OpenTTD** — long-lived transport simulation; reference for tile/vehicle sim.
- **Cataclysm: DDA** and **libtcod** — large ASCII/grid worlds with many ticking
  entities; conventions for grid rendering and the "ASCII art" aesthetic.

**Python/tooling:**
- `asyncio` docs (server event loop); `struct` or `msgpack` for the binary wire
  format — but start with JSON for debuggability, then optimize (R7: version it).
- `cProfile` / `py-spy` — profile the sim early (R2) to know when to invoke the
  planned C++ offload (D-3D).

### 4.5 Pitfalls

- ⚠️ Don't model each item on a belt as its own object — model belt *lanes* with
  positions; otherwise the sim dies at scale. This is the single most important
  Phase 3 implementation choice.
- ⚠️ Decide command vs state ownership before networking, not after.
- ⚠️ Snapshot size: send deltas, not full world, every tick once it's big.
- Keep the protocol versioned — Phase 4's spectator client depends on it.

---

## 5. Phase 4 — Portable LCD Viewer

**Goal:** a small portable device with an LCD + buttons that connects to the
dedicated server as a **view-only spectator** and lets you pan around the factory.

### 5.1 Key decisions

🚩 **D-4A — Raspberry Pi class hardware first (not a microcontroller).** A Pi
Zero 2 W / Pi 4 runs your existing Python + OpenGL-ES stack with minimal porting.
A microcontroller (ESP32/RP2040) would mean re-implementing the renderer from
scratch on bare metal — that's a *bridge toward Phase 5*, not Phase 4. Do the Pi
first; treat MCU/bare-metal as an optional later spin that shares groundwork with
Phase 5.

🚩 **D-4B — Reuse the Phase 3 client's render path verbatim.** Because sim and
render are separated (D-3C) and the spectator sends no commands, the viewer is
"the Phase 3 client minus input commands, plus button-driven camera." If you find
yourself rewriting rendering here, a seam leaked upstream — fix it there.

⚠️ **D-4C — This is where GLES portability (D-2A) gets cashed in.** Walk the
`docs/gl_portability.md` checklist; fix any desktop-only GL usage. The smaller the
list you kept in Phase 2, the cheaper this is.

### 5.2 Milestones

**P4.1 — Hardware bring-up. (1 EU)** Pick Pi + LCD (SPI or small HDMI) + buttons;
get the display lit and GPIO buttons read in Python. *DoD:* display shows
something; button presses print.

**P4.2 — Port renderer to GLES on device. (2 EU?)** Build `tr_render` (and
`tr_font`) on the Pi against GLES; resolve portability deltas. *DoD:* a grid
renders on the LCD at usable FPS.

**P4.3 — Spectator network client. (1.5 EU)** Connect to the dedicated server,
receive state, render — no command sending. *DoD:* the LCD shows the live factory
from the server.

**P4.4 — Button-driven camera. (1 EU)** Buttons pan; maybe zoom; debounced.
*DoD:* navigate the whole factory with buttons only.

**P4.5 — Portability / enclosure. (1.5 EU)** Battery, case, autostart on boot.
*DoD:* untethered handheld that boots straight into the live view.

**Phase 4 total: ~7 EU** (GLES port is the swinger).

### 5.3 Resources for Phase 4 (portable LCD viewer)

**Hardware & OS:**
- Official Raspberry Pi documentation — GPIO, display, and boot/autostart.
- `gpiozero` (friendly) / `RPi.GPIO` (low-level) for buttons (P4.4).
- Adafruit and Pimoroni learn guides — SPI/I²C LCDs and button HATs; the most
  practical, copy-pasteable bring-up material (P4.1).

**Graphics on device:**
- Mesa / OpenGL ES on Raspberry Pi notes; **KMS/DRM** rendering to drive the
  panel without a full X/Wayland desktop (lighter, faster boot).
- *OpenGL ES 3.0 Programming Guide* (Munshi, Ginsburg, Shreiner) — the GLES
  reference for the P4.2 port; pair with `docs/gl_portability.md` (D-2C/D-4C).

**Note:**
- SDF text (P2.9) pays off most here — a low-resolution panel with crisp glyphs
  at any cell size. Prioritize finishing SDF before this phase if you can.

---

## 6. Phase 5 — FPGA + Custom RISC-V (tentative / stretch)

**Goal (as stated):** write the portable layer on an FPGA running your own
RISC-V processor. This is the most ambitious phase and spans HDL, CPU design, a
toolchain, and a display controller. Keep it deliberately staged so each step is
independently rewarding.

### 6.1 Key decisions (to de-risk an enormous scope)

🚩 **D-5A — The FPGA runs the *view-only* layer, not the game.** Don't try to run
Python or the full renderer. The device receives game state over a simple link
and blits a **text grid** into a framebuffer. This reframes Phase 1's atlas as a
**glyph ROM** — precompute a bitmap font and store it in block RAM; the hardware
blits cells. Your font engine's output is the natural source for that ROM.

🚩 **D-5B — Start with an existing open RISC-V soft core; "custom" is the final
stretch.** Bring up PicoRV32 / VexRiscv / Ibex first and run C on it. Designing
your *own* RISC-V core is a worthy capstone, but prove the whole system on a known
core before swapping in yours — otherwise you can't tell whether a bug is in the
CPU or the system.

🚩 **D-5C — Pick a board with an open toolchain if you can.** Lattice ECP5 with
yosys/nextpnr/prjtrellis (fully open) lowers friction; Xilinx Artix-7 + Vivado is
more common in tutorials. Choose before writing any HDL.

### 6.2 Coarse milestones (sizing is intentionally rough — this is research-heavy)

**P5.1 — Board + toolchain + UART hello. (2 EU?)** Blink, UART out, synthesis
flow working. *DoD:* you can build a bitstream and see output.

**P5.2 — Soft RISC-V core running C. (3 EU?)** Bring up an existing core, build
with `riscv-gcc`, run a program over UART. *DoD:* `printf` from a C program on the
soft core.

**P5.3 — Display controller + framebuffer. (3 EU?)** HDL drives the LCD/VGA/HDMI
from a framebuffer in RAM. *DoD:* a static image from the framebuffer on screen.

**P5.4 — Glyph ROM + text-grid blitter. (3 EU?)** Load the bitmap font (from
`tr_font`) into ROM; CPU/HW blits a grid of cells into the framebuffer. *DoD:* a
text grid rendered from a glyph ROM.

**P5.5 — Minimal state-receiving client. (3 EU?)** Receive game state over
serial/SPI/Ethernet; map it to the grid; buttons pan. *DoD:* the FPGA device shows
the live (or recorded) factory and you can pan it.

**P5.6 — (Capstone stretch) Replace the soft core with your own RISC-V. (huge?)**
Implement RV32I, validate against the test suite, drop it in for the existing
core. *DoD:* the system runs on your CPU.

**Phase 5 total: large and uncertain — treat as a multi-stage research project,
not a sprint.**

### 6.3 Resources for Phase 5 (FPGA + custom RISC-V)

**Books:**
- *Digital Design and Computer Architecture, RISC-V Edition* (Harris & Harris) —
  the best single book for going HDL → CPU; builds a RISC-V core by the end.
- *The Elements of Computing Systems* / **nand2tetris** (Nisan & Schocken) — the
  mental model for building a computer up from gates.
- *Computer Organization and Design, RISC-V Edition* (Patterson & Hennessy) —
  the reference for the ISA and microarchitecture (the capstone D-5B/P5.6).

**Hands-on, project-based (most directly on point):**
- **Bruno Levy's "Learn FPGA" / `femtorv32`** — "from blinker to RISC-V" on cheap
  FPGAs with the open toolchain; tracks your exact D-5B path (existing-core-first,
  then your own) and includes a VGA/text framebuffer. Start here.
- geohot/comma *"From the Transistor to the Web Browser"* syllabus — a staged,
  build-it-yourself path through the whole stack.
- fpga4fun.com — classic VGA/HDMI text-mode and "pong"-style tutorials feeding
  the display controller + glyph blitter (P5.3–P5.4).

**Specs & compliance:**
- RISC-V Unprivileged ISA spec (riscv.org) — **RV32I** is your target subset.
- `riscv-arch-test` / `riscv-tests` — validate your custom core (P5.6 DoD).

**Soft cores (bring one up before designing your own — D-5B):**
- **PicoRV32** (small, simple, great first core), **VexRiscv** (SpinalHDL,
  configurable), **Ibex** (lowRISC, production-quality), **SERV** (the smallest
  RISC-V — fun reference).

**Toolchains:**
- Open flow: **yosys + nextpnr + prjtrellis** (Lattice ECP5) or **Project
  IceStorm** (iCE40) — matches D-5C.
- Vendor flow: Xilinx **Vivado** (Artix-7) — more tutorials, closed.
- `riscv-gnu-toolchain` — the C compiler for code running on the core (P5.2).

---

## 7. Cross-cutting concerns

### 7.1 Build & packaging
- **CMake** as the single source of truth (D-A2); `scikit-build-core` bridges to
  Python wheels so one config builds both.
- Per-library `install`/`export` so consumers use `find_package`.
- Keep the makefile alive only until CMake hits parity, then remove it.

### 7.2 Testing
- **C++:** gtest. Real units for parsing (`cmap`, `glyf`, `metrics`), plus
  **golden-image** tests for the rasterizer (render glyph → compare to a stored
  reference PNG within a tolerance; FreeType output is a good oracle to generate
  references once).
- **Python:** pytest for the game sim (belts, recipes, save/load round-trips).
- **Determinism tests** for the sim where it matters (same commands → same state).

### 7.3 CI/CD
- You already have GitLab CI + clang-tidy. Extend to: build every library, run
  C++ and Python tests, build the wheel, and run the golden-image tests headless
  (offscreen GL or software rasterizer for the engine's CPU output).

### 7.4 Documentation
- `docs/coordinates.md` (D-A5), `docs/sdf.md`, `docs/gl_portability.md`,
  `docs/protocol.md` (game command/state format).
- Doxygen on the public C++ headers; a README per library with a 10-line
  "hello" example.

### 7.5 Repo layout (target)
```
/font      libtr_font  (engine core, no GL/SDL)
/render    libtr_render (OpenGL renderer + bindings)
/python    pybind11 modules + wheel packaging
/game      Python factory game (client + server)
/device    Phase 4 viewer code
/fpga      Phase 5 HDL + soft-core + C client
/examples  SDL outline demo, atlas dumper, renderer demos
/docs      canonical references
/tests     C++ + Python tests, golden images
```

---

## 8. Risk register

| # | Risk | Phase | Mitigation |
|---|------|-------|------------|
| R1 | Belt simulation complexity & scale | 3 | Segment/lane model, not per-item objects; profile early |
| R2 | Python sim too slow at scale | 3 | SoA hot data; planned C++ offload via existing pybind11 path (D-3D) |
| R3 | GLES portability surprises on Pi | 2→4 | Keep `docs/gl_portability.md` from day one (D-2C/D-4C) |
| R4 | FPGA scope explosion | 5 | View-only target (D-5A); existing soft core first (D-5B) |
| R5 | Font edge cases — CFF, hinting, exotic cmap | 1 | Explicit scope boundaries; reject CFF with clear error |
| R6 | Backend leakage breaking later reuse | all | Guard the seams (§1); the no-SDL test TU (P1.3) catches regressions |
| R7 | Network protocol churn breaking Phase 4 | 3→4 | Version the protocol (D-3D / §7.4) |
| R8 | Over-engineering the rasterizer before correctness | 1 | Correct fill (P1.4) before any AA/perf work |

---

## 9. Open questions to revisit (not blocking)

These don't block starting Phase 1, but you'll want answers before the phase that
needs them:

1. **Color/theming model for the game** — fixed 16-color "terminal" palette, or
   full RGB per cell? (Affects the grid cell format in P2.5.)
2. **Multiplayer scope** — co-op only, or contested/PvP? (Affects server auth in
   P3.10.)
3. **Target font set for the game** — just ASCII/Latin, or box-drawing & symbols
   for richer "ASCII art"? (Affects the atlas charset in P1.6.)
4. **Dedicated server OS/host** — known? (Affects P3.10 packaging.)
5. **Phase 5 board** — open toolchain (ECP5) vs Xilinx? (Decide before any HDL.)

---

## 10. Recommended immediate next steps

1. Close out the remaining `refs/TODO.md` items and merge `memory-leak-fixs`.
2. Tag `v0.1.0` (outline-renderer baseline).
3. Start **P1.1 (hmtx/hhea)** and **P1.2 (unitsPerEm + coordinates doc)** — small,
   high-leverage, and they unblock correct metrics everywhere downstream.
4. Then **P1.3 (decouple from SDL)** — do this *before* the rasterizer so the new
   raster code is born backend-agnostic.

> Everything after that follows the milestone order above. Re-size EU estimates as
> you learn; the *ordering and the seams* matter more than the numbers.
