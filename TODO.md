# TODO

- [ ] **Support the remaining cmap subtable formats** (`src/CmapTable.cpp`). Only formats 0, 4 and 12 are parsed; anything else is skipped.
  - 2 — high-byte mapping (legacy CJK double-byte encodings)
  - 6 — trimmed table mapping (one contiguous 16-bit range; common as the Mac `1/0` subtable)
  - 8 — mixed 16/32-bit coverage (deprecated, rare)
  - 10 — trimmed array (32-bit version of 6)
  - 13 — many-to-one range mappings (last-resort fonts)
  - 14 — Unicode variation sequences (needs a `(codepoint, selector)` lookup, not just codepoint → glyph)
  - Also: format 0 lookups index out of bounds for codepoints ≥ 256, and platform 0 (Unicode) subtables are never chosen as the active subtable.

- [ ] **Fix bounding boxes not growing.**
- [ ] **Test other fonts** against the current rendering/triangulation pipeline, beyond the handful used so far.
- [ ] **Speed benchmarking for rendering** -- establish a baseline before the speed-up work below.
- [ ] **Render speed up.**
- [ ] **Rasterizing** -- replace/augment the brute-force per-pixel `pointInTriangle`/Loop-Blinn scan with a proper rasterization approach.
- [ ] **Text wrapping mode.**
- [ ] **Individual glyph colors.**
- [ ] **Dynamic font size via scrolling** -- zoom in/out on scroll.
- [ ] **Middle mouse button panning.**
- [ ] **Rendering pipeline using SDL3 GPU stuff** -- larger architectural move off the CPU-side renderer, once the above is stable.
