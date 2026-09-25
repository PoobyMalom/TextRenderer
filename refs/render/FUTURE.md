# Phase 2 — Renderer Future Work

Capabilities beyond the Phase 2 MVP (`PLANS.md` in this folder). Roughly ordered
by value to the game and to Phase 4. No code — purpose, what it unlocks, and the
decision it carries.

---

## Group 1 — Quality & sharpness

### MSDF text (upgrade from single-channel SDF)
Multi-channel SDF keeps sharp corners (box-drawing glyphs, sharp serifs) that
plain SDF rounds. **Unlocks:** crisp UI and box-art at extreme zoom.
**Decision:** worth the generation complexity, or is single-channel SDF enough
for an ASCII aesthetic?

### Gamma-correct + subpixel-aware blending
Blend text coverage in linear space; optionally subpixel (LCD) AA.
**Unlocks:** correct perceived text weight, sharper small text.
**Decision:** linear-space pipeline now vs later; subpixel is likely out of scope
for a grid game.

### HiDPI / fractional scaling
Handle high-DPI displays and the Pi panel's pixel density.
**Decision:** render at native pixels and scale the camera, or render to a
fixed virtual resolution and upscale?

---

## Group 2 — Throughput at scale

### Dirty-rectangle / dirty-cell grid updates
Only re-upload changed cells instead of the whole grid each frame.
**Unlocks:** large factory views at high FPS with cheap updates.
**Decision:** dirty tracking granularity (cell, row, region).

### Multiple atlases & glyph fallback
Symbols, box-drawing, CJK, emoji across several atlases with a fallback chain.
**Decision:** fallback order; atlas-switch cost in the instanced path.

### Render-to-texture / layers / post-processing
Offscreen targets for UI compositing, minimap, bloom/scanline "CRT" effects.
**Unlocks:** a minimap for the factory; retro post FX. **Decision:** how many
layers; whether post-FX is in scope at all.

### Instanced primitives beyond quads
Lines, rectangles, circles for belts/wires/connection overlays without text.
**Decision:** keep everything as textured quads, or add primitive shaders?

---

## Group 3 — Portability (feeds Phase 4)

### Explicit GLES backend abstraction
Formalize the GL/GLES split behind one interface so the Pi build is a config
flag, not a fork. **Decision:** do this once the desktop renderer is stable;
informed by `docs/gl_portability.md`.

### Headless / offscreen rendering
Render to an image without a window — useful for golden-image tests and for the
dedicated server to generate previews. **Decision:** EGL/pbuffer vs a software
path.

---

## Group 4 — Convenience for the game layer

### Higher-level text/UI helpers in the Python API
Labels, text boxes, simple widgets built on the grid/quad primitives.
**Decision:** how much UI lives in C++ vs Python — recommend Python owns UI,
C++ owns drawing.

### Camera niceties
Edge-pan, follow-target, bookmarks, smooth recenter. **Decision:** belongs in the
renderer or in the game's input layer?

### Animation/tween hooks
Per-instance time uniforms for animated belts/flashing entities without CPU work.
**Decision:** generic time uniform vs per-effect shaders.

---

## Priority summary

| Priority | Item | Effort | Payoff |
|---|---|---|---|
| 1 | Dirty-cell grid updates | Medium | Big-factory FPS |
| 1 | GLES backend abstraction | Medium | Unblocks Phase 4 |
| 2 | MSDF text | Medium | Sharp box-art at zoom |
| 2 | Multiple atlases / fallback | Medium | Symbols, CJK, emoji |
| 2 | Instanced primitives | Medium | Belt/wire overlays |
| 3 | Render-to-texture / minimap | Medium | Minimap, post-FX |
| 3 | Headless rendering | Low–Med | Tests + server previews |
| 3 | HiDPI / fractional scaling | Low | Crisp on varied displays |
| 4 | Python UI helpers | Low | Faster game UI |
| 4 | Camera niceties / tweens | Low | Polish |
