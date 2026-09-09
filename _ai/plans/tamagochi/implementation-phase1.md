# Tamagotchi implementation Phase 1: establish the baseline

## Purpose and status

This phase creates the reproducible build, generated-asset, and firmware-size baseline that every later Tamagotchi phase compares against. It does **not** implement any Tamagotchi behavior or artwork.

The observations in **Current repository findings** were collected while this plan was authored. They are useful starting evidence, but they are not a substitute for executing the phase. In particular, no firmware build, upload, serial-monitor session, or hardware validation was run while preparing this document.

## Phase goal

Produce a successful release build of the unchanged pre-Tamagotchi firmware and record:

- the exact source revision and all pre-existing worktree changes;
- the effective PlatformIO platform, framework, libraries, and generator prerequisites;
- evidence that all three pre-build generators ran rather than silently leaving stale outputs;
- PlatformIO's flash and static RAM usage;
- the exact `firmware.bin` size, SHA-256, OTA-slot capacity, and remaining headroom;
- the current embedded-sprite count and raw ARGB8888 byte cost;
- all source/generated diffs caused by the build;
- any dependency-version or capacity decision that later phases must honor.

The durable output is `_ai/plans/tamagochi/phase1-baseline.md`. Later phases must quote its numbers rather than estimating from an old build or from generated C source-file sizes.

## Authoritative inputs

Read these files before executing the phase. If repository behavior conflicts with prose, record the conflict and resolve it explicitly rather than silently choosing one:

- `_ai/plans/tamagochi-features.md`
- `_ai/plans/tamagochi-implementation-plan.md`, especially Step 1 and the 200 KiB sprite budget
- `AGENTS.md`
- `docs/cards.md`
- `docs/games.md`
- `docs/input-and-navigation.md`
- `docs/display-and-lvgl.md`
- `docs/configuration-and-state.md`
- `docs/assets.md`
- `docs/ota.md`
- `docs/build-test-release.md`
- `platformio.ini`
- `partitions.csv`
- `htmlconvert.py`
- `ttf2c.py`
- `png2c.py`
- `.github/workflows/build-release.yml`

## Scope

Phase 1 includes:

1. capturing repository and toolchain identity;
2. checking generator prerequisites in the same Python environment PlatformIO uses;
3. inventorying current sprite inputs and generated descriptors;
4. running the configured release build;
5. checking generator output and generated-file diffs;
6. measuring the binary and OTA headroom;
7. recording a baseline report and any blocking decisions.

## Non-goals

Do not do any of the following in this phase:

- add `CardType::TAMAGOTCHI` or edit any card/controller code;
- create `src/tamagotchi/` or `src/ui/TamagotchiCard.*`;
- create or edit Tamagotchi art, `raw-png/` inputs, fonts, or portal sources;
- hand-edit files under `include/fonts/` or `include/sprites/`;
- add a native test environment or claim native tests passed;
- upload firmware, erase NVS, flash a board, start OTA, or use the release flash command;
- fix unrelated build, workflow, OTA, generator, or IDE issues without separately agreeing that scope;
- discard or overwrite pre-existing user changes.

## Prerequisites and dependencies

### Required local tools

- Git.
- PlatformIO Core capable of running environment `adafruit_feather_esp32s3_reversetft`.
- PlatformIO's Python interpreter with both Pillow and NumPy importable.
- Node.js, npm, and `npx`; `ttf2c.py` invokes `npm install --no-save lv_font_conv` during every build.
- Access to already installed PlatformIO packages, or network access if PlatformIO/npm must resolve missing dependencies.

### Required repository state

- Work from `/home/deliriusz/git/DeskHog`.
- Do not require a clean worktree. Instead, record and preserve every existing change.
- Before building, generated outputs under `include/` must either be clean or have clearly identified user-owned changes.
- The application partition limit is taken from `partitions.csv`, not from a board default.

### Phase ordering

- Phase 1 has no dependency on Tamagotchi implementation work.
- Phases 2 through 14 depend on the completed baseline report.
- Asset-conversion and final-validation phases additionally depend on the recorded binary headroom and sprite baseline.
- LVGL API-sensitive phases depend on resolving the documented/resolved LVGL version discrepancy described below.

