# Phase 3 — Python Factory Game (Implementation Plan)

> Mid-level breakdown of `FULL_PLAN.md` §4. An ASCII-art-inspired, Factorio-style
> factory automation game in Python on top of the font engine and renderer, with
> an authoritative networked server. No code — structure and the decisions you
> own.

---

## Goal

A grid world of tiles and entities (belts, inserters, machines, resources) you
build up to automate production, rendered as a grid of glyph cells (with an
optional freeform style), running against an authoritative server so it can be
hosted on a dedicated box. Phase 4's LCD viewer reuses this client's render path
as a spectator.

**Depends on:** the Phase 2 renderer's Python API (P2.8) and the Phase 1 font
atlas (via the renderer).

---

## Guiding decisions (from FULL_PLAN, restated)

- 🚩 **Authoritative server, fixed-tick sim, decoupled render.** Sim at a fixed
  rate (start 20 UPS); render at display FPS. Server owns truth; clients send
  commands and receive state. No lockstep determinism.
- 🚩 **Build single-player first, but behind the network seam from day one.**
  Start with an in-process server reached only through the command/state
  interface; "going networked" is then swapping a function call for a socket.
- 🚩 **Sim never imports the renderer.** The client reads sim state and builds a
  render buffer. This is what lets Phase 4 reuse the client as a spectator.
- ⚠️ **Python sim performance is the real risk.** Keep hot data
  structure-of-arrays so a C++ offload of the belt/inserter tick is mechanical if
  needed.

---

## Architecture

```
[Server (authoritative)]            [Client]
  fixed-tick simulation               input → commands ──▶ (to server)
  world: tiles + entities             receives state snapshots/deltas
  applies commands                    builds render buffer from visible state
  broadcasts state          ◀───────▶ renderer (Phase 2) draws grid/freeform
        ▲                             camera / cursor / UI
        │ command + state protocol (versioned)
        └── in-process first, socket later
```

---

## Milestones

### P3.1 — Scaffold + import the stack
- Python project layout; import the renderer + font modules; open a window and
  show a static "hello" grid.
- **Decisions:** package layout; dependency/venv management; how the main loop is
  structured around the renderer's input model (from P2.8).
- **DoD:** a static grid renders from Python.

### P3.2 — Render a static world
- Tile map data → grid cell buffer → renderer.
- **Decisions:** tile/world data model; glyph + color mapping per tile type;
  coordinate convention (world↔grid↔screen).
- **DoD:** a hand-authored map renders with per-cell glyphs and colors.

### P3.3 — Input, camera, cursor
- Pan/zoom; a tile cursor; screen↔world mapping for picking.
- **Decisions:** control scheme; whether camera lives in the renderer or game;
  cursor/selection model.
- **DoD:** pan/zoom + a tile highlight under the cursor.

### P3.4 — Core sim: map + place/remove
- Tile/entity model; command handling; the in-process server.
- **Decisions:** entity representation (class hierarchy vs component/SoA);
  command set and validation; tick scheduler design.
- **DoD:** place and remove entities; state persists across ticks.

### P3.5 — Belts + item transport (the hard part)
- Model belts as transport-line **segments** (not per-item objects); items flow,
  turn corners, merge at junctions, back up when blocked.
- **Decisions:** ⚠️ segment/lane model is the single most important choice; item
  density representation; junction/merge rules; how belt state is rendered (glyph
  + animation).
- **DoD:** items ride belts realistically; performance holds at hundreds of belt
  tiles.

### P3.6 — Inserters, machines, recipes, resources
- Inserters move items between belts/machines; machines run recipes on a timer;
  resource patches deplete.
- **Decisions:** recipe data format; inventory model; inserter timing; resource
  patch representation.
- **DoD:** a closed mine→belt→machine→belt→output loop runs unattended.

### P3.7 — Save / load
- Serialize world + entity state; load deterministically.
- **Decisions:** format (JSON vs binary); **versioning/migration** from the start;
  what's authoritative on load.
- **DoD:** round-trip a built factory.

### P3.8 — Extract the server module (still in-process)
- Formalize the command/state protocol as data structures; the client only
  touches the sim through that interface.
- **Decisions:** protocol message shapes; full-state vs delta representation; tick
  ↔ message cadence.
- **DoD:** the client has zero direct access to sim internals.

### P3.9 — Network transport
- Put the protocol over the wire (TCP and/or WebSocket); join = full-state sync,
  then deltas at the tick rate; server on asyncio.
- **Decisions:** transport; serialization (JSON first for debugging → binary/
  msgpack later); snapshot vs delta; how clients reconcile; auth/handshake.
- **DoD:** two LAN clients see the same factory; one builds, the other watches.

### P3.10 — Dedicated server deployment
- Headless server process; config; restart-on-crash; basic allowlist/auth.
- **Decisions:** host/OS; config format; logging; how spectators (Phase 4)
  authenticate as view-only.
- **DoD:** the server runs on the dedicated box; clients connect over the network.

### P3.11 — Freeform render style option
- Player-selectable: grid style (default) vs freeform (entities as freely-placed
  sprites/quads + text). Both read the same sim state.
- **Decisions:** how entities map to freeform sprites; whether styles can mix; the
  toggle UX.
- **DoD:** switch styles at runtime; both render the same factory.

---

## Forward dependencies to honor now (for Phase 4)

- Keep **command-sending** and **state-receiving** as separate concerns — a
  spectator is "client minus commands."
- **Version the protocol** (P3.8/P3.9): Phase 4's viewer depends on a stable wire
  format.
- Keep the render-buffer build pure (no input side effects) so it runs on a
  view-only device.

---

## Milestone summary

| Milestone | Done when |
|---|---|
| **P3.1** | Static grid from Python |
| **P3.2** | Hand-authored map renders |
| **P3.3** | Pan/zoom + tile cursor |
| **P3.4** | Place/remove entities; ticking sim |
| **P3.5** | Belts move items at scale |
| **P3.6** | A closed production loop runs |
| **P3.7** | Save/load round-trips |
| **P3.8** | Client only talks to sim via the protocol |
| **P3.9** | Two LAN clients share a world |
| **P3.10** | Dedicated server hosts clients |
| **P3.11** | Grid + freeform styles selectable |

---

## Open decisions

- **Color model** — fixed terminal palette vs full RGB per cell (coordinate with
  Phase 2 P2.5; palette is smaller on the wire).
- **Multiplayer scope** — co-op only, or contested/PvP (affects server auth)?
- **Sim language boundary** — how long does pure Python last before the belt/
  inserter tick moves to C++ (the planned escape hatch)? Decide the trigger
  (entity count / frame budget) and profile against it.
- **World size model** — bounded map vs chunked/streamed infinite world?
- **Tick rate** — 20 UPS to start; revisit once belts exist.
- **Atlas charset** — confirm box-drawing/symbols are in the Phase 1 atlas if the
  art style needs them.
