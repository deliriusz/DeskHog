# Tamagotchi Phase 6 execution record

## Result

Host validation passed. The physical Feather smoke test remains blocked because a
board was not available in this environment; no upload, OTA operation, or
temporary LVGL smoke path was attempted.

## Conversion environment

| Tool | Measured version |
|---|---|
| PlatformIO Python | 3.12.3 at `/home/deliriusz/.platformio/penv/bin/python` |
| Pillow | 12.1.0 |
| NumPy | 2.4.1 |
| PlatformIO Core | 6.2.0 |

The conversion ran through that interpreter and reported 51 source frames: the
45 Tamagotchi frames in 17 groups plus six walking regression frames.

`tools/validate_tamagotchi_sprites.py` passed after conversion and after the
production build. It verifies the exact source set, dimensions, RGBA mode,
binary alpha, output-name/symbol collisions, descriptors, numeric map bytes,
aggregate arrays/counts, the generated linker root, and walking order.

## Linker retention

The release link uses section garbage collection. Before gameplay consumes the
new groups, that discarded every new descriptor/map even though PlatformIO
compiled each generated C file. `png2c.py` now emits
`deskhog_all_sprite_groups`, a catalog that references every generated group,
and `platformio.ini` roots it with `-Wl,-u,deskhog_all_sprite_groups`. This is
source-generated and preserves the existing BGRA `ARGB8888` pixel/descriptor
contract. It makes the Phase 6 binary-size measurement represent the embedded
assets that later phases will use.

## Determinism and build

Two complete `include/sprites/` SHA-256 inventories, separated by an unchanged
explicit `png2c.py` run, were identical. The production command completed:

```sh
~/.platformio/penv/bin/pio run -e adafruit_feather_esp32s3_reversetft
```

The final log ended with `[SUCCESS]`, compiled every generated Tamagotchi
source, and reported RAM usage of 81,484 / 327,680 bytes and Flash usage of
1,955,894 / 2,031,616 bytes. Existing Flappy, OTA timeout, portal API, and
`src_filter` deprecation warnings remain pre-existing technical debt.

## Firmware and OTA budget

| Measure | Bytes |
|---|---:|
| Phase 1 baseline `firmware.bin` | 1,773,984 |
| Phase 6 `firmware.bin` | 1,956,560 |
| Actual firmware delta | 182,576 |
| OTA application slot | 2,031,616 |
| Post-build OTA headroom | 75,056 |
| Baseline OTA headroom | 257,632 |
| Raw Tamagotchi maps (44 × 4096 + 1 × 1024) | 181,248 |
| Sprite budget | 204,800 |
| Budget slack before descriptor/linker overhead | 23,552 |
| Actual delta slack against sprite budget | 22,224 |

`firmware.bin` SHA-256 is
`35101c0028e4a9d66fbe0d73220c4a380e7e2bbf78b5b14dcccc832c2c674719`.
The final ELF contains exactly 45 `sprite_tamagotchi_*_map` symbols totaling
181,248 bytes. The binary fits the OTA slot and its isolated sprite delta is
within the 200 KiB budget, but later model/UI/persistence phases must preserve
and remeasure the remaining 75,056-byte margin.

## Phase 7/10 handoff

Include `sprites/sprites.h`; use the generated pointer arrays and `uint8_t`
counts directly, never copy pixel data or edit generated C/H files. The group
symbols are `tamagotchi_egg_sprites`, `tamagotchi_child_idle_sprites`,
`tamagotchi_adult_idle_sprites`, `tamagotchi_child_feed_sprites`,
`tamagotchi_adult_feed_sprites`, `tamagotchi_child_play_sprites`,
`tamagotchi_adult_play_sprites`, `tamagotchi_child_clean_sprites`,
`tamagotchi_adult_clean_sprites`, `tamagotchi_child_rest_sprites`,
`tamagotchi_adult_rest_sprites`, `tamagotchi_child_sick_sprites`,
`tamagotchi_adult_sick_sprites`, `tamagotchi_child_doctor_sprites`,
`tamagotchi_adult_doctor_sprites`, `tamagotchi_evolve_sprites`, and
`tamagotchi_mess_sprites`.

Frames remain lexicographically ordered `_00.._NN`; pet frames are natively
32×32 and should use LVGL scale `512` for a 2×, approximately 64×64 display.
The mess frame is natively 16×16.

## Remaining physical gate

On an Adafruit ESP32-S3 Reverse TFT Feather, upload only this slot-fitting
production firmware and show `sprite_tamagotchi_child_idle_00` from the
LVGL-owning task at scale `512`. Record color order, transparency, 2× size,
ground/center alignment, and clipping. Until that observation is captured,
Phase 6 is `HOST PASS — HARDWARE SMOKE BLOCKED` rather than fully complete.