## Current repository findings

These are plan-authoring observations as of 2026-09-09. Re-run the commands in the execution procedure because the branch may move before Phase 1 is performed.

### Source and worktree

- Repository root: `/home/deliriusz/git/DeskHog`.
- Branch: `main`.
- Observed commit: `67daad62ddb35b900967f42d78ef3d5f4fb4a7da`.
- The observed worktree already had a user change in `.vscode/extensions.json`; it adds `pioarduino.pioarduino-ide` to `unwantedRecommendations`.
- No Tamagotchi source, sprite, or generated symbols currently exist under `src/`, `include/`, `html/`, or `raw-png/`.
- The only native environment in `platformio.ini` is commented out. `test/README` remains a placeholder, so no native-test claim belongs in this phase.

### Configured build

- `platformio.ini` defines only `[env:adafruit_feather_esp32s3_reversetft]`.
- It uses board `adafruit_feather_esp32s3_reversetft`, Arduino, `build_type = release`, PSRAM-related flags, and `partitions.csv`.
- `src_filter` compiles `include/fonts/*.c` and `include/sprites/*.c`. PlatformIO 6.2.0 warns that `src_filter` is deprecated in favor of `build_src_filter`; that warning is not a Tamagotchi Phase 1 fix.
- Pre-build scripts run in this order: `htmlconvert.py`, `ttf2c.py`, `png2c.py`.
- The configured firmware version macro is `CURRENT_FIRMWARE_VERSION="0.1.5"`.
- `partitions.csv` provides two OTA application slots, `ota_0` at `0x20000` and `ota_1` at `0x210000`, each sized `0x1F0000` = 2,031,616 bytes. There is no filesystem partition.
- The normal build command is `pio run -e adafruit_feather_esp32s3_reversetft`.
- In the observed shell, `pio` was not on `PATH`, but `/home/deliriusz/.platformio/penv/bin/pio` exists and reports PlatformIO Core 6.2.0.

### Resolved dependencies

An observed `pio pkg list` resolved:

- PIOArduino Espressif32 platform 54.3.20;
- Arduino-ESP32 framework 3.2.0;
- LVGL 9.4.0;
- ArduinoJson 6.21.5;
- Bounce2 2.72.0;
- ESPAsyncWebServer 3.8.0;
- AsyncTCP 3.4.10;
- FastLED 3.10.3;
- Adafruit ST7735/ST7789 library 1.11.0.

This exposes a decision that must not be hidden: `platformio.ini` declares `lvgl/lvgl @ ^9.2.2`, while `AGENTS.md`, the feature specification, and the source implementation plan describe LVGL 9.2.2. The caret currently permits 9.4.0. Phase 1 must record the effective version and obtain an explicit project decision to either pin 9.2.2 or validate/adopt 9.4.0. Do not describe an API as “verified on LVGL 9.2.2” while building 9.4.0.

### Generator behavior

- `htmlconvert.py` reads `html/portal.html`, `html/portal.css`, and `html/portal.js`, then writes `include/html_portal.h`.
- `htmlconvert.py` catches several errors and returns without forcing a nonzero build result. Its success log and output existence must be checked.
- `ttf2c.py` invokes `npm install --no-save lv_font_conv`, generates four fonts into `include/fonts/`, and writes `include/fonts/fonts.h`.
- Individual font conversion failures are counted and printed, but the script does not exit nonzero merely because fewer than four fonts were converted. Require the `Successfully processed 4 of 4 fonts` evidence.
- `png2c.py` imports `PIL.Image` and NumPy at module load. If either import fails, it prints `Sprite conversion skipped` and exits with status 0. A green build is therefore insufficient proof of sprite regeneration.
- `png2c.py` sorts `raw-png/**/*.png`, groups frames by the first directory beneath `raw-png/`, derives symbols from each basename, and writes all per-frame `.c/.h` files into the flat `include/sprites/` directory.
- It writes pixel bytes in BGRA order and descriptors using `LV_COLOR_FORMAT_ARGB8888`, `data_size = width * height * 4`.
- It regenerates `include/sprites/sprites.h` and `include/sprites/sprites.c`, but does not delete stale per-frame files after a PNG is renamed or removed.

