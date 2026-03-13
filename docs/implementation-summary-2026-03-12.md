# Implementation Summary - 2026-03-12

## Scope Completed
This document summarizes all gameplay, systems, and UI changes implemented in the current development pass.

## Controls and Input
- Rebinding flow hardened to avoid invalid option index crashes.
- Key persistence fixed and loaded on startup.
- Combat inputs updated:
  - `Attack` -> Thrust
  - `Parry` binding repurposed as Throw
- Crouch input integrated into movement/combat flow.

## Startup and Menus
- Press-start screen now shows blinking `PRESS ANY KEY TO START`.
- Added arm delay to prevent instant skip on first boot frame.
- Options menu keeps stable behavior during rebind and save.

## Combat System
- Nidhogg-style one-hit rules implemented.
- Three sword heights (`HIGH`, `MID`, `LOW`) used in lane checks.
- Auto-block behavior on matching sword height.
- Removed manual parry dependency from core kill logic.
- Fixed equal-height growth bug (sword no longer grows infinitely).

## Throw and Sword Handling
- Throw now uses a moving horizontal projectile.
- Projectile has:
  - launch position from sword origin
  - horizontal velocity (`THROW_SPEED`)
  - lane height at launch
- Projectile interactions:
  - blocked by matching enemy sword lane
  - kills on body hit
  - drops to ground on block/hit/out-of-bounds
- Added dropped sword world state and pickup logic.
- Added long fallback auto-return timeout for lost swords.

## Disarm and Unarmed Flow
- Disarm state integrated.
- Unarmed close-range kill path added.
- Crouch pickup of nearby dropped sword supported.

## Air and Mobility Actions
- Dive-like airborne attack disarm interactions added.
- Added downed state with recovery timer.
- Neck-snap finisher on downed enemy added.
- Added roll/cartwheel/crawl state behavior and timing hooks.
- Added wall-slide and wall-jump interaction logic.

## Priority and Map Progression
- Added priority (`GO`) ownership after kill.
- Double-kill clears priority.
- Segment progression tied to priority owner.
- Segment transitions keep entry/respawn consistency.
- Zone-cross counters tracked per player.

## Camera and Rendering
- Camera uses both fighters as focus (midpoint behavior).
- Transition overlay retained for segment changes.
- Rendered sword states:
  - held sword
  - dropped sword
  - sword in flight
- Game-over panel expanded with full stats.

## HUD and Match Stats
- Added priority indicator on HUD.
- Added armed/unarmed and downed status hints.
- Expanded tracked stats:
  - kills/deaths
  - throw kills
  - thrust kills
  - disarms
  - neck snaps
  - zones crossed
  - score and round time

## Stability and Validation
- CMake build completes successfully.
- Test suite passes successfully.
- Existing non-blocking CTest DartConfiguration warning remains unchanged.

## Main Files Touched (high-level)
- `include/types.h`
- `include/constants.h`
- `src/engine/input.c`
- `src/combat/character.c`
- `src/combat/physics.c`
- `src/game/config.c`
- `src/game/game.c`
- `src/game/menu.c`
- `src/game/match.c`
- `src/game/hud.c`
- `src/engine/render.c`
- `tests/test_game.c`
