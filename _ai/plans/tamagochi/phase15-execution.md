# Tamagotchi Phase 15 execution record

## Result

The font-generation remediation is implemented and verified. The release
decision remains `BLOCKED`: no designated Feather, fixtures, or authorized
hardware evidence was available for H01--H13. The generator gate is green, but
the clean firmware images retain an explained build-time timestamp difference.

The supplied plan is named `phase15-remediation-plan.md` but its heading says
Phase 14. This record treats it as Phase 15 remediation of the Phase 14
findings.

## Source identity and environment

- Repository `HEAD`: `7ad87998ca6bcbbdfad33f1c5f1989c99ddb07ae`
- The remediation and its execution record are committed on `main`; the
  working tree is clean.
- PlatformIO Core: 6.2.0
- Platform: Espressif 32 54.3.20
- Arduino framework: 3.2.0
- Python: 3.12.3
- Node.js: v22.14.0
- npm: 10.9.2

No credentials, API keys, raw NVS records, or private URLs were included in
the retained logs or this record.

## Remediation implemented

`ttf2c.py` now:

- executes when PlatformIO imports the extra script through `Import("env")`;
- uses `$PROJECT_DIR` under PlatformIO and the script directory for direct
  invocation;
- runs npm and `lv_font_conv` with the project directory as the working
  directory while passing project-relative paths to keep generated metadata
  deterministic across checkout locations;
- validates all four required source fonts before conversion;
- fails non-zero on a missing source, failed converter invocation, or fewer
  than four successful conversions; and
- prints `Successfully processed 4 of 4 fonts` followed by
  `All fonts were successfully converted to LVGL format!` on success.

The existing font names, sizes, ranges, LVGL format, 4-bit output, and
no-compression settings were preserved. `tools/test_ttf2c.py` covers the
PlatformIO import path, direct invocation, missing input, failed conversion,
and project-relative generated metadata. `node_modules/` is ignored because
the existing `npm install --no-save` prerequisite creates that local cache.

## Generator and host verification

```text
python3 tools/test_ttf2c.py
Ran 4 tests
OK
```

The test includes negative cases for a missing font and a forced converter
failure; both exit non-zero. The PlatformIO import-context success path was
also exercised by the test.

The production build log is retained at:

- `/tmp/deskhog-tamagotchi-font-build-relative.log`
- `/tmp/deskhog-tamagotchi-clean-build-1.log`
- `/tmp/deskhog-tamagotchi-clean-build-2.log`
- `/tmp/deskhog-tamagotchi-clean-build-3.log`

The logs contain the portal success output, four font conversions, the
`4 of 4` success line, the all-fonts success line, and 51 processed sprites.
The tracked generated outputs had no diff after regeneration. All four font
C/H pairs, `fonts.h`, `include/html_portal.h`, and the 51 sprite inputs'
generated outputs exist.

## Clean build and artifact evidence

The configured environment completed three clean verification builds while
investigating reproducibility. All three reported the same resource usage:

| Measure | Result |
|---|---:|
| RAM | 81,492 / 327,680 bytes (24.9%) |
| Flash / OTA application slot | 1,988,462 / 2,031,616 bytes (97.9%) |
| OTA-slot headroom | 43,154 bytes |
| `firmware.bin` size | 1,989,120 bytes |
| Generated-asset hash set | identical across clean builds |
| `firmware.map` SHA-256 | `1341540ab0a324cb5b37490c74480d3f6abcb220b9414f38f65c31eac38ab516` |

Two final-source candidates were:

| Build | `firmware.bin` SHA-256 | `firmware.elf` SHA-256 |
|---|---|---|
| Clean build 2 | `64fe6eb3a81ad12dc9e43737e829d4ceaad0183cb6d49a23e969bba3aef77fbb` | `c85363af157d8b169d6bb93f127e3ff1d8dd52c23d3ea602f3820607f5e49faf` |
| Clean build 3 | `8b741472baeedcf7640f0bcbd8b70ff910aafddb844916db935bc932678535b9` | `3f6908f1d8c4f2480e3adae88409abbfbd9fd3fffef6da41548e25dfd9a7d3d1` |

The difference is explained. Generated inputs, the map, resource usage, and
image size match. After excluding the ESP image digest at `0xb0`, the first
meaningful firmware difference is the embedded `__TIME__` string at image
offset 470581 (`08:25:59` versus `08:29:21`); the later digest differences are
derived from that content. The framework source
`framework-arduinoespressif32/cores/esp32/chip-debug-report.cpp` embeds
`__DATE__` and `__TIME__`. This framework diagnostic timestamp was not changed
as part of the font remediation.

The historical Phase 13 and Phase 14 hashes therefore remain historical
evidence. The current clean-build candidates supersede them for the current
checkout. The latest candidate is tied to the committed source, but a
maintainer must still select the exact artifact hash and accept the documented
build metadata policy before release.

## Feather validation status

No board, stable-power setup, fixtures, serial evidence, or destructive
operation authorization was available. No flashing, erase, reset/power-loss,
deep-sleep, or OTA operation was performed.

| Case | Status | Evidence |
|---|---|---|
| H01 | BLOCKED | No designated Feather or clean-NVS fixture |
| H02 | BLOCKED | No designated Feather or input fixture |
| H03 | BLOCKED | No designated Feather or persistence fixture |
| H04 | BLOCKED | No designated Feather or evolution fixture |
| H05 | BLOCKED | No designated Feather or physical button matrix |
| H06 | BLOCKED | No authorized sleep/reset/power-loss setup |
| H07 | BLOCKED | No authorized existing-NVS/portal fixture |
| H08 | BLOCKED | No clock/network fixture |
| H09 | BLOCKED | No authorized OTA-restart setup |
| H10 | BLOCKED | No board or 20-cycle heap/PSRAM telemetry run |
| H11 | BLOCKED | No authorized AP/portal/OTA scenario |
| H12 | BLOCKED | No TFT visual inspection evidence |
| H13 | BLOCKED | No authorized final flash/OTA recovery run |

## Release decision

- Font generation gate: `PASS`.
- Clean production build and OTA capacity: `PASS`, with the existing
  `src_filter` deprecation warning and timestamp nondeterminism recorded.
- Native tests: not run; no native PlatformIO test environment is enabled.
- H01--H13: `BLOCKED`.
- Overall release: `BLOCKED`, not `PASS`.

Before release, select one exact artifact hash and execute H01--H13 against
that hash with redacted evidence.
