# Tamagotchi Phase 14 validation report

## Release decision

**FAIL**

The production image for `cb7fe3ae5fd7a5f58714c93e4eddecddbf719320`
compiled and fits an OTA application slot, but S04 failed: the configured
PlatformIO build did not run the font generator. The build log contains no
`ttf2c.py` output, no `Successfully processed 4 of 4 fonts`, and the script
only calls `main()` under `if __name__ == "__main__"`, so it is inert when
loaded as a PlatformIO extra script.

This is independently not release-ready because the required Phase 13 execution
evidence is absent and no designated Feather was available for H01--H13. Those
items remain blockers; they do not downgrade the observed S04 failure to a pass.
No production artifact was uploaded, no NVS was read, written, or erased, and no
OTA operation was attempted.

## Run identity and evidence

| Field | Value |
| --- | --- |
| Timestamp | 2026-09-17T10:21:00+02:00 (CEST) |
| Validation worktree | `/tmp/deskhog-tamagotchi-phase14` |
| Branch / commit | `phase14-validation` / `cb7fe3ae5fd7a5f58714c93e4eddecddbf719320` |
| Last commit | `cb7fe3a phase 13` (2026-09-17T08:47:40+02:00) |
| Evidence directory | `/tmp/deskhog-tamagotchi-phase14-rm4Uwx` |
| Board / port / power | Not designated; no hardware operation authorized or performed |
| Secrets reviewed/redacted | Yes; evidence contains source/build metadata and no credentials, tokens, raw NVS records, or portal request bodies |

The validation worktree was clean before this report. The source checkout at
`/home/deliriusz/git/DeskHog` had pre-existing, excluded work in
`docs/captive-portal.md`, `docs/configuration-and-state.md`,
`src/ConfigManager.cpp`, `src/ui/CaptivePortal.cpp`, and the new
`src/config/JsonEnvelope.h`. It was preserved and was not part of the built
revision.

Relevant retained evidence includes:

- `build-production-final.log`, `artifact-sha256.txt`, `artifact-bytes.txt`,
  `firmware-sections.txt`, and `tamagotchi-map-symbols.txt`;
- `generated-pre.sha256`, `generated-post.sha256`,
  `generated-inventory.diff`, and `walking-regression.diff`;
- `process-sprites-check.log` command context, `validate-sprites-postbuild.log`,
  and `static-audit-search.txt`;
- `build-comparison-baseline-lvgl922.log` and
  `comparison-baseline-lvgl922-summary.txt`;
- the exact unuploaded release image, `firmware-cb7fe3a.bin`, SHA-256
  `19d3a146fc6a45f3bda717cadabbe3bf4ca441a595ec1b54628368c029b34c68`.

## S01 -- preflight and evidence identity

**Result: BLOCKED.** The source revision, immutable build identity, and
dependency observations are present, but there is no Phase 13 execution record.
The user explicitly directed this run to continue without it; that direction
does not make the missing evidence pass.

| Requirement | Observation |
| --- | --- |
| Phase 1 baseline | `phase1-baseline.md` records `447587f`, a 1,773,984-byte image, and a documented LVGL 9.2.2 decision. |
| Phase 6 evidence | `phase6-execution.md` records exact 45-map total of 181,248 bytes and a hardware-smoke blocker. |
| Phase 7 evidence | `phase7-execution.md` records a temporary C++17 model harness. That harness was not retained in this environment, so it was not rerun. |
| Phase 13 evidence | No `phase13-execution.md` or equivalent execution evidence was found. The Phase 13 plan exists but is not execution proof. |
| Final dependency resolution | PlatformIO Core 6.2.0; PIOArduino Espressif32 54.3.20; Arduino 3.2.0; LVGL 9.2.2; ArduinoJson 6.21.6; Bounce2 2.72.0; AsyncTCP 3.5.0; ESPAsyncWebServer 3.8.0; FastLED 3.10.3; Adafruit ST7735/ST7789 1.11.0. |
| Generator prerequisites | PlatformIO Python 3.12.3 with Pillow 12.1.0 and NumPy 2.4.1. |
| LVGL contract | Current `platformio.ini` pins LVGL exactly to 9.2.2 and the final build resolved 9.2.2. |
| Test board / fixtures | Absent. `CLEAN` and `EXISTING` fixtures, port, board revision, power source, credential authorization, and erase/OTA approval were not supplied. |