### Current sprite baseline

- Source inputs are six files: `raw-png/walking/Normal-Walking_01.png` through `Normal-Walking_06.png`.
- Each is 80x80, RGBA, with binary 0/255 alpha.
- Each costs 25,600 bytes as ARGB8888; the six-frame raw pixel total is 153,600 bytes.
- Generated descriptors are `sprite_Normal_Walking_01` through `sprite_Normal_Walking_06`.
- `include/sprites/sprites.h` declares `walking_sprites[]` and `walking_sprites_count`.
- `include/sprites/sprites.c` orders the six pointers lexicographically from `_01` through `_06`.
- The generated `.c` file's textual size is not firmware size and must not be used for OTA budgeting.

### Current build-output state

- `.pio/build/adafruit_feather_esp32s3_reversetft/idedata.json` exists.
- `firmware.bin`, `firmware.elf`, `firmware.map`, `partitions.bin`, and `bootloader.bin` were absent when this plan was authored.
- Consequently there is no valid pre-existing flash/RAM or OTA-size baseline to quote. A fresh successful build is a hard Phase 1 requirement.

## Execution procedure

### 1. Create a read-only pre-build snapshot

From the repository root, capture the following into the baseline report before running any generator or build:

```sh
pwd
git branch --show-current
git rev-parse HEAD
git log -1 --format='%cI %s'
git status --porcelain=v1 -uall
git diff --stat
```

Record each dirty path as one of:

- pre-existing user change;
- expected Phase 1 evidence file;
- unexpected/unowned change requiring investigation.

Do not stash, reset, checkout, clean, or delete any path. In particular, preserve `.vscode/extensions.json` if that change is still present.

Also record the relevant configuration without modifying it:

```sh
sed -n '1,140p' platformio.ini
sed -n '1,80p' partitions.csv
```

In `_ai/plans/tamagochi/phase1-baseline.md`, state the slot conversion explicitly:

```text
OTA slot hex:     0x1F0000
OTA slot decimal: 2,031,616 bytes
```

### 2. Resolve and record the build executable

Run:

```sh
command -v pio
pio --version
```

If `pio` is not on `PATH` on this workstation, use:

```sh
/home/deliriusz/.platformio/penv/bin/pio --version
```

Use the same executable for package inventory and the build. Record the exact executable path and version in the report.

Record effective dependencies:

```sh
/home/deliriusz/.platformio/penv/bin/pio pkg list -e adafruit_feather_esp32s3_reversetft
```

Do not treat the `src_filter` deprecation warning as a Phase 1 failure. Do record it as technical debt.

### 3. Prove generator prerequisites in the correct environments

The observed PlatformIO interpreter is `/home/deliriusz/.platformio/penv/bin/python`. Verify imports using that interpreter, not an unrelated system Python:

```sh
/home/deliriusz/.platformio/penv/bin/python -c 'import sys, PIL, numpy; print(sys.executable); print("Pillow", PIL.__version__); print("NumPy", numpy.__version__)'
node --version
npm --version
npx --version
```

Record interpreter and package versions. The authoring-time imports found Pillow 12.1.0 and NumPy 2.4.1; these are observations, not required pins.

If Pillow or NumPy is missing:

1. stop and mark the generator gate failed even if a later build exits 0;
2. do not rely on current generated sprite files;
3. request approval before installing packages into the user's PlatformIO environment;
4. after installation, rerun the import command and record the new versions.

If Node/npm/npx is missing or npm cannot resolve `lv_font_conv`, stop before claiming a baseline. Do not edit generated fonts manually.

### 4. Inventory inputs and generated assets before the build

Record the tracked input/output lists:

```sh
git ls-files html typography raw-png include/html_portal.h include/fonts include/sprites
git status --porcelain=v1 -uall -- include/html_portal.h include/fonts include/sprites
```

Verify the sprite source inventory with Pillow. The evidence must list for each PNG:

- path;
- width and height;
- image mode;
- alpha-channel minimum and maximum;
- whether all alpha values are binary;
- `width * height * 4` bytes.

Record totals. Before Tamagotchi assets, the expected values are six 80x80 RGBA frames and 153,600 raw ARGB8888 bytes. A mismatch is not automatically an error, but it means the source changed and the new measured value becomes the baseline only after review.

