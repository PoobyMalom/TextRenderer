# TextRenderer — Implementation Plan

## Goal

Build a smooth, interactive SDL2 text viewer that rasterizes TrueType glyphs on the CPU and renders them into a zoomable, pannable 2D canvas. The viewer doubles as a debug/tuning tool with live parameter controls — font size, line spacing, fill mode, anti-aliasing, and more.

---

## Current State

- TTF parsing works (head, maxp, loca, cmap, glyf tables)
- Glyphs are drawn as Bezier **outlines only** using `SDL_RenderDrawLine`
- Basic scroll with arrow keys and mouse wheel
- Basic zoom with +/- (rebuilds nothing, just rescales on draw)
- Entire canvas redrawn every frame (no caching, no culling)
- Advance width is hardcoded to 600 font units for all glyphs
- No filled rasterization, no anti-aliasing, no font metrics

---

## Architecture Overview

```
TTF File
   │
   ▼
[Parsing Layer]          (existing — fix bugs, then freeze)
   │  TTFFile, Glyph, CmapTable, LocaTable, HeadTable, MaxpTable
   │  + NEW: HheaTable, HmtxTable
   │
   ▼
[Layout Engine]          (new)
   │  GlyphInstance positions in font-unit world space
   │  Line breaking, advance widths, baseline, line height
   │
   ▼
[CPU Rasterizer]         (new — the core new work)
   │  Bezier flattening → scanline fill → SDL_Surface pixels
   │  Optional: super-sample anti-aliasing
   │
   ▼
[Glyph Cache]            (new)
   │  SDL_Texture per (glyphIndex, pixelSize)
   │  Invalidated on zoom change past threshold
   │
   ▼
[Viewport / Camera]      (new — replaces current large-canvas approach)
   │  World-space coordinates, smooth pan/zoom
   │  Frustum culling — skip off-screen glyphs
   │
   ▼
[SDL2 Renderer + HUD]    (refactor of existing main.cpp)
   │  Blit cached glyph textures via SDL_RenderCopy
   │  Draw HUD overlay with current parameter values
```

---

## Phase 0 — Prerequisites (from TODO.md)

Complete these before any new feature work. They directly unblock later phases.

- [ ] Fix **B3** — `TTFFile::parse` inside compound glyph loop (blocks compound glyphs)
- [ ] Fix **B5** — unthrown `runtime_error` in `getGlyphIndex`
- [ ] Fix **B6** — Format 4 cmap parsing offset (blocks many fonts)
- [ ] Fix **M1/M2** — raw `new` leaks (corrupts memory on long runs)
- [ ] Fix **BLD1** — hardcoded Homebrew paths (blocks building)
- [ ] Fix **D4** — decouple SDL from `GlyphTable.h` (needed for clean rasterizer split)
- [ ] Fix **D5** — getters return `const vector<T>&` (needed for performance)
- [ ] Remove debug `cout` spam (**DC2, DC3**) — pollutes perf measurements

---

## Phase 1 — Font Metrics (parse `hhea` and `hmtx` tables)

Currently advance width is hardcoded. Real text layout requires per-glyph advance widths and global line height metrics.

### 1.1 — Parse `hhea` table

New file pair: `include/HheaTable.h`, `src/HheaTable.cpp`

Fields needed:
```
int16_t  ascender           — distance above baseline (in font units)
int16_t  descender          — distance below baseline (negative)
int16_t  lineGap            — extra spacing between lines
uint16_t advanceWidthMax    — widest glyph
int16_t  numberOfHMetrics   — count of full hMetric entries in hmtx
```

### 1.2 — Parse `hmtx` table

New file pair: `include/HmtxTable.h`, `src/HmtxTable.cpp`

Structure (per glyph, count = `numberOfHMetrics` from hhea):
```
uint16_t advanceWidth
int16_t  lsb          — left side bearing
```
Glyphs beyond `numberOfHMetrics` share the last `advanceWidth` but have individual `lsb` entries.

Expose: `uint16_t getAdvanceWidth(uint16_t glyphIndex) const`

### 1.3 — Wire into TTFFile

Add `HheaTable hheaTable` and `HmtxTable hmtxTable` to `TTFFile`. Remove the hardcoded 600-unit constant everywhere.