The source commit cited by the historical baseline used `lvgl @ ^9.2.2`, which
currently resolves to 9.6.0 and produced an incomparable 1,825,424-byte image.
To avoid using that value, this run built a disposable Phase 1 worktree with
only its LVGL constraint pinned to 9.2.2. It resolved the same relevant
libraries as the final build and produced a labeled comparison baseline:

| Same-toolchain comparison baseline | Result |
| --- | ---: |
| Commit / local comparison-only change | `447587f`; LVGL constraint pinned from `^9.2.2` to `9.2.2` only in `/tmp/deskhog-tamagotchi-phase14-baseline` |
| `firmware.bin` bytes | 1,775,504 |
| `firmware.bin` SHA-256 | `c753ed639138efae59054cdf2503de4cce3aea1913c1004fa256de4d0bfb52ca` |
| PlatformIO RAM / flash | 81,308 / 327,680; 1,774,842 / 2,031,616 |
| Final direct-binary delta | 214,752 bytes |

The initial pre-feature report's historical byte figure remains recorded, but
the comparison baseline above is the only delta used in this report.

## S02 -- generated assets and deterministic source review

**Result: PASS (host/static).**

| Check | Observed result |
| --- | --- |
| Phase 5 deterministic processor | `process_tamagotchi_sprites.py check` reproduced 17 groups and 45 PNGs: 44 at 32x32, one at 16x16, exactly 181,248 ARGB8888 bytes. |
| Phase 6 descriptor validator, after build | Passed: 45 descriptors, 44 at 4,096 bytes and one at 1,024 bytes, total 181,248 bytes. |
| Runtime group / frame inventory | Exact: egg 2; Child idle/feed/play/clean/doctor 3 each; Child rest/sick 2 each; Adult idle/feed/play/clean/doctor 3 each; Adult rest/sick 2 each; evolve 4; mess 1. |
| Symbols and ordering | Validator confirmed globally unique normalized symbols, expected group order, matching descriptors, and no stale generated outputs. |
| Generated hash inventory | Pre- and post-build inventory digest were both `a8a4f378b81ce8e6ded5e08b7ec6b940e1ba81c7506ef45eb422158a143dc74c`; the recorded diff is empty. |
| Existing walking frames | The direct diff from `447587f` for `raw-png/walking` and all six walking descriptors is empty. |
| Embedded references | No reference strip or contact sheet appears below `raw-png/`; source strips and review assets remain under `_ai/art/tamagotchi/`. |

The final build log explicitly says `Successfully processed 51 sprites` and
does not contain `Sprite conversion skipped`. Font generation is excluded
from this S02 result and fails S04 below.

## S03 -- static implementation audit

**Result: PASS (source-only; not a substitute for Feather verification).**

The audit searched the implementation and inspected the card, store, clock,
controller, input, and startup paths. The main observed contracts are:

- `CardType::TAMAGOTCHI` has the stable persisted spelling `"TAMAGOTCHI"`,
  exact non-fallback parsing, a portal-visible singleton definition, and a
  `allowMultiple == false` factory.
- The factory receives references to one startup-created
  `TamagotchiStateStore` and one `ClockService`. Reconciliation stable-sorts
  legacy configurations and keeps the first singleton without rewriting NVS.
- `configTime()` occurs only in `ClockService`. There is no Tamagotchi task,
  Tamagotchi event subscription, or OTA/time reference to a removable card.
- The pure model includes no Arduino, LVGL, NVS, network, task, logging,
  randomness, or allocation dependency. The card model actions, persistence,
  and LVGL mutations are reached through the active-card UI path.
- `uint32_t` subtraction/deadline comparison is used for same-boot timing;
  only epochs are persisted. The offline path subtracts same-boot seconds,
  applies once, and caps at 604,800 seconds.
- `saveWithClockBaseline()` is the card's sole state-store save route. It
  forces saves for hatch, applied actions, important transitions, removal, and
  sleep; passive dirty state has a five-minute checkpoint. Failed saves retain
  dirty state and render `SAVE!`.
- `prepareForRemoval()` is idempotent, stops the animation, clears object
  pointers, and leaves deletion of the stack-owned root to
  `CardNavigationStack::removeCard()`. The global sleep chord calls
  `CardController::prepareForSleep()` before deep sleep.
- The recurring card update path uses fixed fields and pre-created LVGL objects;
  it does not construct `String`, `std::string`, JSON documents, vectors,
  or new LVGL objects. Tamagotchi store/card logs are categorical and do not
  print raw state or configuration data.