Check the current generated group:

```sh
rg -n 'walking_sprites|walking_sprites_count' include/sprites/sprites.h include/sprites/sprites.c
rg -n 'LV_COLOR_FORMAT_ARGB8888|\.w =|\.h =|data_size' include/sprites/sprite_Normal_Walking_*.c
```

Save pre-build hashes of generated outputs to temporary evidence outside the repository, for example under `/tmp/deskhog-tamagotchi-phase1/`. Do not add a large generated-file hash dump to the report; summarize whether files changed and retain the temporary file until the phase review is complete.

### 5. Run the configured release build

Create a temporary evidence directory, enable pipeline failure propagation in the shell, and run exactly the configured environment:

```sh
mkdir -p /tmp/deskhog-tamagotchi-phase1
set -o pipefail
pio run -e adafruit_feather_esp32s3_reversetft 2>&1 | tee /tmp/deskhog-tamagotchi-phase1/build.log
```

If `pio` remains unavailable on `PATH`, substitute `/home/deliriusz/.platformio/penv/bin/pio` without changing any other argument.

Do not run `clean` first. The normal command is the canonical build. If an incremental-build problem is suspected, record why, run the environment's clean target, then rerun the full build and clearly label which output is authoritative.

The build is successful only if all of these are true:

- the command exits 0;
- output ends with success for `adafruit_feather_esp32s3_reversetft`;
- `firmware.bin` and `firmware.elf` exist and are non-empty;
- PlatformIO prints flash and RAM usage;
- portal generation reports `include/html_portal.h` was written;
- font generation reports four of four fonts processed;
- sprite generation reports six sprites processed for the unchanged baseline, or the reviewed current source count;
- the log does not contain `Sprite conversion skipped`.

### 6. Review generator side effects before measuring

Immediately after the build, run:

```sh
git status --porcelain=v1 -uall
git diff --stat
git diff -- include/html_portal.h include/fonts include/sprites
```

Classify every new diff:

- **No diff:** preferred baseline result; generated outputs were current and deterministic in this environment.
- **Expected deterministic regeneration:** inspect every hunk and record why it differs. Do not call it harmless solely because it is generated.
- **Unexpected generated change:** phase fails until the source/generator cause is understood.
- **Unrelated path changed:** preserve it and identify its owner; do not fold it into Tamagotchi work.
- **Untracked `node_modules/` or npm metadata:** report the build-tool side effect. Do not commit it as Tamagotchi source, and do not delete a potentially pre-existing path without proving this build created it.

For sprites, confirm that the walking array remains ordered `_01` through `_06`, descriptors remain 80x80 ARGB8888 with `data_size = 25600`, and no stale extra per-frame outputs appeared.

### 7. Record flash, RAM, binary, and map evidence

The report must include the exact PlatformIO summary lines for:

- RAM percentage, used bytes, and maximum bytes;
- Flash percentage, used bytes, and maximum bytes.

Then measure the OTA artifact directly:

```sh
stat -c '%n %s bytes' .pio/build/adafruit_feather_esp32s3_reversetft/firmware.bin
sha256sum .pio/build/adafruit_feather_esp32s3_reversetft/firmware.bin
stat -c '%n %s bytes' .pio/build/adafruit_feather_esp32s3_reversetft/firmware.elf
stat -c '%n %s bytes' .pio/build/adafruit_feather_esp32s3_reversetft/firmware.map
```

Calculate and record:

```text
slotBytes                 = 2,031,616
firmwareBinBytes          = exact stat result
slotHeadroomBytes         = slotBytes - firmwareBinBytes
slotUsedPercent           = firmwareBinBytes / slotBytes * 100
plannedSpriteBudgetBytes  = 204,800
headroomAfterSpriteBudget = slotHeadroomBytes - 204,800
```

Keep PlatformIO's Flash number and `firmware.bin` size as separate fields; they measure related but not necessarily identical things.

If available, use the current Xtensa toolchain to retain a section-level reference:

```sh
/home/deliriusz/.platformio/packages/toolchain-xtensa-esp-elf/bin/xtensa-esp32s3-elf-size -A .pio/build/adafruit_feather_esp32s3_reversetft/firmware.elf
/home/deliriusz/.platformio/packages/toolchain-xtensa-esp-elf/bin/xtensa-esp32s3-elf-nm -S --size-sort .pio/build/adafruit_feather_esp32s3_reversetft/firmware.elf
```

Use the ELF/map to confirm the six existing sprite pixel arrays account for approximately 153,600 bytes before descriptor/alignment overhead. Record actual symbol sizes; do not infer firmware contribution from the much larger textual `.c` files.

### 8. Decide capacity and version gates

Apply these branches:

1. If `firmwareBinBytes >= 2,031,616`, stop. The pre-feature firmware already violates the OTA slot and later phases must not proceed.
2. If `slotHeadroomBytes < 204,800`, stop asset production and revise the Tamagotchi sprite manifest before generating art. The planned 200 KiB budget cannot fit even before model/UI growth.
3. If `slotHeadroomBytes >= 204,800`, record `headroomAfterSpriteBudget`; do not interpret this as guaranteed final fit because later C++ code and descriptor overhead also consume flash.
4. The exact planned raw Tamagotchi pixel payload is 44 frames at 32x32 plus one frame at 16x16: `44 * 4096 + 1024 = 181,248 bytes`. The 204,800-byte budget leaves 23,552 bytes inside the sprite allowance for descriptors/alignment, not for all remaining feature code.
5. If resolved LVGL is not 9.2.2, record a decision owner and outcome. The preferred consistency fix is an exact 9.2.2 pin followed by a fresh baseline build; the alternative is an explicit adoption of 9.4.0 with documentation and API validation. Do not silently change the dependency during a measurement run.

### 9. Write the durable baseline report

Create `_ai/plans/tamagochi/phase1-baseline.md` with this minimum structure:

```text
# Tamagotchi pre-feature baseline
Timestamp and timezone
Git branch and commit
Pre-existing dirty paths
Build command and exit result
PlatformIO executable/Core version
Resolved platform/framework/library versions
Generator prerequisite versions
Generator success evidence
Current source sprite inventory and raw byte total
PlatformIO RAM summary
PlatformIO Flash summary
firmware.bin bytes and SHA-256
OTA slot bytes, used percentage, and headroom
Projected headroom after 200 KiB sprite budget
ELF/map sprite-symbol total
Generated diffs and disposition
Warnings and blockers
LVGL version decision
Phase status: PASS or BLOCKED
```

Do not paste secrets, Wi-Fi configuration, API keys, arbitrary NVS contents, or the full noisy build log into the report. Refer to the temporary build log path and quote only relevant evidence lines.

## Failure and decision branches

### Build fails before linking

Capture the first causal error and the final PlatformIO summary. Consider at most five plausible root causes, narrow to the most likely one or two, and write a remediation plan before changing code. Common baseline candidates are missing npm/network access, missing PlatformIO packages, generator dependency failure, or an unrelated pre-existing source change.

Do not implement Tamagotchi code to work around a failing baseline. If repair would change unrelated firmware or tooling, report Phase 1 as blocked and request separate scope.

### Build reports success but sprites were skipped

Treat the phase as failed. Install/restore Pillow and NumPy only with appropriate approval, rerun the import check, and rebuild. Existing generated sprite files do not prove the current source inputs were converted.

### Build reports success but a different sprite count

Compare `raw-png/` with the pre-build inventory. If another phase or user concurrently added assets, do not overwrite them. Rebase the baseline on an agreed commit or coordinate ownership before continuing.

### Generated files change unexpectedly

Review source hashes, generator versions, output order, descriptors, and stale flat output files. Do not hand-edit generated outputs and do not discard the diff. Either accept a reviewed regeneration as a separate baseline change or stop for maintainer direction.

### Tool installation or dependency resolution needs network access

Do not claim the phase passed using cached/stale artifacts. Request permission for the environment change or execute in the repository's normal approved build environment, then record what was installed and rerun the entire build gate.

### Binary fits, but planned margin is poor

Record the exact shortage. The source implementation plan says to reduce action frame counts before reducing legibility. Do not change the 32x32 target, remove mandatory lifecycle readability, or begin high-resolution art production until the revised manifest has an agreed byte budget.