**Default values to derive from hhea:**
```
lineHeight   = (ascender - descender + lineGap) * lineSpacingMultiplier
baseline     = ascender (distance from top of em square to baseline)
```

---

## Phase 2 — Text Layout Engine

New file pair: `include/TextLayout.h`, `src/TextLayout.cpp`

### Data Structures

```cpp
struct GlyphInstance {
    uint16_t glyphIndex;
    float    x;          // left edge, in font units from layout origin
    float    y;          // baseline, in font units from layout origin
};

struct TextLayout {
    std::vector<GlyphInstance> glyphs;
    float totalWidth;    // font units
    float totalHeight;   // font units
};
```

### Layout Algorithm

```
pen_x = 0, pen_y = 0 (baseline of first line, y increases downward in layout space)
maxLineWidth = desired wrap width in font units

for each codepoint in text:
    glyphIndex = cmap.getGlyphIndex(codepoint)
    advanceWidth = hmtx.getAdvanceWidth(glyphIndex)

    if codepoint == '\n':
        pen_x = 0
        pen_y += lineHeight
        continue

    if pen_x + advanceWidth > maxLineWidth and pen_x > 0:
        pen_x = 0
        pen_y += lineHeight

    emit GlyphInstance { glyphIndex, pen_x, pen_y }
    pen_x += advanceWidth
```

### Parameters

| Parameter | Default | Range | Notes |
|---|---|---|---|
| `lineSpacingMultiplier` | 1.2 | 0.5 – 3.0 | Multiplied with `ascender - descender + lineGap` |
| `letterSpacing` | 0 | -200 – 500 | Extra font units added to each advance width |
| `wrapWidth` | auto | 1 – ∞ | In font units; auto = fit to viewport |

---

## Phase 3 — CPU Glyph Rasterizer

This is the core new work. Output: an `SDL_Surface*` containing a filled, anti-aliased glyph bitmap at a specified pixel size.

New file pair: `include/Rasterizer.h`, `src/Rasterizer.cpp`

### 3.1 — Bezier Flattening

Convert a glyph's quadratic Bezier contours into polylines (lists of `(x, y)` line segments) at the target pixel size.

```cpp
// Returns a list of contours; each contour is a list of 2D points in pixel space
std::vector<std::vector<SDL_FPoint>> flattenGlyph(
    const Glyph& glyph,
    float pixelsPerUnit,   // zoom * unitsPerEm / pixelSize
    float originX,
    float originY
);
```

Quadratic Bezier subdivision:
- Recursively subdivide until the curve is flat (deviation from chord < 0.5 px)
- Or: use fixed step count adaptive to segment length
- Flatness test: `distance(midpoint_of_chord, midpoint_of_curve) < threshold`

### 3.2 — Scanline Fill (winding number rule)

TrueType uses the **non-zero winding number** rule.

```
for y in [yMin_px .. yMax_px]:
    intersections = []
    for each edge (p0 → p1) in all flattened contours:
        if edge crosses scanline y:
            x = interpolate x at y
            winding = +1 if going upward, -1 if going downward
            intersections.push_back({x, winding})
    
    sort intersections by x
    
    winding = 0
    for each intersection left to right:
        winding += intersection.winding
        if winding != 0:   // inside
            fill pixels from this x to next intersection x
```

Output pixel format: 8-bit grayscale coverage (0 = transparent, 255 = fully inside).

### 3.3 — Anti-aliasing

**Option A (simple, implement first):** Supersampling
- Rasterize at 2× or 4× the target size
- Downsample with box filter: each output pixel = average of 4 (or 16) input pixels
- Coverage values 0–255

**Option B (better, implement second):** Analytical coverage
- For each scanline, compute fractional x coverage at each edge crossing
- Fill partial pixels proportionally rather than sampling

Start with Option A — it's straightforward and already gives good results at 2×.

### 3.4 — SDL_Surface Output

```cpp
SDL_Surface* rasterizeGlyph(
    const Glyph& glyph,
    int pixelSize,            // target height in pixels
    uint16_t unitsPerEm,
    SDL_Color fgColor,        // text color
    SDL_Color bgColor         // background (for blending)
);
```