This result cannot prove task scheduling, pointer lifetime under load, NVS
durability, visual clipping, physical input behavior, or any H case.

## S04 -- release build and capacity

**Result: FAIL.**

The canonical configured build command eventually completed with exit status 0:

```sh
/home/deliriusz/.platformio/penv/bin/pio run -e adafruit_feather_esp32s3_reversetft
```

The initial sandbox invocation could not create PlatformIO's shared locks; the
successful retained run used the same command with access to those locks. It
uses release mode, PSRAM flags, and `partitions.csv`.

| Measure | Exact result |
| --- | ---: |
| PlatformIO RAM | 81,492 / 327,680 bytes (24.9%) |
| PlatformIO flash | 1,989,598 / 2,031,616 bytes (97.9%) |
| `firmware.bin` | 1,990,256 bytes |
| `firmware.bin` SHA-256 | `19d3a146fc6a45f3bda717cadabbe3bf4ca441a595ec1b54628368c029b34c68` |
| `firmware.elf` | 28,147,000 bytes |
| `firmware.map` | 19,625,822 bytes |
| `partitions.bin` / `bootloader.bin` | 3,072 / 20,208 bytes |
| OTA application slot | 0x1F0000 = 2,031,616 bytes |
| Direct binary headroom | 41,360 bytes |
| Direct binary slot use | 97.964182% |
| Comparison-baseline delta | 214,752 bytes |
| Tamagotchi maps / budget / slack | 181,248 / 204,800 / 23,552 bytes |

All required binary artifacts exist and the final image is below the slot limit.
The remaining 41,360-byte direct-image headroom is operationally tight and
requires explicit maintainer acceptance even after the build gate is repaired.

The portal generator completed and the sprite generator completed for all 51
PNG inputs. The build nevertheless fails the S04 generator gate:

1. Neither final nor comparison build log contains `TTF to LVGL Font
   Converter`, `Successfully processed 4 of 4 fonts`, or
   `All fonts were successfully converted`.
2. `ttf2c.py` runs `main()` only under `if __name__ == "__main__"`.
   PlatformIO imports the configured `pre:` extra script, so its conversion
   code does not run during the normal build.

The generated font hashes remained stable, but compiling prior generated font C
files is not proof that the source-to-generated font path ran. This is a
release-gate failure owned by the generated-assets/build setup (Phase 1
baseline contract); Phase 14 makes no in-place fix.

## Acceptance and hardware traceability

No acceptance criterion is marked satisfied solely by static code or a host
build. Static observations are noted where available, but every criterion still
needs its stated Feather evidence.

| Trace ID | Current result | This-run evidence / reason |
| --- | --- | --- |
| AC-01 | BLOCKED | Static singleton/factory audit passed; portal/reorder/remove hardware cases H01/H07/H11 were not run. |
| AC-02 | BLOCKED | Static egg/hatch path inspected; H01 and H05 were not run. |
| AC-03 | BLOCKED | Model source and evolution assets inspected; H04/H08/H09 not run. |
| AC-04 | BLOCKED | Static model audit only; H04 not run. |
| AC-05 | BLOCKED | Static selector/action mapping only; H02/H04/H05 not run. |
| AC-06 | BLOCKED | Static input consumption only; H05 not run. |
| AC-07 | BLOCKED | Static chord/sleep hook only; H06/H09 not run. |
| AC-08 | BLOCKED | Static model/action path only; H03 not run. |
| AC-09 | BLOCKED | Static timing audit only; H08 not run. |
| AC-10 | BLOCKED | Static dedicated-store audit only; H01/H07/H09 not run. |
| AC-11 | BLOCKED | Static no-terminal-state audit only; H04/H08 not run. |
| AC-12 | BLOCKED | Descriptor/layout source review only; H10/H11/H12 not run. |
| AC-13 | FAIL | Asset structure and slot arithmetic pass, but the required font-generation proof is absent; H13 not run. |

## H01--H13 Feather matrix

All Feather cases are **BLOCKED / NOT RUN**. S04 failed before upload became
eligible, and no designated board, USB port, non-secret fixtures, stable power,
or destructive-operation authorization was supplied.

