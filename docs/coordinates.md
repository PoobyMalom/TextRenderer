# Coordinate System

## Font units and `unitsPerEm`

All geometry in a TTF file — glyph outlines, advance widths, bearings, ascender,
descender — is stored in an abstract integer grid called **font units**. The size
of that grid is `unitsPerEm`, stored in the `head` table. Common values are 1000
(PostScript-origin fonts) and 2048 (TrueType).

Font units are resolution-independent. They only become pixels when multiplied by
a target size:

```
pixels = fontUnits * pixelsPerEm / unitsPerEm
```

`pixelsPerEm` is the caller's requested render size (e.g. 32 for a 32px font).
This is the engine's native input unit — callers converting from point size should
use `ppem = pointSize * dpi / 72` before passing it in.

This transform lives in `FontTransform` (`include/Metrics.h`).

## The baseline and vertical metrics

The **baseline** is the line glyphs sit on, at Y = 0 in font coordinates. All
vertical measurements are relative to it:

```
        ^  +ascent   (top of capital letters, from hhea.ascent)
        |
   ─────┼───────────  baseline  (Y = 0)
        |
        v  +descent  (below baseline, hhea.descent is negative)
```

Line height = `ascent - descent + lineGap` (all from `hhea`, in font units).

## Y-axis convention

TTF glyph coordinates are **Y-up**: positive Y moves toward the ascender.
Screen and atlas pixels are **Y-down**: Y=0 is the top row, Y increases downward.

The flip happens at the render boundary via `FontTransform::toScreenY`:

```
screenY = baselineY - toPixels(fontY)
```

Where `baselineY` is the pixel row of the baseline on screen. This is the only
place the flip should occur — never flip inline in rendering code.

## Worked example

Font: 2048 `unitsPerEm`, `hhea.ascent` = 1638, `hhea.descent` = -410.
Target size: 32 `pixelsPerEm`.

```
scale = 32 / 2048 = 0.015625

ascent in pixels  = 1638 * 0.015625 = 25.6 px
descent in pixels =  410 * 0.015625 =  6.4 px
line height       = (1638 + 410) * 0.015625 = 32 px
```

Rendering the string "Ag" with baseline at `baselineY = 100`:

```
Glyph 'A':  advanceWidth = 1024 font units  →  16 px
             leftSideBearing = 0
             cursor starts at x = 0, draw at x = 0
             cursor after 'A': x = 16

Glyph 'g':  advanceWidth = 1024 font units  →  16 px
             cursor starts at x = 16, draw at x = 16
             cursor after 'g': x = 32

Outline point at fontY = 1200:
  screenY = 100 - (1200 * 0.015625) = 100 - 18.75 = 81  (above baseline on screen)

Outline point at fontY = -200 (descender):
  screenY = 100 - (-200 * 0.015625) = 100 + 3.125 = 103  (below baseline on screen)
```