Implementation:
```cpp
// 1. Compute scale: pixelsPerUnit = pixelSize / unitsPerEm
// 2. Flatten bezier curves to pixel-space polylines
// 3. Allocate coverage buffer (width x height, 8-bit float or uint8)
// 4. Run scanline fill, write coverage values
// 5. If supersampling: downsample coverage buffer
// 6. Create SDL_Surface with ARGB8888 format
// 7. Write pixels: alpha = coverage, RGB = fgColor
// 8. Return surface (caller owns it)
```

---

## Phase 4 — Glyph Cache

Rasterizing a glyph is expensive. Cache the result as an `SDL_Texture`.

New file pair: `include/GlyphCache.h`, `src/GlyphCache.cpp`

```cpp
struct CacheKey {
    uint16_t glyphIndex;
    int      pixelSize;
    SDL_Color fgColor;
    bool operator==(const CacheKey&) const;
};

struct CacheEntry {
    SDL_Texture* texture;
    int width, height;    // in pixels
    int bearingX;         // pixels from pen position to left edge of glyph
    int bearingY;         // pixels from baseline to top of glyph
};

class GlyphCache {
public:
    GlyphCache(SDL_Renderer* renderer, const TTFFile& ttf);
    ~GlyphCache();  // destroys all textures

    const CacheEntry* get(uint16_t glyphIndex, int pixelSize, SDL_Color fg);
    void invalidate();   // clear all — call when zoom crosses a threshold
    size_t size() const;

private:
    std::unordered_map<CacheKey, CacheEntry, CacheKeyHash> cache;
    SDL_Renderer* renderer;
    const TTFFile& ttf;
};
```

### Cache Invalidation Strategy

Don't invalidate on every zoom tick — that would cause constant rasterization during scroll.
Invalidate when zoom crosses a **size threshold**:

```
cachedPixelSize = round(currentZoom * unitsPerEm / someBaseUnit)
if abs(cachedPixelSize - lastCachedPixelSize) > 4:
    cache.invalidate()
    lastCachedPixelSize = cachedPixelSize
```

This lets you zoom smoothly by scaling the cached texture (blurry but fast), then re-rasterizes once you settle at a new size.

---

## Phase 5 — Viewport / Camera System

Replace the current large canvas texture approach with a proper world-space camera.

```cpp
struct Camera {
    float worldX;       // world-space x of viewport center (font units)
    float worldY;       // world-space y of viewport center (font units)
    float zoom;         // pixels per font unit (current)
    float targetZoom;   // for smooth interpolation
    float targetX;      // for smooth pan interpolation
    float targetY;
};
```

### Coordinate Transforms

```cpp
// World (font units) → Screen (pixels)
SDL_Point worldToScreen(float wx, float wy, const Camera& cam, int screenW, int screenH) {
    return {
        (int)((wx - cam.worldX) * cam.zoom + screenW / 2.0f),
        (int)((wy - cam.worldY) * cam.zoom + screenH / 2.0f)
    };
}

// Screen → World (for mouse picking)
SDL_FPoint screenToWorld(int sx, int sy, const Camera& cam, int screenW, int screenH) {
    return {
        (sx - screenW / 2.0f) / cam.zoom + cam.worldX,
        (sy - screenH / 2.0f) / cam.zoom + cam.worldY
    };
}
```

### Smooth Pan/Zoom (lerp each frame)

```cpp
// In render loop, each frame:
float lerpFactor = 1.0f - powf(0.1f, dt);  // dt = frame time in seconds
cam.worldX += (cam.targetX - cam.worldX) * lerpFactor;
cam.worldY += (cam.targetY - cam.worldY) * lerpFactor;
cam.zoom   += (cam.targetZoom - cam.zoom) * lerpFactor;
```

Zoom toward cursor (not screen center):
```cpp
// On scroll wheel:
SDL_FPoint mouseWorld = screenToWorld(mouseX, mouseY, cam, W, H);
cam.targetZoom *= zoomFactor;
// Adjust target position so world point under mouse stays fixed
cam.targetX = mouseWorld.x - (mouseX - W/2.0f) / cam.targetZoom;
cam.targetY = mouseWorld.y - (mouseY - H/2.0f) / cam.targetZoom;
```