| Case | Trace IDs | Result | Owner and smallest next action |
| --- | --- | --- | --- |
| H01 | AC-01, AC-02, AC-10 | BLOCKED | Hardware owner: designate a safe CLEAN board and authorize its erase after S04 passes. |
| H02 | AC-05 | BLOCKED | Hardware owner: supply reviewed non-secret Child fixture after S04 passes. |
| H03 | AC-08 | BLOCKED | Hardware owner: supply reviewed threshold fixtures after S04 passes. |
| H04 | AC-03, AC-04, AC-05, AC-11 | BLOCKED | Hardware owner: supply adult/evolution fixtures after S04 passes. |
| H05 | AC-02, AC-05, AC-06 | BLOCKED | Hardware owner: test physical three-button matrix. |
| H06 | AC-07 | BLOCKED | Hardware owner: approve sleep/chord test and collect before/after proof. |
| H07 | AC-01, AC-10 | BLOCKED | Hardware owner: provide EXISTING board with behavior-based, redacted Wi-Fi/API/insight baseline. |
| H08 | AC-03, AC-09, AC-11 | BLOCKED | Hardware owner: provide controlled clock/network fixture. |
| H09 | AC-03, AC-07, AC-10 | BLOCKED | Hardware owner: approve reset, power-loss, removal, and OTA-restart fixtures. |
| H10 | AC-12 | BLOCKED | Hardware owner: provide production telemetry for twenty rebuilds. |
| H11 | AC-01, AC-12 | BLOCKED | Hardware owner: provide safe AP/insight/OTA scenario and power. |
| H12 | AC-12 | BLOCKED | Hardware owner: inspect all groups and thresholds on the actual TFT. |
| H13 | AC-13 | BLOCKED | Phase 1/build owner must first repair S04; then hardware owner can approve serial flash and OTA. |

### Sprite and action visual checklist

The host descriptor inventory covers every group structurally, but **all visual
observations remain BLOCKED**: Egg; Child/Adult idle; Child/Adult sick; Evolve;
Mess; and Child/Adult Feed, Play, Clean, Rest, and Doctor. No display color,
alpha, clipping, frame timing, bar/status priority, or return-to-idle claim is
made from the build or source inspection.

## NVS, clock, lifecycle, concurrency, and secrets

| Area | Result | Evidence boundary |
| --- | --- | --- |
| Clean/existing NVS and immediate durability | BLOCKED | No designated board and no Phase 13 execution evidence. Source routing was reviewed only. |
| Clock scenarios / same-boot accounting | BLOCKED | Source-only audit of wrap-safe timing and capped once-only catch-up; no real or controlled epoch observation. |
| Removal, deep sleep, reset, and OTA restart | BLOCKED | Source-only audit; no physical transition or retained-state evidence. |
| 20 rebuild heap/PSRAM protocol | BLOCKED | No production telemetry or device sampling. No CSV was fabricated. |
| Five-minute concurrency schedule | BLOCKED | No AP, portal client, insight service, or safe OTA target was supplied. |
| Log/secret audit | PASS (static/build evidence) | Reviewed retained logs contain component/category output, build information, and paths only; no credential, token, raw portal body, raw NVS record, or full pet state was observed. This cannot prove future device logs are clean. |

## Failures, blockers, and required restart point

1. **FAIL -- font generator omitted from PlatformIO build.** Owner: generated
   assets / Phase 1 baseline. Smallest remediation: make the configured
   `ttf2c.py` pre-script invoke the converter in PlatformIO's SCons context,
   then rerun a clean-source S01--S04 build with the exact generator success
   log and reviewed generated diff. Do not hand-edit `include/fonts/`.
2. **BLOCKED -- missing Phase 13 execution evidence.** Owner: Phase 13. Smallest
   remediation: locate or create an evidence record that demonstrates immediate
   saves, checkpoints, removal flush, sleep flush, retained state, and absence
   of background/OTA access to a removable card; do not infer it from the plan.
3. **BLOCKED -- Feather validation unavailable.** Owner: test-board owner.
   Smallest remediation: designate one stable-power Feather, redacted CLEAN and
   EXISTING fixtures, port/board identity, and explicit erase/OTA authorization,
   then run H01--H13 against the exact rebuilt production hash.
4. **RISK -- capacity is tight.** Owner: maintainer. The current image is
   41,360 bytes below the slot; accept or reduce risk only after the repaired
   generator build is measured again.

After any build/configuration/source/generated change, restart at S01, issue a
new production hash, and repeat every affected H case. No waiver can turn an
unrun hard requirement into a technical pass.
