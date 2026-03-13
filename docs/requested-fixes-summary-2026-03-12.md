# Requested Fixes Summary - 2026-03-12

## What Was Fixed

### 1. Weapon pickup after kill
- Fixed the issue where a player could not recover a weapon after a kill.
- Ground sword pickup is now processed independently, including when the opponent is dead.
- Killed fighters now properly drop their held sword to the ground.

### 2. Next map / segment progression with two players
- Verified and preserved progression flow where the priority owner (GO) can advance to the next segment.
- Added regression coverage to ensure segment transition still works while both players are alive.

### 3. Match rules: round time configuration
- Added configurable round time rule in match setup menu.
- Round time can now be changed directly from menu options.
- Round time is persisted through config save/load.
- Implemented round time limits and step values:
  - Minimum: 60 seconds
  - Default: 300 seconds
  - Maximum: 600 seconds
  - Step: 30 seconds

## Technical Changes (High Level)
- Updated combat and match flow for weapon drop/pickup behavior.
- Updated character death behavior so sword ownership state is correct after kill.
- Updated config system to clamp, persist, and load round time values.
- Updated menu/game setup logic to expose and modify round time rule.

## Validation
- Project builds successfully with CMake.
- Automated test suite passes.
- Added tests for:
  - sword pickup when opponent is dead
  - sword drop on kill
  - configurable round timeout behavior
  - segment advance while both players are alive

## File Created
- `docs/requested-fixes-summary-2026-03-12.md`