### Frustum Culling

Only render glyphs whose bounding boxes intersect the visible viewport:

```cpp
SDL_FRect viewportWorld = {
    cam.worldX - screenW / (2 * cam.zoom),
    cam.worldY - screenH / (2 * cam.zoom),
    screenW / cam.zoom,
    screenH / cam.zoom
};

for (const GlyphInstance& gi : layout.glyphs) {
    SDL_FRect glyphBounds = getGlyphWorldBounds(gi, ttf);
    if (!SDL_HasIntersectionF(&viewportWorld, &glyphBounds)) continue;
    // render this glyph
}
```

---

## Phase 6 — Rendering Loop

Replace `main.cpp` render loop with:

```
each frame:
  1. Handle input events (pan/zoom/toggle)
  2. Lerp camera toward targets
  3. Determine if cache needs invalidation (zoom threshold crossed)
  4. SDL_SetRenderDrawColor white, SDL_RenderClear
  5. For each GlyphInstance in layout.glyphs:
       a. Skip if outside viewport (culling)
       b. Compute screen position from world position + camera
       c. Compute pixelSize = glyphHeight * camera.zoom / unitsPerEm
       d. Get or create CacheEntry for (glyphIndex, pixelSize)
       e. SDL_RenderCopy(renderer, entry.texture, NULL, &dstRect)
  6. Draw HUD overlay
  7. SDL_RenderPresent
```

### Dirty Flag

```cpp
bool layoutDirty = true;   // rebuild text layout (word wrap, etc.)
bool cacheDirty  = false;  // true when zoom crosses threshold

if (layoutDirty) { layout = buildTextLayout(...); layoutDirty = false; }
if (cacheDirty)  { glyphCache.invalidate(); cacheDirty = false; }
```

---

## Phase 7 — HUD Overlay

Draw a semi-transparent info panel in the top-left corner each frame using `SDL_SetRenderDrawBlendMode` and `SDL_SetRenderDrawColor`.

### HUD Content

```
Font:          JetBrainsMono-Bold.ttf
Glyphs:        1234 rendered / 456 visible / 789 cached
Font size:     48 pt  (unitsPerEm: 2048)
Zoom:          2.34x  (pixelSize: 112 px)
Line spacing:  1.20x
Letter spacing: +0 units
Pan:           (12345, 6789) font units
FPS:           60.0
Mode:          Fill | AA ON | BBox OFF | Points OFF
```

Render text to HUD using SDL2_ttf (for the HUD itself) or a prebuilt bitmap font so the HUD doesn't depend on the renderer being built.

---

## Controls Reference

### Mouse

| Action | Effect |
|---|---|
| Left-drag | Pan camera |
| Scroll wheel | Zoom in/out toward cursor |
| Ctrl+Scroll | Fine zoom (smaller steps) |
| Middle-click drag | Pan camera (alternate) |

### Keyboard — Navigation

| Key | Action |
|---|---|
| `W` / `A` / `S` / `D` | Pan up/left/down/right |
| Arrow keys | Pan (same as WASD) |
| Shift + direction | Pan 5× faster |
| `+` / `=` | Zoom in |
| `-` | Zoom out |
| `0` | Reset camera (fit all text to window) |
| `1` | Jump to 1:1 pixel zoom |
| `2` | Jump to 2× zoom |
| `Home` | Jump to top-left of text |

### Keyboard — Rendering Mode

| Key | Action |
|---|---|
| `F` | Toggle fill / outline-only |
| `A` | Toggle anti-aliasing on/off |
| `P` | Toggle show control points (debug) |
| `G` | Toggle show glyph bounding boxes |
| `B` | Toggle background: white / black / gray |
| `H` | Toggle HUD overlay |
| `V` | Toggle baseline/ascender/descender guides |

### Keyboard — Text Parameters (live, triggers layout + cache rebuild)

