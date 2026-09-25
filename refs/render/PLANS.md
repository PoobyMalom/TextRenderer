# Phase 2 — OpenGL 2D Renderer (Implementation Plan)

> Mid-level breakdown of `FULL_PLAN.md` §3. The renderer (`libtr_render`) is a
> C++ library on OpenGL that consumes the Phase 1 font engine's atlases and
> exposes a small C++ + pybind11 API for the Phase 3 game. No code here —
> structure and the design decisions you own.

---

## Goal

A reusable 2D renderer whose hot path is **"draw a grid of glyph cells"** (the
grid render style), with a **freeform textured-quad layer** beneath it for the
freeform style and UI. Text is just textured quads sampled from the font atlas.
The renderer must stay portable enough to run on **OpenGL ES** later (Phase 4 on
a Raspberry Pi).

**Depends on:** Phase 1 atlas + metrics output (P1.6), ideally the SDF atlas
(P1.9) before the zoom-quality milestone.

---

## Guiding decisions (from FULL_PLAN, restated)

- 🚩 **Target OpenGL 3.3 Core**, restricted to features with a clean **GLES 3.0**
  analogue. Keep a running `docs/gl_portability.md` of anything desktop-specific.
- 🚩 **Data-oriented submission:** Python builds a whole-scene buffer; C++
  consumes it in one call per layer. No per-cell/per-sprite FFI.
- 🚩 **Single atlas texture, instanced rendering** for the grid: one draw call
  for the whole visible grid.
- 🚩 **SDL2 stays the platform layer** (window/context/input); OpenGL draws.

---

## Architecture

```
[Platform layer]  SDL2 window + GL context + input
       │
[GL resource layer]  RAII wrappers: Shader/Program, Buffer, VertexArray, Texture
       │
[Renderer core]
   ├─ Quad batch      (freeform textured/colored quads, UI)
   ├─ TextGrid        (instanced MxN glyph cells — the hot path)
   └─ Camera (2D ortho, pan + smooth zoom)
       │
[Public API]  C++  ──▶  pybind11 module  ──▶  Phase 3 game
       │
[Asset input]  font atlas + metrics from libtr_font
```

---

## Milestones

### P2.1 — Platform + context + clear
- Bring up an SDL2 window with a GL 3.3 core context; a stable clear/present loop
  with frame timing.
- **Decisions:** context version/profile; vsync on/off; how input events surface
  to the API (poll vs callback).
- **DoD:** a colored window at stable FPS.

### P2.2 — GL resource abstractions
- Thin RAII wrappers around shaders/programs, buffers, vertex arrays, textures;
  debug-output/error checking.
- **Decisions:** how much to wrap (minimal vs a mini-framework); shader sourcing
  (files vs embedded); uniform handling.
- **DoD:** one hardcoded quad renders through the abstractions.

### P2.3 — Quad batch renderer
- Dynamic batch of textured, colored quads under an orthographic projection; one
  draw call per texture.
- **Decisions:** batch sizing/flush policy; vertex layout; coordinate origin
  (top-left vs bottom-left) — pick once, document.
- **DoD:** thousands of sprites from a test texture at stable FPS.

### P2.4 — Consume the font atlas; render a string
- Upload a `libtr_font` atlas as a GL texture; turn shaped-string output into
  quads; correct blending.
- **Decisions:** ⚠️ premultiplied alpha for text; texture filtering (`NEAREST`
  for crisp bitmap look vs `LINEAR` when scaling); how the atlas crosses the API
  (path, bytes, or a `tr_font` handle).
- **DoD:** a crisp string on screen matching the offline atlas.

### P2.5 — TextGrid (instanced) — the grid render style
- An MxN grid; each cell = {glyph index, fg color, bg color}; rendered in **one
  instanced draw call**.
- **Decisions:** per-cell data format (this is also the wire/Python buffer
  format — get it right once); full-buffer re-upload vs dirty-cell updates; how
  glyph index maps to atlas UV (direct vs indirection table); cell size /
  fixed-pitch assumption.
- **DoD:** a 200×80 grid with per-cell color updates at 60 FPS.

### P2.6 — 2D camera: pan + smooth zoom
- Orthographic camera; world↔screen transforms; pan and continuous zoom (zoom
  toward cursor is a nice touch).
- **Decisions:** smoothing (lerp vs none); zoom limits; whether the grid snaps to
  integer scales for crispness. ⚠️ Continuous zoom is what motivates SDF (P2.9).
- **DoD:** smooth pan/zoom over a large grid.

### P2.7 — Freeform sprite/quad API — the freeform render style
- Free world-space placement of textured quads + text labels; z-order/layering;
  blend modes.
- **Decisions:** layer/z model; whether grid and freeform compose in one scene or
  are mutually exclusive styles; sorting strategy.
- **DoD:** sprites + text coexist with correct layering; a flag swaps grid-style
  vs freeform-style rendering of the same scene.

### P2.8 — pybind11 renderer bindings
- Expose: create window, load atlas, set camera, submit grid buffer, submit
  sprite buffer, present, poll input.
- **Decisions:** bulk buffers as numpy arrays; lifetime/ownership of GL resources
  across the FFI; threading model (must GL calls stay on one thread?); error
  propagation to Python.
- **DoD:** a Python script animates a grid with zero C++ edits.

### P2.9 — SDF integration (pull in Phase 1 P1.9)
- Build the SDF atlas in the engine; add the SDF sampling shader; switch text to
  SDF for crisp arbitrary-zoom.
- **Decisions:** SDF vs MSDF (corner sharpness); shader AA approach
  (smoothstep over screen-space derivative); fallback for cells that don't need
  SDF.
- **DoD:** text stays sharp across the full zoom range.

---

## Forward dependencies to honor now (for Phases 3 & 4)

- The **grid cell buffer format** (P2.5) is also the game's render interface and
  effectively the network-visible state shape — design it deliberately.
- Keep the public API **small and stable**: Phase 3 and Phase 4 both depend on it.
- Track every desktop-only GL feature in `docs/gl_portability.md` so the GLES
  port (Phase 4) is a checklist, not an investigation.

---

## Milestone summary

| Milestone | Done when |
|---|---|
| **P2.1** | Window + GL context + stable loop |
| **P2.2** | Quad renders through RAII GL wrappers |
| **P2.3** | Thousands of batched sprites at FPS |
| **P2.4** | Crisp atlas text string on screen |
| **P2.5** | 200×80 colored grid at 60 FPS, one draw call |
| **P2.6** | Smooth pan/zoom over a large grid |
| **P2.7** | Grid + freeform styles both render a scene |
| **P2.8** | Python drives the renderer with no C++ edits |
| **P2.9** | SDF text crisp at any zoom |

---

## Open decisions

- **GL version floor** — 3.3 core is the recommendation; confirm against your
  dev GPU and the Pi's GLES capabilities.
- **Instancing path** — instanced arrays (GLES 3.0 OK) vs a texture-buffer of
  cell data; pick the one that ports cleanly.
- **Color model for cells** — full RGBA per cell vs a fixed palette index (palette
  is smaller on the wire and very "terminal"). Coordinate with Phase 3.
- **Input ownership** — does the renderer own the event loop, or hand raw events
  to Python? Affects how the game's main loop is structured.
- **Multiple fonts / fallback** — one atlas now; design for multiple atlases
  later (CJK, symbols)?
