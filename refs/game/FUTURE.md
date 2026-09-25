# Phase 3 — Game Future Work

Features beyond the Phase 3 MVP (`PLANS.md` in this folder). Roughly ordered by
value. No code — purpose, what it unlocks, and the decision it carries.

---

## Group 1 — Deeper factory systems

### More entity types & production tiers
Assemblers, smelters, drills, power, tiered recipes.
**Decision:** recipe/tech data as external data files vs hardcoded — recommend
data-driven from the start so this group is content, not code.

### Fluids & pipes
A second transport network with pressure/flow.
**Decision:** simulate flow physically or abstractly (source/sink balancing)?

### Trains / long-distance logistics
Rail network for moving items across a large base.
**Decision:** grid-locked rails vs freeform; pathfinding model.

### Circuit / logic network
Wires, signals, conditions — programmable factory control.
**Decision:** how much of a "language" to expose; ties into the freeform overlay
primitives from the renderer's future work.

### Power & pollution/biters (conflict systems)
Power grid; optional enemies/pollution for pressure.
**Decision:** is conflict in scope, or is this a pure builder?

---

## Group 2 — Player tools & UX

### Blueprints, copy/paste, undo
Select regions, stamp them, undo mistakes.
**Decision:** how blueprints serialize (shares format with save/load versioning).

### Tech tree / progression
Research unlocks recipes.
**Decision:** linear vs branching; ties to data-driven recipes.

### Better UI (inventory, crafting menus, tooltips, minimap)
Built on the renderer's grid/quad + future UI helpers.
**Decision:** UI in Python (recommended) using renderer primitives.

---

## Group 3 — Scale & performance

### C++ sim offload (the planned escape hatch)
Move the belt/inserter/machine hot loops into a C++ module via the existing
pybind11 toolchain when Python can't keep up.
**Decision:** the trigger (entity count / frame budget) and which subsystems move
first (belts almost certainly first). Keep hot data SoA so the port is mechanical.

### Chunked / streamed world
Infinite or very large maps loaded in chunks.
**Decision:** chunk size; what the server keeps hot vs cold.

### Spatial indexing
Grids/quadtrees for fast neighbor and region queries as entity counts grow.

---

## Group 4 — Networking maturity

### Delta compression & interest management
Send only what changed and only what a client can see.
**Decision:** per-client viewport interest vs whole-world deltas.

### Client-side prediction / interpolation
Smooth the view between server snapshots.
**Decision:** needed for a slow-tick factory game, or is interpolation enough?

### Reconnection, persistence, multiple saves
Server-side save rotation; clients rejoin cleanly.

### Spectator API hardening (feeds Phase 4)
A first-class view-only client role with its own auth and a minimal protocol
subset. **Decision:** define the spectator subset explicitly so the LCD/FPGA
clients implement the smallest possible surface.

---

## Group 5 — Modding & content

### Scripting / mod API
Let recipes, entities, and behaviors be defined in data or a scripting layer.
**Decision:** data-only mods vs an embedded scripting language.

### Procedural world generation
Resource/terrain generation with seeds.
**Decision:** determinism guarantees for shared seeds.

---

## Priority summary

| Priority | Item | Effort | Payoff |
|---|---|---|---|
| 1 | Data-driven recipes/entities | Medium | Unlocks most content cheaply |
| 1 | C++ sim offload (belts first) | High | Scale past Python's ceiling |
| 1 | Spectator API subset | Low–Med | Unblocks Phase 4 |
| 2 | Blueprints / copy / undo | Medium | Core quality-of-life |
| 2 | Fluids & pipes | High | Genre depth |
| 2 | Tech tree | Medium | Progression |
| 3 | Chunked world + spatial index | High | Big bases |
| 3 | Delta compression / interest mgmt | Medium | Network scale |
| 3 | Trains / circuits | High | Advanced logistics |
| 4 | Client prediction | Medium | Smoothness |
| 4 | Mod/scripting API | High | Longevity |
| 5 | Procedural worldgen | Medium | Replayability |
