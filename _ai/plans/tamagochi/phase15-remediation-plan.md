# Tamagotchi Phase 14 remediation plan

## Current assessment

The project is not release-ready yet.

The Phase 14 report was run against `cb7fe3a` before the Phase 13 execution
record was added. The Phase 13 record now exists in the current workspace, so
the report's missing-record statement is historically accurate for that run but
stale for the current checkout. It does not remove the remaining release gates:

1. The configured font generator is still not executed by PlatformIO.
2. No Feather validation case H01--H13 has been run.
3. The Phase 13 and Phase 14 reports identify different firmware artifacts for
   the same historical phase-13 revision, and the difference has not been
   explained.
4. The current `main` branch contains changes after `cb7fe3a`, so neither
   historical binary is an artifact for the current release candidate.

This document records the verified findings and the smallest safe path to a new
release decision. It does not claim that unrun hardware cases passed.

## Evidence reviewed

- `_ai/plans/tamagochi/phase13-execution.md`
- `_ai/plans/tamagochi/phase14-validation-report.md`
- `_ai/plans/tamagochi/implementation-phase13.md`
- `platformio.ini`, `ttf2c.py`, `png2c.py`, and the generated-asset guide
- Phase 13 implementation in `src/ui/TamagotchiCard.cpp`,
  `src/ui/CardController.cpp`, `src/main.cpp`, and `src/ui/InputHandler.h`
- Git history and current worktree identity

## Verified findings

| ID | Finding | Status | Consequence |
|---|---|---|---|
| F01 | `ttf2c.py` is configured as a PlatformIO `pre:` extra script, but only calls `main()` behind the standalone `__main__` guard. PlatformIO imports the script, so the font conversion body is skipped. | Confirmed defect | S04 and AC-13 fail even though stale generated font C files compile. |
| F02 | No designated board, port, stable-power setup, fixtures, or destructive-operation authorization was available. H01--H13 were not run. | Confirmed validation blocker | Persistence, input, visual, sleep, reset, OTA, and heap claims remain unverified. |
| F03 | Phase 13 records `firmware.bin` SHA-256 `3e8985...`, 1,989,120 bytes, while Phase 14 records SHA-256 `19d3a1...`, 1,990,256 bytes. Both reports identify `cb7fe3a` as the source revision. | Confirmed traceability gap; root cause unknown | Do not select either binary for release until a clean reproducible build explains the difference. |
| F04 | Current `main` is merge commit `9ef798c` and includes `c6a8181` changes beyond `cb7fe3a` in `ConfigManager`, `CaptivePortal`, and `JsonEnvelope.h`. | Confirmed scope drift | A new build must use and identify the actual release commit. Historical phase-14 evidence cannot certify current `main`. |
| F05 | Phase 13 reports a successful host build, but the build did not prove that all three configured generators ran; Phase 14 specifically found no font-generator output. | Incomplete evidence | “Build succeeded” must not be treated as “generated assets were regenerated.” |
| F06 | The image has only about 41--43 KiB of reported OTA headroom and uses roughly 98% of the application slot. | Confirmed release risk | Re-measure after repairing font generation and require explicit maintainer acceptance. |

### What was verified as implemented

The source review found no additional Phase 13 contract violation beyond the
unverified runtime behavior:

- `TamagotchiCard.cpp` has one direct state-store save call and one
  `markPersisted()` call, both in `saveWithClockBaseline()`.
- `prepareForSleep()` performs no LVGL work and is reached through the
  `dynamicCards` fan-out on the LVGL task.
- `prepareForRemoval()` saves before animation/pointer cleanup and leaves root
  deletion to `CardNavigationStack`.
- No native tests are configured, so no native-test pass is claimed.

These are static findings only. They do not prove NVS durability, task timing,
pointer lifetime under load, physical button behavior, display layout, or OTA
restart behavior.

## Remediation 1: repair the font generator build gate

### Root cause

`platformio.ini` loads `ttf2c.py` with:

```ini
extra_scripts =
    pre:${PROJECT_DIR}/ttf2c.py
```

But `ttf2c.py` ends with:

```python
if __name__ == "__main__":
    main()
```

That works only when the file is launched as a program. It is inert when
PlatformIO imports it as an extra script. `png2c.py` already demonstrates the
required import-context pattern.

### Required code change

Update `ttf2c.py` so the same conversion entry point runs when PlatformIO loads
the file and when a developer invokes it directly. Keep the generated files
source-owned; do not hand-edit `include/fonts/`.

The implementation should:

1. Detect the PlatformIO/SCons context with `Import("env")` and invoke the
   converter during import.
2. Use the project directory (or paths derived from `__file__`) instead of
   depending on an incidental current working directory.
3. Preserve the four existing font definitions, ranges, sizes, LVGL format,
   4-bit output, and no-compression settings.
4. Treat a missing font, failed `npx lv_font_conv`, or fewer than four
   successful conversions as a non-zero build failure. A successful build must
   not silently compile stale generated C files.
5. Print and retain an unambiguous success line:
   `Successfully processed 4 of 4 fonts`, followed by the existing all-fonts
   success message.

Do not change the LVGL version, font ranges, or generated symbols as part of
this repair unless the clean conversion exposes a separate reproducible issue.

### Verification gate

After the script change:

1. Save SHA-256 hashes and the working-tree status of all tracked generated
   portal, font, and sprite outputs.
2. Run the configured environment from a clean build directory:

   ```sh
   ~/.platformio/penv/bin/pio run -t clean -e adafruit_feather_esp32s3_reversetft
   ~/.platformio/penv/bin/pio run -e adafruit_feather_esp32s3_reversetft \
     2>&1 | tee /tmp/deskhog-tamagotchi-font-build.log
   ```