### LVGL version remains undecided

Phase 1 may record a successful current build, but mark the handoff as blocked for API-sensitive card/animation work. Asset concept work can proceed only if the project owner accepts that it does not validate the final LVGL integration target.

## Artifacts and deliverables

Required durable artifact:

- `_ai/plans/tamagochi/phase1-baseline.md`, completed with measured values and PASS/BLOCKED status.

Required local build artifacts:

- `.pio/build/adafruit_feather_esp32s3_reversetft/firmware.bin`
- `.pio/build/adafruit_feather_esp32s3_reversetft/firmware.elf`
- `.pio/build/adafruit_feather_esp32s3_reversetft/firmware.map` when emitted by the configured linker
- `.pio/build/adafruit_feather_esp32s3_reversetft/partitions.bin`

Temporary evidence, not intended for commit:

- `/tmp/deskhog-tamagotchi-phase1/build.log`
- pre/post generated-output hash inventories under the same temporary directory.

There should be no Tamagotchi source or runtime asset deliverable in Phase 1.

## Validation checklist

- [ ] All authoritative documents were read.
- [ ] Branch, commit, timestamp, and pre-existing dirty paths were recorded.
- [ ] No user-owned change was stashed, reverted, overwritten, or deleted.
- [ ] The exact PlatformIO executable and Core version were recorded.
- [ ] Effective platform, framework, and library versions were recorded.
- [ ] Pillow and NumPy imported in PlatformIO's own Python interpreter.
- [ ] Node, npm, and npx were available.
- [ ] Current raw sprite paths, properties, count, and ARGB8888 byte total were recorded.
- [ ] The configured release build exited successfully.
- [ ] The build log proves portal, four-font, and sprite generation ran.
- [ ] The build log does not contain `Sprite conversion skipped`.
- [ ] `firmware.bin` and `firmware.elf` exist and are non-empty.
- [ ] PlatformIO RAM and Flash used/max lines were copied exactly.
- [ ] Exact `firmware.bin` byte size and SHA-256 were recorded.
- [ ] Slot usage and headroom were calculated against 2,031,616 bytes.
- [ ] Projected headroom after the 204,800-byte sprite budget was recorded.
- [ ] Existing sprite symbol bytes were checked from ELF/map where available.
- [ ] Every post-build generated or unrelated diff was reviewed and classified.
- [ ] The resolved LVGL version mismatch has an explicit decision or blocker.
- [ ] No native tests, hardware tests, upload, OTA test, or NVS test was falsely reported.
- [ ] The baseline report ends with an unambiguous PASS or BLOCKED status.

## Exit criteria

Phase 1 is **PASS** only when all of the following hold:

1. the unchanged pre-Tamagotchi firmware builds successfully in `adafruit_feather_esp32s3_reversetft`;
2. all generators demonstrably ran with their prerequisites available;
3. no generated diff remains unexplained;
4. the firmware fits the `0x1F0000` application slot;
5. exact RAM, Flash, binary, hash, and headroom evidence is stored in `phase1-baseline.md`;
6. pre-existing worktree changes remain intact;
7. the effective LVGL version is either aligned with the documented target or recorded as a named blocker for API-sensitive work.

If any item fails, label the report **BLOCKED**, describe the smallest required remediation, and do not claim the baseline is known-good.

## Handoff to later phases

- Phase 2 uses the exact commit and LVGL/dependency decision as the contract context for model types and constants.
- Phases 3–6 use the recorded 153,600-byte existing sprite baseline, exact OTA headroom, 181,248-byte planned pixel payload, and 204,800-byte sprite budget.
- Phase 6 must rebuild and report deltas against both `firmware.bin` and ELF/map sprite symbols from this phase.
- Phases 7–13 must not change baseline numbers retroactively; if dependencies or unrelated assets change, create a new labeled baseline and explain the delta.
- Phase 14 repeats the same build and measurement method and compares final results to `_ai/plans/tamagochi/phase1-baseline.md`.
- Hardware validation remains for the phases that introduce render, input, persistence, clock, removal, sleep, and OTA behavior. Phase 1 supplies no hardware-behavior evidence.