| Key | Effect | Step | Range |
|---|---|---|---|
| `]` | Font size up | +2 pt | 4 – 256 pt |
| `[` | Font size down | −2 pt | 4 – 256 pt |
| Shift+`]` | Font size up large | +8 pt | 4 – 256 pt |
| Shift+`[` | Font size down large | −8 pt | 4 – 256 pt |
| `L` | Line spacing up | +0.05 | 0.5 – 4.0 |
| Shift+`L` | Line spacing down | −0.05 | 0.5 – 4.0 |
| `.` | Letter spacing up | +10 units | −500 – 1000 |
| `,` | Letter spacing down | −10 units | −500 – 1000 |
| `W` (held + scroll) | Adjust wrap width | ±50 units | 500 – ∞ |
| `R` | Reset all text params to defaults | | |

### Parameter Defaults

| Parameter | Default | Notes |
|---|---|---|
| Font size | 24 pt | At 72 DPI: 24pt = 32px |
| Line spacing | 1.2 | Multiplied with `(ascender - descender + lineGap)` |
| Letter spacing | 0 | Added to each advance width in font units |
| Fill mode | Fill (solid) | |
| Anti-aliasing | On (2× supersample) | |
| Background | White | |
| Wrap width | Viewport width | Recalculated on window resize |
| Supersampling | 2× | 1 = off, 2 = 4 samples, 4 = 16 samples |

---

## Phase 8 — Render Modes in Detail

### Mode 1: Fill (default)
Solid filled glyphs using scanline fill. Anti-aliased edges when AA is on.

### Mode 2: Outline
Draw only the Bezier outlines as line segments (current behavior). Useful for debugging curve quality.

### Mode 3: Wireframe  
Outline + control points: on-curve points drawn as green dots, off-curve as red dots, control handles as gray lines. Shows the raw TrueType data.

### Mode 4: Filled + Wireframe overlay
Fill the glyph, then overlay the wireframe on top. Best debugging view.

### Toggle: Bounding Boxes
Draw a thin rectangle for each glyph's `[xMin, yMin, xMax, yMax]` bounding box in font units.

### Toggle: Metrics Guides
Draw horizontal lines for:
- Baseline (blue)
- Ascender (green)  
- Descender (red)
- x-height (yellow, optional — needs OS/2 table or heuristic)

---

## Phase 9 — Window Resize Support

Handle `SDL_WINDOWEVENT_RESIZED`:
- Update `SCREEN_WIDTH` / `SCREEN_HEIGHT`
- Recalculate `wrapWidth` if set to auto
- Set `layoutDirty = true`
- No glyph cache invalidation needed (pixel sizes don't change on resize)

---

## Milestones

| Milestone | Done when |
|---|---|
| **M0: Clean build** | All Phase 0 bugs fixed, builds on Linux with `sdl2-config` |
| **M1: Metrics** | `hhea`/`hmtx` parsed; advance widths drive real character spacing |
| **M2: Layout** | Text wraps at viewport edge; line spacing matches font metrics |
| **M3: Fill** | At least one glyph rendered as a solid filled shape (no AA yet) |
| **M4: Full fill** | All simple glyphs rasterized; compound glyphs working |
| **M5: AA** | 2× supersampled anti-aliasing active; text is smooth |
| **M6: Cache** | Glyph cache working; pan is smooth at 60fps; no re-rasterize on pan |
| **M7: Camera** | Smooth lerp zoom; zoom toward cursor; culling active |
| **M8: HUD** | All parameters shown on screen; all keyboard controls wired up |
| **M9: Polish** | Resize support; all render modes; metrics guides; wireframe mode |

---

## Open Questions / Decisions to Make Later

- **Kerning**: The `kern` table provides pair-specific spacing adjustments. Add it after `hmtx` is working; it's optional for a first pass.
- **Subpixel rendering (ClearType-style)**: Requires RGB channel offsets per pixel. Complex. Probably out of scope.
- **OS/2 table**: Contains `sTypoAscender`, `sTypoDescender`, `usWinAscent` — better line height values than `hhea` for some fonts. Parse it in a later pass.
- **glyph compositing**: Currently glyphs draw over each other if they overlap. For correct text, draw each glyph into its own ARGB surface then alpha-blend into the scene.
- **Dynamic font loading**: Add a file picker or command-line argument for the font path rather than hardcoding it in `main.cpp`.