3. Require the log to contain all of:

   - `TTF to LVGL Font Converter`
   - four successful font conversions
   - `Successfully processed 4 of 4 fonts`
   - `All fonts were successfully converted to LVGL format!`
   - the portal and sprite generator success lines

4. Validate that all four expected font C/H pairs and `fonts.h` exist, review
   the generated diff, and compare the output hashes with the source inputs.
5. Run one negative check with a missing or failing font prerequisite and verify
   that the PlatformIO build fails instead of compiling stale output.

S04 is not repaired by seeing unchanged font hashes: unchanged output is
acceptable only after the log proves that the source-to-output conversion ran.

## Remediation 2: establish one authoritative release artifact

The two historical reports disagree on the firmware size and SHA-256 even
though they name the same phase-13 source revision. The reports do not contain
enough evidence to identify whether the cause was build-tree state, generated
inputs, dependency resolution, compiler nondeterminism, or another environment
difference. Do not guess; reproduce the boundary first.

### Required diagnostic sequence

1. Choose the release source revision explicitly. The recommended candidate is
   the current `main` commit after all intended fixes are committed. If the
   release is intentionally based on `cb7fe3a`, say so and exclude later
   changes explicitly.
2. Build from a clean worktree at that exact commit. Record:
   `git rev-parse HEAD`, `git status --short`, PlatformIO Core, platform,
   framework, library versions, Python/npm versions, and all generator output.
3. Run two clean builds from the same source tree and compare:
   `firmware.bin`, `firmware.elf`, generated-file hashes, map-file sections,
   and final PlatformIO size lines.
4. If the binaries differ, identify the first differing generated file or map
   section before proceeding. Check for timestamps, build metadata, dependency
   drift, and untracked generated changes. The release must not rely on an
   unexplained binary difference.
5. Publish one final artifact manifest containing the exact source commit,
   firmware SHA-256, byte count, RAM/flash usage, OTA slot size, and retained
   build log path.

The current checkout contains `c6a8181` changes after `cb7fe3a`; those changes
may be valid, but they make the old phase-14 binary historical evidence rather
than a current-main release artifact. Any source, configuration, dependency,
or generated-asset change restarts S01--S04.

## Remediation 3: run the missing Feather validation

Hardware validation remains blocked, not passed. Before flashing, designate:

- one stable-power Adafruit ESP32-S3 Reverse TFT Feather and its board revision;
- a verified USB port and serial-monitor settings;
- a safe `CLEAN` fixture and a behavior-based, redacted `EXISTING` fixture;
- non-secret clock/network fixtures, including pending-clock and backward-clock
  cases;
- explicit authorization for erase, reset/power-loss, deep sleep, serial flash,
  and OTA operations.

Run S04 first and flash only the exact artifact recorded in the new manifest.
Collect serial logs, before/after state observations, photos for visual cases,
and heap/PSRAM measurements in a timestamped evidence directory. Redact Wi-Fi
passwords, API keys, raw NVS records, and signed/private URLs.

The minimum execution order is:

| Cases | Required coverage |
|---|---|
| H01, H02, H03 | Clean boot, hatch, all five actions, refusals, mess/sickness thresholds, and reset-after-action durability. |
| H04, H05 | Child-to-Adult evolution, visual/action return paths, and the physical three-button matrix. |
| H06, H09 | Center+down deep sleep from egg/normal/selector modes, removal/reconciliation, reset, power-loss, and OTA-restart retention. |
| H07 | Existing-NVS boot, portal add/remove/reorder/re-add, and persistence of the independent pet record. |
| H08 | Valid clock, pending clock, epoch `0`, backward clock, once-only catch-up, and no negative/duplicated simulation. |
| H10 | Twenty production rebuild/reconciliation cycles with recorded heap/PSRAM telemetry. |
| H11 | AP/portal, insight activity, and safe OTA scenario while the Tamagotchi card is configured. |
| H12 | Actual TFT inspection of every sprite group, clipping, alpha/color, timing, bars, and status priority. |
| H13 | Final serial flash and OTA update using the exact repaired production image, with recovery evidence. |

Mark a case `PASS` only with its required device evidence. A static audit or
successful host build may support a case but cannot replace it.

## Remediation 4: update the release decision

Keep the existing Phase 14 report as the historical record, but use a new
validation addendum or a replacement report for the repaired artifact. Correct
the S01 wording to acknowledge that `phase13-execution.md` now exists, while
retaining the hardware blockers. The final report must include:

- the repaired generator log and reviewed generated diff;
- one authoritative artifact hash tied to the final source commit;
- the post-repair OTA capacity measurement;
- H01--H13 results with evidence paths;
- explicit residual risks and maintainer acceptance, if any.

The release decision can become `PASS` only when the font-generation gate is
green, the final artifact fits the `0x1F0000` application slot, and all required
hardware cases have evidence. Until then, the correct status is `FAIL` for the
font gate and `BLOCKED` for the unrun hardware matrix.

## Completion checklist

- [ ] `ttf2c.py` runs under PlatformIO import and direct invocation.
- [ ] A missing/failed font conversion fails the build.
- [ ] Clean build log proves portal, four-font, and sprite generation.
- [ ] Generated outputs are reviewed; no generated file was hand-edited.
- [ ] Final source commit and binary SHA-256 are uniquely recorded.
- [ ] The phase-13/phase-14 binary discrepancy is explained or superseded by
      two matching clean builds.
- [ ] Post-repair RAM, flash, and OTA headroom are accepted.
- [ ] A designated Feather, fixtures, power, and destructive-operation approval
      are recorded.
- [ ] H01--H13 are executed against the exact final hash.
- [ ] No credentials, API keys, raw NVS records, or private URLs enter the
      evidence or logs.
