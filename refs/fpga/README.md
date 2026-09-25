# Phase 5 — FPGA + Custom RISC-V (`fpga/`)

Placeholder folder for the tentative Phase 5: an FPGA running a RISC-V processor
that drives the **view-only** layer — receiving game state over a simple link and
blitting a text grid into a framebuffer, with the font engine's output as a
**glyph ROM**.

See `FULL_PLAN.md` §6 for the high-level plan, coarse milestones (P5.1–P5.6), and
key decisions (D-5A: run the view-only layer, not the game; D-5B: bring up an
existing soft core before designing your own; D-5C: pick an open toolchain if you
can), plus resources (Bruno Levy's `femtorv32`, Harris & Harris, etc.).

## What goes here

This phase is mostly **downloaded / vendored toolchains, soft cores, and board
support** (e.g. PicoRV32/VexRiscv/Ibex, yosys/nextpnr or Vivado projects,
riscv-gnu-toolchain output). You intend to pull those in here and **remove them
from the repo before committing**. So:

- Keep this `README.md` (and `.gitkeep`) tracked so the folder exists.
- Add vendored cores, toolchains, bitstreams, and build artifacts to `.gitignore`.
- Only commit small, original HDL/glue and the glyph-ROM generation config you
  write yourself.

## Design decisions to make (deferred until this phase)

- Board + toolchain: open flow (Lattice ECP5 / yosys-nextpnr) vs Xilinx Vivado.
- Which existing RISC-V soft core to bring up first.
- Display controller target (LCD vs VGA vs HDMI) and framebuffer layout.
- Link to the server/game state: UART vs SPI vs Ethernet.
- How the glyph ROM is generated from `libtr_font` (bitmap font export).
- Whether the custom RISC-V core (P5.6) is in scope or stays a stretch goal.

> The natural bridge from earlier phases is the font engine's bitmap/atlas output
> reframed as a glyph ROM — capture that export need back in Phase 1 if/when you
> commit to this phase.
