# Tamagotchi pre-feature baseline

## Run identity

- Timestamp: 2026-09-15 10:10:34 CEST (UTC+02:00), refreshed after the LVGL pin.
- Repository: `/home/deliriusz/git/DeskHog`
- Branch and revision: `main` at `447587fde02b9593f33c03dcbdf5ed65d4114de0`
- Commit: `2026-09-09T17:05:11+02:00 detailed implementation plans`
- Firmware version macro: `CURRENT_FIRMWARE_VERSION="0.1.5"`

### Pre-existing dirty paths

Before this phase ran, the only dirty path was `.vscode/extensions.json`.  It changes the
`unwantedRecommendations` list to add `pioarduino.pioarduino-ide`; this is a pre-existing,
user-owned change and was preserved.  No generated asset path was dirty before the build.

The durable report and the exact LVGL pin in `platformio.ini` are the expected Phase 1 changes.
No Tamagotchi source, card, runtime asset, portal source, or generated asset was added or edited.

## Configuration and build toolchain

The configured environment is `adafruit_feather_esp32s3_reversetft`, with `build_type =
release`, Arduino, and `partitions.csv`.  LVGL is pinned exactly to `9.2.2`.  It declares the
three pre-build scripts, in order:
`htmlconvert.py`, `ttf2c.py`, and `png2c.py`.

`pio` was not on `PATH`, so this run used:

```text
/home/deliriusz/.platformio/penv/bin/pio
PlatformIO Core, version 6.2.0
```

Resolved package versions:

| Component | Effective version |
|---|---:|
| PIOArduino Espressif32 platform | 54.3.20 |
| Arduino-ESP32 framework | 3.2.0 |
| Arduino framework libraries | 5.4.0+sha.2f7dcd862a |
| Xtensa ESP toolchain | 14.2.0+20241119 |
| LVGL | 9.2.2 |
| ArduinoJson | 6.21.5 |
| Bounce2 | 2.72.0 |
| ESPAsyncWebServer | 3.8.0 |
| AsyncTCP | 3.4.10 |
| FastLED | 3.10.3 |
| Adafruit ST7735/ST7789 | 1.11.0 |

Generator prerequisites were checked in PlatformIO's own interpreter:

```text
/home/deliriusz/.platformio/penv/bin/python
Pillow 12.1.0
NumPy 2.4.1
Node v22.14.0
npm 10.9.2
npx 10.9.2
```

## Generator and asset evidence

The six checked source PNGs are listed below.  Each was `80x80`, source mode `RGBA`, had
alpha minimum/maximum `0`/`255`, used only binary alpha, and costs 25,600 bytes as ARGB8888.

| Source sprite | ARGB8888 bytes |
|---|---:|
| `raw-png/walking/Normal-Walking_01.png` | 25,600 |
| `raw-png/walking/Normal-Walking_02.png` | 25,600 |
| `raw-png/walking/Normal-Walking_03.png` | 25,600 |
| `raw-png/walking/Normal-Walking_04.png` | 25,600 |
| `raw-png/walking/Normal-Walking_05.png` | 25,600 |
| `raw-png/walking/Normal-Walking_06.png` | 25,600 |
| **Total** | **153,600** |

The generated walking group is ordered `_01` through `_06`.  Each descriptor remains
`LV_COLOR_FORMAT_ARGB8888`, `80x80`, with `data_size = 25600`; the flat sprite output contains
only the expected six C/header pairs plus `sprites.c` and `sprites.h` (no stale per-frame files).

Pre- and post-build SHA-256 inventories of all 24 tracked generated portal/font/sprite files
were identical.  Their inventory digest was
`268982965f5bd2f50c52196f804bf5c38a3a709e7aed967f1e3aeb05c237d69d`.
The evidence files are retained locally at:

```text
/tmp/deskhog-tamagotchi-phase1/generated-pre-lvgl-9.2.2.sha256
/tmp/deskhog-tamagotchi-phase1/generated-post-lvgl-9.2.2.sha256
```

The build log does prove that two generators executed:

```text
DEBUG: Successfully generated include/html_portal.h
Successfully processed 6 sprites:
  - walking: 6 sprites
```

It does **not** contain `Sprite conversion skipped`.

