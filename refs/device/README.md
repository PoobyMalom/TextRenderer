# Phase 4 — Portable LCD Viewer (`device/`)

Placeholder folder for the Phase 4 portable LCD viewer: a Raspberry-Pi-class
device that connects to the dedicated game server as a **view-only spectator** and
pans around the factory with buttons.

See `FULL_PLAN.md` §5 for the high-level plan, milestones (P4.1–P4.5), key
decisions (D-4A: Pi-class hardware first; D-4B: reuse the Phase 3 client render
path; D-4C: cash in GLES portability), and resources.

## What goes here

This phase is mostly **downloaded / device-specific code and assets** (OS images,
GPIO/display drivers, on-device build of the renderer). You intend to pull those
in here and **remove them from the repo before committing**. So:

- Keep this `README.md` (and `.gitkeep`) tracked so the folder exists.
- Add downloaded SDKs, vendored drivers, and large assets to `.gitignore`.
- Only commit small, original config/glue you write yourself.

## Design decisions to make (deferred until this phase)

- Exact board (Pi Zero 2 W vs Pi 4) and display (SPI vs small HDMI).
- Rendering path on device: KMS/DRM (no desktop) vs running under X/Wayland.
- Spectator auth against the dedicated server (the view-only protocol subset).
- Button layout and camera control mapping.
- Power/enclosure/autostart for true portability.

> No engine/renderer changes belong here — portability work belongs in Phase 2
> (`docs/gl_portability.md`) and the spectator role in Phase 3.