At the project owner's direction, execution and `Successfully processed 4 of 4 fonts` evidence
for `ttf2c.py` were not checked in this refreshed baseline.  The existing generated font C files
were compiled, but this report makes no claim that they were regenerated.  This is an explicit
Phase 1 verification waiver, not a blocker.

## Release build result

The authoritative configured build command was:

```sh
/home/deliriusz/.platformio/penv/bin/pio run -e adafruit_feather_esp32s3_reversetft \
  2>&1 | tee /tmp/deskhog-tamagotchi-phase1/build-lvgl-9.2.2.log
```

It exited `0` and ended with:

```text
========================= [SUCCESS] Took 20.61 seconds =========================
```

The full post-pin recompilation completed before this final incremental verification; the latter
is the authoritative logged command because it also exits successfully and regenerates the
portal/sprite outputs.  Neither run changed generated files.

PlatformIO reported:

```text
RAM:   [==        ]  24.8% (used 81308 bytes from 327680 bytes)
Flash: [========= ]  87.3% (used 1773318 bytes from 2031616 bytes)
```

The required non-empty artifacts exist:

| Artifact | Bytes |
|---|---:|
| `firmware.bin` | 1,773,984 |
| `firmware.elf` | 27,148,452 |
| `firmware.map` | 19,324,068 |
| `partitions.bin` | 3,072 |

`firmware.bin` SHA-256:

```text
fed60891714155256b4f8e61ff460ee5245eef49a021bc261d4c965cb29059bf
```

`xtensa-esp32s3-elf-size -A` reports, among other relevant sections, `.flash.text =
1,055,524` and `.flash.rodata = 586,816` bytes.  The ELF symbol evidence reports six
`sprite_Normal_Walking_*_map` symbols of `0x6400` (25,600) bytes each: exactly 153,600 bytes of
sprite pixels.  It also reports six 24-byte descriptors, a 24-byte `walking_sprites` pointer
array, and a one-byte count (153,769 selected symbol bytes before linker padding/alignment).

## OTA capacity

`partitions.csv` defines two OTA application slots:

```text
OTA slot hex:     0x1F0000
OTA slot decimal: 2,031,616 bytes
```

| Measure | Value |
|---|---:|
| `firmwareBinBytes` | 1,773,984 |
| `slotHeadroomBytes` | 257,632 |
| `slotUsedPercent` | 87.318863% |
| Planned Tamagotchi sprite budget | 204,800 |
| `headroomAfterSpriteBudget` | 52,832 |
| Planned raw Tamagotchi pixels (44 × 32×32 + 1 × 16×16) | 181,248 |

The baseline firmware fits its OTA slot and can accommodate the 200 KiB sprite budget, but only
52,832 bytes remain after that allowance.  This is not sufficient evidence that later model,
UI, persistence, and descriptor growth will fit; those phases must rebuild and measure deltas.

## Generated diffs and warnings

Post-build `git diff -- include/html_portal.h include/fonts include/sprites` was empty, as was
the generated-file hash diff.  The only worktree change before this report was the preserved
user-owned `.vscode/extensions.json` modification.  No `node_modules`, npm metadata, or
unexpected generated outputs appeared in the worktree.

Warnings to retain as technical debt, not Phase 1 fixes:

- PlatformIO deprecates `src_filter` in favor of `build_src_filter`.
- The current firmware emits existing LVGL enum-conversion, OTA timeout-overflow, and
  ESPAsyncWebServer API-deprecation warnings.

## LVGL version decision

The project owner selected LVGL 9.2.2.  `platformio.ini` now pins `lvgl/lvgl @ 9.2.2`, and the
fresh resolved build reports `lvgl @ 9.2.2`.  API-sensitive later phases may use LVGL 9.2.2 as
their validation contract.

## Warnings, blockers, and next action

**Accepted exception:** Font-generator execution evidence was intentionally skipped at the
project owner's direction.  Later font-asset work must restore generator verification before it
claims regenerated font output.

No upload, hardware validation, native test, OTA operation, or NVS change was performed.

## Phase status: PASS (with accepted font-generator verification waiver)

The LVGL 9.2.2 firmware builds successfully, fits the OTA slot, and portal/sprite generation is
verified.  The source revision, effective dependencies, capacity measurements, generated diffs,
and accepted verification waiver are recorded above.
