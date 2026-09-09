# Tamagotchi implementation Phase 14: final validation and release gate

## Outcome

Validate the completed Tamagotchi MVP against the feature scope and Steps 1–13,
produce reproducible host and Feather evidence, and make one unambiguous release
decision: `PASS`, `FAIL`, or `BLOCKED`.

This phase is validation only. It does not implement missing behavior, retune the
model, redesign sprites, add diagnostics to the release image, change persisted
data, or weaken an acceptance rule to obtain a pass.

The durable execution artifact is:

```text
_ai/plans/tamagochi/phase14-validation-report.md
```

Keep raw build logs, serial logs, photos, videos, HTTP transcripts, and measurement
tables under a timestamped temporary evidence directory such as:

```text
/tmp/deskhog-tamagotchi-phase14-YYYYMMDD-HHMMSS/
```

The report must identify that directory, summarize relevant evidence, and state
which hardware-only checks remain unverified. Do not commit noisy logs or secrets.

## Authoritative requirements and traceability

Read and reconcile these inputs before executing the phase:

- `_ai/plans/tamagochi-features.md`, especially the 13 MVP acceptance criteria;
- `_ai/plans/tamagochi-implementation-plan.md`, especially Steps 1–14 and the
  definition of done;
- every available `implementation-phase1.md` through
  `implementation-phase13.md` plan and its execution evidence;
- `AGENTS.md`;
- `docs/build-test-release.md`, `docs/assets.md`, `docs/ota.md`, `docs/cards.md`,
  `docs/input-and-navigation.md`, `docs/runtime-and-tasks.md`, and
  `docs/hardware-and-power.md`;
- `platformio.ini` and `partitions.csv`.

`implementation-phase13.md` is the authoritative detailed lifecycle/persistence
handoff for Step 13. The executor must trace the implemented
save/removal/deep-sleep behavior to that plan, the master plan, and recorded
implementation evidence. Missing Step 13 implementation evidence is a release
blocker; Phase 14 must not infer that lifecycle safety exists from the plan alone.

Use these trace IDs in every result:

| ID | Requirement | Primary final checks |
| --- | --- | --- |
| AC-01 | Portal adds exactly one removable/reorderable/resumable card | H01, H07, H11 |
| AC-02 | Fresh state is one egg and center hatches Child | H01, H05 |
| AC-03 | Child deterministically reaches Adult | H04, H08, H09 |
| AC-04 | No alternate forms or care-quality branches | S03, H04 |
| AC-05 | All five actions work with three buttons | H02, H04, H05 |
| AC-06 | Up/down page outside the selector | H05 |
| AC-07 | Center+down remains the two-second global sleep chord | H06, H09 |
| AC-08 | Neglect causes sickness; Doctor cures it | H03 |
| AC-09 | Same-boot and synchronized offline time are safe | H08 |
| AC-10 | Versioned pet NVS is isolated from other settings | H01, H07, H09 |
| AC-11 | Adult care continues; no death/reset terminal flow | H04, H08 |
| AC-12 | UI fits TFT and respects LVGL/task/allocation constraints | S03, H10, H11, H12 |
| AC-13 | Production firmware builds, assets are reviewed, OTA slot fits | S01–S04, H13 |

No requirement is satisfied merely because a source phase planned it. Link each
one to an observed result from this run.

## Scope boundaries

Phase 14 may:

- run read-only source, manifest, generated-output, and static checks;
- run the configured production build and measure its outputs;
- upload the reviewed production image to the designated test Feather;
- deliberately erase or seed the designated test board's NVS after recording that
  this is destructive and obtaining the board owner's approval;
- exercise portal, Wi-Fi, PostHog insight, reset, deep sleep, and OTA workflows on
  that test board;
- use a temporary host harness for the platform-neutral model if one already
  exists or can be built outside the repository without creating a fake
  PlatformIO native environment;
- collect temporary diagnostic evidence from an explicitly labeled
  instrumentation build, provided it is reverted and never substituted for the
  final production artifact.

Phase 14 must not:

- fix a failure in place; return it to the owning phase after triage;
- add an enabled native test environment merely to improve the report;
- claim host inspection proves TFT color, clipping, physical button behavior,
  NVS durability, deep sleep, Wi-Fi, TLS, or OTA behavior;
- hand-edit `include/sprites/` or use generated C as the asset source of truth;
- delete unrelated files, reset the worktree, or erase a non-designated device;
- use the workflow-generated `flash-command.txt`, `multi_flash.py -f`, or an
  application offset of `0x10000`; normal PlatformIO upload and Arduino OTA are
  the supported paths;
- log or place in URLs Wi-Fi passwords, PostHog personal API keys, bearer tokens,
  signed URLs, or raw NVS records.

## Host-runnable versus hardware-only checks

Keep these result classes separate in the report.

| Class | Checks | What it can prove |
| --- | --- | --- |
| Host/static | S01 preflight, S02 clean/source-generated diff, S03 source invariants, S04 production build/size | Revision, contract coverage, deterministic asset structure, compile/link success, exact artifact bytes, static RAM/flash reports |
| Host harness, if actually run | Pure model vectors and deterministic expected-state calculations | Platform-neutral state transitions only; not NVS, clock service, LVGL, buttons, tasks, or hardware |
| Feather required | H01–H13 | Real display, input, persistence, time integration, heap/PSRAM behavior, concurrency, reset/sleep, Wi-Fi, portal, insight, and OTA behavior |

There is no enabled native PlatformIO test environment. Report a temporary host
harness by its compiler, command, and vector list, not as “native tests.” Never
write “all tests passed” when any Feather case is unrun.

## S01 — preflight and immutable evidence identity

Before any build or device mutation:

1. Create the temporary evidence directory.
2. Record timestamp/time zone, repository path, branch, exact commit, last commit
   description, `git status --porcelain=v1 -uall`, and `git diff --stat`.
3. Classify every dirty path as pre-existing user work, expected Tamagotchi work,
   expected Phase 14 report, or unexplained. Preserve all unrelated changes.
4. Record the exact PlatformIO executable/Core version and resolved platform,
   framework, LVGL, ArduinoJson, Bounce2, web-server, and display-library versions.
5. Resolve the Phase 1 baseline report and record its commit, `firmware.bin` size,
   SHA-256, PlatformIO RAM/flash lines, sprite-symbol total, and slot headroom. If
   the baseline is missing or was built from an incomparable dependency/revision,
   create a newly labeled comparison baseline or mark delta claims blocked.
6. Verify the project decision for the documented LVGL 9.2.2 versus caret-resolved
   LVGL version. An undecided or unrecorded mismatch blocks UI release claims.
7. Check that Step 13 implementation and evidence cover immediate saves,
   five-minute checkpoints, removal flush, deep-sleep flush, retained pet NVS, and
   absence of direct OTA/background access to the removable card.
8. Record test-board identity, USB port, board revision if known, power source,
   serial baud 115200, and whether the board is safe to erase and OTA-update.
9. Define two redacted NVS fixtures: `CLEAN` and `EXISTING`. Record only schema,
   card layout, and the presence/functional status of Wi-Fi/API/insight settings;
   never record credential values.
10. Establish a monotonic evidence-case numbering scheme and a result owner.

Preflight passes only when every prerequisite is present or explicitly marked as a
blocker. Do not begin destructive board preparation while the target device or NVS
ownership is ambiguous.

## S02 — clean diff and generated-asset review

Capture hashes of tracked generated outputs before the build, then review source
and generated sides together:

- portal inputs versus `include/html_portal.h`;
- typography inputs versus all generated font files;
- all `raw-png/**/*.png` versus per-frame C/H files, `sprites.c`, and `sprites.h`;
- Phase 3/4 references and source strips remain outside `raw-png/`;
- exactly 17 Tamagotchi runtime groups contain 45 PNGs: 44 at 32x32 and one mess
  frame at 16x16;
- every runtime basename and normalized C symbol is globally unique and ordered;
- every Tamagotchi descriptor is ARGB8888 with 4096 bytes for 32x32 or 1024 bytes
  for 16x16;
- group arrays contain lexicographic `_00.._NN` order and exact counts;
- the six pre-existing walking frames/descriptors remain unchanged;
- there are no stale orphaned per-frame C/H files;
- total `sprite_tamagotchi_*_map` bytes are exactly 181,248.

Use `tools/process_tamagotchi_sprites.py check` and
`tools/validate_tamagotchi_sprites.py` when those Phase 5/6 tools exist. A green
build does not prove sprite regeneration: require PlatformIO's Python to import
Pillow and NumPy and reject a log containing `Sprite conversion skipped`. Require
the font generator's complete success, not just a zero exit.

After the build, compare generated hashes and inspect every generated diff. Accept
only an empty diff or a source-explained deterministic regeneration. A generated
change with no matching source change, a source PNG with no descriptor, a stale
descriptor, a walking regression, or an embedded reference/contact sheet is a
release blocker.

## S03 — static implementation audit

Record exact search/diff evidence for all of these invariants:

- one `CardType::TAMAGOTCHI` stable spelling and exact non-fallback ingestion;
- catalog metadata matches the master plan and `allowMultiple == false`;
- API validation rejects duplicate singletons and runtime reconciliation keeps only
  the first stable-ordered instance without rewriting crafted NVS;
- exactly one boot-lifetime state store and one boot-lifetime clock service;
- `configTime()` exists only in `ClockService`; OTA uses its bounded wait;
- no Tamagotchi, clock, or sprite task and no removable-card event subscription;
- model code has no Arduino, LVGL, NVS, network, task, logging, randomness, or heap
  dependency;
- card actions/model updates/state writes/LVGL mutations remain on `lvglTask`;
- no event or background callback captures a removable `TamagotchiCard`;
- all uptime/deadline arithmetic is wrap-safe and no `millis()` value is persisted;
- offline catch-up is once-only, subtracts same-boot seconds, then caps at 604,800;
- all Tamagotchi saves route through the clock-baseline-aware helper;
- state writes occur after hatch/applied action/important transition, at no-more-
  frequent-than-five-minute checkpoints, and before removal/deep sleep;
- save failure retains dirty state and visibly reports `SAVE!`;
- `prepareForRemoval()` stops animimg, nulls pointers, does not delete the
  stack-owned root, and is idempotent;
- no recurring `String`, `std::string`, JSON document, vector, LVGL-object creation,
  or animation restart appears in the card update path;
- no raw portal body, raw Tamagotchi NVS JSON, credentials, tokens, or private URLs
  are logged.

Any disagreement between implementation and phase contracts returns to the owning
phase before hardware sign-off.

## S04 — production build and exact capacity record

Use the canonical release environment without a preliminary clean unless an
incremental-build problem is documented:

```sh
set -o pipefail
pio run -e adafruit_feather_esp32s3_reversetft 2>&1 | tee "$EVIDENCE_DIR/build-production.log"
```

If `pio` is not on `PATH`, use the resolved PlatformIO executable and record it.
The authoritative build passes only when:

1. the command exits zero and reports success for the configured environment;
2. portal, all four fonts, and the reviewed sprite inventory regenerate
   successfully with no skip warning;
3. `firmware.bin`, `firmware.elf`, `firmware.map` when emitted, `partitions.bin`,
   and `bootloader.bin` exist and are non-empty;
4. the build uses `build_type = release`, PSRAM support, and `partitions.csv`;
5. source/generated post-build diffs satisfy S02.

Record, without rounding away bytes:

```text
gitCommit
buildCommand
PlatformIO RAM: used bytes / maximum bytes / percentage (exact printed line)
PlatformIO Flash: used bytes / maximum bytes / percentage (exact printed line)
firmware.bin bytes
firmware.bin SHA-256
firmware.elf bytes and section-size output
OTA slot bytes = 0x1F0000 = 2,031,616
slot headroom bytes = 2,031,616 - firmware.bin bytes
slot used percent = firmware.bin bytes / 2,031,616 * 100
Phase 1 firmware delta
Phase 6 sprite-only firmware delta
Tamagotchi map bytes = 181,248
sprite budget bytes = 204,800
sprite budget slack = 23,552
```

Keep PlatformIO's flash measurement distinct from the direct `firmware.bin` byte
count. The release image must be strictly no larger than 2,031,616 bytes and the
45 Tamagotchi map symbols must total exactly 181,248. Any over-slot image must not
be uploaded or offered through OTA. If final headroom is positive but operationally
too small for the observed toolchain/build variance, record that risk for an
explicit maintainer decision rather than calling it comfortable.

The SHA-256 from this step identifies the production image used for final Feather
validation. If any source or generated file changes later, rebuild, issue a new
hash, and repeat every affected hardware case.

## Deterministic Feather preparation

Use one designated Adafruit ESP32-S3 Reverse TFT Feather with stable USB power.
Upload through PlatformIO only after S01–S04 pass. Monitor at 115200 baud and keep
the exact production SHA-256 with every observation.

For tests needing a precise pet state, seed the dedicated `tamagotchi/state` JSON
with a separately reviewed one-shot fixture or existing bounded diagnostic seam,
then upload the exact production artifact without erasing NVS. The seed must obey
all Phase 2 invariants. Do not add a reset/debug route to production firmware. A
temporary fixture may contain no real Wi-Fi or API secret and must be removed from
the repository before the production build.

For a genuinely clean-NVS case, use the approved erase-flash workflow on the test
board and provision test credentials again. Record that the operation destroys all
settings. For existing-NVS cases, never erase; establish functionality of saved
Wi-Fi, API, insight, and card configuration before and after the test without
printing or hashing low-entropy secret values.

Use synchronized video or timestamped photos for visual/timing cases. “Looked OK”
without a start condition, duration, and expected result is not evidence.

## H01–H13 — complete Feather validation matrix

Each row is a separate recorded case. Repeat a row after any relevant rebuild.

| ID | Deterministic setup and data | Action and timing | Required result | Evidence |
| --- | --- | --- | --- | --- |
| H01 | Erased `CLEAN` board; no `tamagotchi/state`; production image; provision test Wi-Fi | Boot, open portal, add one Tamagotchi with empty config/order 0, observe egg for two full 900 ms cycles, press center once | One card only; egg colors/transparency/order are correct; center produces `HATCHED`, Child idle, all needs 100, age 0/deadline 21,600; saved state survives reset | Portal response, serial categories, video, post-reset state/visual |
| H02 | Valid healthy Child fixture with needs chosen so each delta is visible; restore fixture before each action | For Feed, Play, Clean, Rest, and sick-only Doctor: open selector, select action, center, observe complete one-shot and return | Exact action result/stat clamp; matching Child sprite group and duration; result text; return to healthy/sick idle; applied action requests durable save | Before/after typed state, video, reset check |
| H03 | Child then Adult fixtures immediately before mess/sickness thresholds; visible mess state; low hygiene/health state | Advance across due minute; leave mess through one 900-second penalty boundary; Clean; enter sickness; Doctor | Mess appears at due age and overlay persists; hygiene penalty applies; Clean clears/reschedules and uses matching sprite; sickness indicator/loop persists; Doctor clears sickness, health >=60, uses matching treatment sprite | State snapshots and full animation video for both stages |
| H04 | Child at age 86,340 with valid deadline/remainder; later Adult fixtures for every action | Allow exactly 60 simulated seconds, then exercise Adult Feed/Play/Clean/Rest/Doctor from controlled states | One Evolve sequence of 1200 ms, then Adult; never replays on reload; all five Adult action groups/results match model; adult remains playable with no death/alternate form | Timed video, state before/after/reboot |
| H05 | Stack has provisioning, Tamagotchi, and at least one other card; test Egg, Normal, and selector | Apply the full button matrix below, including wrap order and rapid/expiry edges | Up/down page only in Egg/Normal; selector consumes them; center hatches/opens/executes once; no synthetic held action; card remains escapable | Button-event log without levels/secrets and video |
| H06 | Test Egg, Normal, and selector separately; record state before each | Press center+down within one 50 ms poll and hold >=2.2 s; separately forward center first, then add down and hold | Same-sample chord forwards no pet edge and enters deep sleep after ~2 s; reset wakes; pre-sleep save survives. In staggered case only the already-forwarded center effect may remain; no later action occurs | Timestamped video, before/after state, sleep/reset log |
| H07 | `EXISTING` board with working Wi-Fi/API/insight and Tamagotchi among other cards | Navigate away/back; reorder twice; remove Tamagotchi; reboot; re-add; inject crafted duplicate config and reboot twice | State advances once while inactive, survives reorder/remove/reboot/re-add; unrelated settings still function; only first stable-ordered duplicate is created; NVS config is not silently rewritten | Redacted configured-card responses, functional checks, state age/stats |
| H08 | Controlled valid epoch fixtures described below | Run connected, delayed, absent, later-sync, backward, inactive-card, and eight-day cases | Same-boot and epoch seconds are never double counted; baseline zero is safe; backward time advances zero; cap is exactly seven days; `TIME?` semantics are correct | Epoch/age deltas, state snapshots, serial categories, wall-time record |
| H09 | Controlled Child/Adult/action/evolution fixtures; production image | After hatch, each of five Child actions, each of five Adult actions, and evolution, press hardware reset before transient animation/result expires; also test power loss, deep-sleep reset, removal flush, and OTA restart | Last applied transition persists; refused action does not invent state; evolution does not replay; no stale epoch replay; removed card record remains | Per-case before/action/reboot table and video timestamps |
| H10 | Warm production board with `EXISTING` stack and stable network; telemetry fields defined below | Perform 3 warm-up plus 20 measured full rebuilds, alternating two valid card orders; sample at prescribed quiet points | No crash, watchdog, double delete, invalid callback, duplicate handler, or cumulative heap/PSRAM loss beyond thresholds | CSV samples, serial log, regression calculations |
| H11 | Tamagotchi active/animating; valid portal client, reconnectable AP, refreshable insight, safe OTA scenario | Run the concurrency schedule below for >=5 minutes and through one OTA check/update restart | UI remains responsive; model/state stays coherent; no cross-task LVGL access, deadlock, corruption, missed removal cleanup, or secret log; OTA wait remains bounded | Video, HTTP results, insight timestamp, OTA status, task/watchdog log |
| H12 | Production image and all 17 sprite groups, values at bar/color thresholds | Observe each loop >=2 cycles and each one-shot once on actual TFT; inspect room/header/footer/bars at all threshold values | Correct BGRA color/transparency/frame order/timing; no halo, jitter, idle flash, clipping, text overlap, or indicator coverage; bars/status colors match thresholds/priorities | Group checklist plus macro photos/video |
| H13 | Exact S04 artifact; stable power/Wi-Fi; safe OTA target | Confirm direct artifact/slot arithmetic, perform a production serial flash, then an approved OTA using the stable `firmware.bin` contract | Binary fits both 0x1F0000 slots; serial-flashed release boots cleanly; OTA writes inactive slot, restarts, retains NVS/Tamagotchi; serial recovery path remains available | Hash/size table, OTA status, boot partition/version, retained-state proof |

## Child/adult action and sprite matrix

Use a fresh valid fixture for every row so one action cannot hide another through
clamping or a prior one-shot. Press center once; do not hold it.

| Stage | Action setup | Expected model/result | Required visual and return |
| --- | --- | --- | --- |
| Child | hunger 50, happiness 50, health 50 | Feed `Applied`; 85/55/53 | `tamagotchi_child_feed`, 750 ms, then Child idle |
| Adult | same | Feed `Applied`; 85/55/53 | `tamagotchi_adult_feed`, 750 ms, then Adult idle |
| Child | happiness 50, energy 15 | Play `Applied`; 70/0 | `tamagotchi_child_play`, 750 ms, then current idle/sick |
| Adult | same | Play `Applied`; 70/0 | `tamagotchi_adult_play`, 750 ms, then current idle/sick |
| Child | hygiene 25, health 50, mess true at due deadline | Clean `Applied`; hygiene 100, health 53, mess false, deadline age+21,600 | `tamagotchi_child_clean`, 750 ms, overlay hides, then idle |
| Adult | same | Clean `Applied`; same rules | `tamagotchi_adult_clean`, 750 ms, overlay hides, then idle |
| Child | energy 50, health 50 | Rest `Applied`; 85/52 | `tamagotchi_child_rest`, 900 ms, then idle |
| Adult | same | Rest `Applied`; 85/52 | `tamagotchi_adult_rest`, 900 ms, then idle |
| Child | sick true, health 10, hygiene valid | Doctor `Applied`; sick false, health 60 | `tamagotchi_child_doctor`, 900 ms, then Child idle |
| Adult | same | Doctor `Applied`; same rules | `tamagotchi_adult_doctor`, 900 ms, then Adult idle |

Also record boundary/refusal rows: Play at energy 14 gives `TOO TIRED`, no mutation,
animation, or new save; healthy Doctor gives `NOT SICK`; action while Egg gives
`HATCH FIRST`; Play at 15 is applied; Applied actions at already-clamped values
still animate and request persistence. A sick Feed/Play/Clean/Rest returns to the
matching sick loop. Doctor returns to healthy idle.

The visual inventory additionally requires Egg, Child idle, Adult idle, Child sick,
Adult sick, Evolve, and the static Mess overlay. Together with the ten action rows,
this covers every one of the 17 generated groups.

## Button and chord matrix

Run this matrix on physical buttons, not simulated GPIO calls:

| Start | Input | Expected consumption/effect |
| --- | --- | --- |
| Egg | Up / Down | false; previous / next card; no pet mutation |
| Egg | Center | true; hatch exactly once |
| Normal | Up / Down | false; previous / next card, even during action/result |
| Normal | Center | true; selector opens at Feed |
| Selector Feed | Down five times | true each; Feed→Play→Clean→Rest→Doctor→Feed |
| Selector Feed | Up once | true; Doctor |
| Selector Doctor | Down once | true; Feed |
| Selector each action | Center | true; execute once and return Normal |
| Normal while Evolve | Center | true; `GROWING`, no action |
| Selector when passive evolution occurs | any edge | selector closes, Evolve wins, no action/navigation |
| Any mode | unknown index, if injectable without firmware change | false; no mutation |
| Any mode | center+down same poll, held >=2.2 s | handler receives no individual edge; global sleep |

Repeat rapid center, an edge exactly as an action deadline expires, navigation away
during result text, and return after expiry. No button may repeat merely because
Bounce2's edge remains observable between 50 ms polls.

## NVS, persistence, reset, removal, and reorder protocol

### Clean NVS

1. Record destructive approval and erase only the designated board.
2. Flash the exact production artifact and confirm first-boot provisioning.
3. Add Tamagotchi through the portal; verify missing state creates canonical Egg.
4. Hatch and reset before the transient result expires.
5. Confirm Child state and unrelated freshly provisioned settings survive.

### Existing NVS

1. Begin with functional Wi-Fi, PostHog API/insight, multiple card configs, and a
   non-default Tamagotchi state.
2. Record redacted behavior-based baselines: connection succeeds, insight renders,
   configured-card types/order, and pet typed fields.
3. Reorder, remove, reboot, and re-add Tamagotchi. Removing configuration must not
   delete `tamagotchi/state`.
4. Confirm provisioning remains index 0 and all non-Tamagotchi settings still work.
5. Do not expose raw NVS bytes or secret values in evidence.

### Immediate durability

For hatch, ten stage/action combinations, and evolution, restore a controlled
fixture, execute one edge, and trigger hardware reset before its 750–1800 ms
transient completes. Record the observed edge-to-reset interval from video. After
boot, verify the exact expected typed state and that a one-shot/evolution is not
replayed. Repeat representative Feed, Doctor, and evolution cases with abrupt power
loss, deep sleep, removal-triggered rebuild, and OTA restart.

A save failure case must retain `SAVE!`, the model dirty state, and the immediate
save request until a later successful retry. Do not simulate failure by corrupting
real user NVS.

## Clock and elapsed-time protocol

Use real synchronized epoch readings or a controlled local network/time fixture.
Record baseline epoch, synchronization epoch, same-boot seconds applied, expected
catch-up, model age before/after, and stored final epoch. Allow at least two
one-second card update gates after changing network/time conditions.

| Case | Seed/setup | Exact expectation |
| --- | --- | --- |
| Connected catch-up | Hatched state with baseline exactly 600 s behind valid `now`; activate immediately | 600 s total advancement once; second update/revisit adds no duplicate epoch interval |
| Delayed NTP | Baseline 900 s behind eventual sync; block network for 120 measured same-boot seconds | 120 s comes from uptime and 780 s from epoch catch-up; total 900 s, once |
| Epoch delta smaller than uptime | Baseline delta 60 s, same-boot applied 120 s | Offline part saturates to 0; age never goes backward |
| No-network boot | Valid saved state, prevent Wi-Fi/time for >=120 s | `TIME?`; same-boot model advances; no guessed epoch catch-up |
| Unsynced action save/reset | Perform action during preceding case and reset offline | Action persists with epoch 0; already-applied uptime is not replayed |
| Later sync after epoch 0 | Restore network in same boot and on a subsequent boot | Apply no unknowable history; establish/save current baseline; clear `TIME?` |
| Backward clock | Seed baseline `now + 3600` | Apply zero elapsed; preserve age/needs; safely replace baseline with valid current epoch |
| Eight-day gap | Hatched Child age 0/remainder 0, baseline exactly 691,200 s behind | Advance exactly 604,800 s: Adult, age 604,800, needs clamp safely, sick/mess per model; store actual current epoch; excess day never replays |
| Inactive card | Navigate away for 180 measured seconds, return once | First active update applies about 180 s once; no background task or second application |
| `millis()` wrap | Use an existing seam/instrumented validation build only | Unsigned subtraction produces the intended positive delta and deadlines expire normally |

If manipulating upstream time risks TLS or another user's network, use an isolated
test network. A backward-clock or cap result from a diagnostic build must be
reconfirmed through production code paths using a valid seeded epoch record.

## Twenty-rebuild leak and heap/PSRAM protocol

Runtime samples must include:

```text
timestamp/cycle/phase
free internal heap
largest free 8-bit internal block
minimum-ever free internal heap
free PSRAM
largest free PSRAM block
minimum-ever free PSRAM
active card count and current index
```

Use existing production telemetry. If these fields are unavailable, a narrowly
gated diagnostic probe may be used to find a leak, but its absolute values do not
replace production evidence. Missing a reliable way to sample both heap classes is
a blocker for the leak gate, not permission to report “stable by observation.”

Procedure:

1. Boot production, allow Wi-Fi/insight activity to settle for 30 seconds, and
   collect ten one-second idle samples.
2. Record the median and range for free/largest internal heap and PSRAM.
3. Perform three unmeasured warm-up rebuilds.
4. Define one rebuild as a successful portal save that alternates between two valid
   card orders, followed by `CARD_CONFIG_CHANGED`, completed UI reconciliation,
   valid Tamagotchi root/handler registration, selection of Tamagotchi, and five
   quiet seconds.
5. Perform exactly 20 measured rebuilds. After every rebuild record one immediate
   sample and five one-second quiet samples; use the quiet median for analysis.
6. During cycles 5, 10, 15, and 20, open/execute one harmless controlled action and
   verify state remains the same pet rather than a fresh load.
7. After cycle 20, wait 30 seconds and collect ten final idle samples.

Pass requires all of the following:

- no allocation failure, crash, reboot, watchdog, invalid LVGL access, double
  deletion, stale callback, duplicate handler, or lost card;
- final median free internal heap is no more than
  `max(2048 bytes, 2 × initial idle range)` below the initial median;
- final median free PSRAM is no more than
  `max(4096 bytes, 2 × initial idle range)` below the initial median;
- final largest internal block is no more than
  `max(4096 bytes, 2 × its initial range)` below baseline;
- final largest PSRAM block is no more than
  `max(16384 bytes, 2 × its initial range)` below baseline;
- quiet medians for cycles 6–20 do not show a consistent negative per-cycle trend
  outside the corresponding idle-noise tolerance;
- minimum-ever values and their first-drop cycle are recorded, not interpreted as
  recoverable free-memory values.

Any threshold breach is `FAIL` pending triage. Repeat once under quiescent Wi-Fi to
separate card ownership from TLS/portal noise; do not average away a monotonic leak.

## Concurrency schedule

With Tamagotchi visible and a one-shot or sick loop active:

1. Disable/re-enable the test access point and observe disconnect/reconnect/time
   behavior without blocking the LVGL task.
2. While reconnecting, submit a valid portal reorder that forces a full rebuild.
3. Trigger an insight refresh so background fetch and event parsing overlap the
   rebuild; verify the insight later renders.
4. Submit one rejected malformed/duplicate portal payload and prove it causes no
   NVS write/rebuild, then one valid save.
5. Start an OTA update check before time is valid and verify it returns within the
   shared approximately ten-second bound; repeat when time is already valid.
6. With stable power and an approved known artifact, begin OTA download/write.
   Perform a pet action immediately before the write/restart and verify it after
   reboot. Do not interact with buttons during the flash write if doing so risks
   power loss.
7. Continue the combined scenario for at least five minutes or through OTA restart,
   whichever is longer.

Pass requires responsive navigation/animation, coherent state, successful later
insight rendering, exactly one clock/SNTP owner, no deadlock/watchdog/allocation
failure, no background LVGL call, no removed-card callback, no NVS corruption, and
no credential/private URL in logs. If no safe OTA artifact or stable test power is
available, H11/H13 remain hardware-blocked and the release is not fully validated.

## Visual frame, color, clipping, and status protocol

Review every group on the real TFT:

- loops: Egg, Child idle, Adult idle, Child sick, Adult sick for at least two full
  cycles each;
- one-shots: five Child actions, five Adult actions, and Evolve from first frame
  through live-state return;
- static: Mess beside both life stages and during an action/evolution.

For every group record: descriptor/group name, frame count, expected total
duration, observed order, loop/one-shot behavior, BGRA color correctness,
transparent-background appearance, stable body center/ground line, no halo,
no scale jump, and no clipped quill/foot/prop/effect. Compare against a known
walking descriptor if color or alpha is suspect.

Verify the full 233x135 layout: navigation indicators remain visible; the transformed
pet stays inside the room; mess does not overlap the pet; header/footer and all five
rows fit actual fonts. Check bar values and colors at 0, 29, 30, 59, 60, and 100.
Check status priority `SAVE! > TIME? > SICK > MESS > empty` and ensure lower-priority
conditions remain visually discoverable through the sick sprite or mess overlay.

## Logging and secret review

Capture serial logs for boot, load origin, save success/failure category, clock
state transitions, reconciliation, 20 rebuilds, and OTA. The accepted log may show
component name, category, schema version for unsupported data, byte counts, heap
metrics, public firmware version, and redacted test URLs.

Fail the release if logs contain:

- Wi-Fi password, PostHog personal API key, authorization header, cookie, token,
  private/signed download URL, or submitted portal JSON;
- raw `tamagotchi/state`, arbitrary NVS contents, or a full state dump;
- high-frequency per-frame/per-second spam that hides operational errors;
- raw configuration strings from malformed requests.

Search stored evidence before sharing it. Redact by replacing the complete value,
not by showing a prefix/suffix. Do not use secret hashes as proof of equality.

## Result and evidence template

Use this block for every S/H case:

```text
Case ID / trace IDs:
Result: PASS | FAIL | BLOCKED | NOT RUN
Production git commit:
firmware.bin SHA-256:
Board / port:
Start state and fixture hash (non-secret only):
Preconditions:
Actions and measured timing:
Expected result:
Observed result:
Heap/PSRAM before and after, when relevant:
Serial log timestamps/categories:
Photo/video/HTTP/build evidence path:
Secrets reviewed/redacted: yes | no
Deviation or uncertainty:
Owner and next action:
Retest required:
```

The final report must also contain:

- preflight/worktree classification;
- source-to-acceptance trace table;
- exact production build/capacity table;
- generated-asset inventory and diff disposition;
- host/static results separated from Feather results;
- H01–H13 summary;
- complete child/adult sprite/action checklist;
- NVS/time/reset/rebuild/concurrency tables;
- leak CSV summary and threshold calculations;
- logging/secret audit;
- failures, blockers, waivers (if any), and release decision.

A waiver may document an owner decision but cannot turn an unrun hard requirement
into a technical `PASS`.

## Failure triage and rollback

For each failure, consider no more than five plausible root causes, rank them by
evidence, and narrow to the most likely one or two before proposing code. Use this
order where relevant:

1. fixture/evidence error: invalid seeded cross-field state, wrong artifact hash,
   clock/network not controlled, or observation timing wrong;
2. source/generated mismatch: skipped generator, stale C/H, wrong frame order,
   palette/alpha/descriptor error, or dependency-version drift;
3. model/time/persistence defect: action/step ordering, dirty flag, baseline
   invalidation, save result handling, or NVS validation/isolation;
4. UI/input/lifecycle defect: button consumption, animation deadline/restart,
   task context, root ownership, callback lifetime, or reconciliation queue failure;
5. system-capacity/concurrency defect: internal heap/PSRAM fragmentation, stack
   pressure, TLS/JSON overlap, OTA task/status locking, or power/network instability.

Do not randomly patch during Phase 14. Preserve the failing evidence, identify the
owning phase, write the smallest remediation plan, implement/review it separately,
then restart validation from S01 with a new production hash. Repeat all cases whose
code, generated assets, dependencies, timing, NVS schema, or artifact changed.

Rollback rules:

- never reset the entire dirty worktree;
- regenerate C/H only from authoritative PNGs and remove only proven stale files;
- restore any temporary diagnostic/fixture wiring, prove its diff is gone, then
  rebuild production;
- if the binary exceeds the slot, do not upload/OTA; reduce source assets first as
  the master plan directs and repeat Phases 5/6/14;
- if the device becomes unbootable, use the documented BOOT+reset serial recovery
  and PlatformIO-managed erase/upload path, then treat NVS recovery as destructive;
- keep the last known-good production binary/hash until the replacement completes
  S01–H13.

## Release blockers

Any of the following prevents a final `PASS`:

- missing or incomparable Phase 1 baseline when a required delta cannot be proven;
- missing Step 13 lifecycle implementation/evidence;
- unresolved LVGL target/version decision;
- dirty/generated changes with unknown ownership or origin;
- generator skip, missing prerequisite, stale/orphaned descriptor, asset collision,
  walking regression, wrong sprite count/order/dimensions/format, or map total other
  than 181,248 bytes;
- production build failure, absent artifact, or `firmware.bin` over 2,031,616 bytes;
- unverified clean/existing NVS isolation, immediate durability, backward/no-clock/
  delayed-sync/seven-day behavior, physical buttons/chord, leak cycle, display,
  concurrency, or OTA preservation;
- any reset, crash, watchdog, deadlock, invalid LVGL access, double deletion,
  callback-after-removal, allocation failure, state corruption, or reproducible
  memory trend outside the leak thresholds;
- secret/raw-state exposure in logs or evidence;
- hardware observations made with a binary whose hash differs from S04.

Use `BLOCKED` for unavailable hardware, unsafe destructive test conditions, absent
credentials/test service, or missing evidence. Use `FAIL` when the check ran and
the implementation violated the expected result. Neither status is release-ready.

## Definition of done

Phase 14 and the Tamagotchi MVP are complete only when:

1. S01–S04 pass against one identified production revision and binary hash;
2. source and generated assets are complete, deterministic, reviewed, and clean;
3. the production image fits one `0x1F0000` OTA slot with exact recorded headroom;
4. all H01–H13 cases pass on the target Feather, with no hardware-only item inferred
   from host evidence;
5. Egg, Child, Adult, all ten stage/action combinations, both sick loops, Evolve,
   and Mess show the intended group with correct color, timing, transparency, and
   clipping behavior;
6. clean and existing NVS, immediate resets, removal/re-add/reorder, deep sleep,
   power loss, and OTA restart preserve exactly the intended state and unrelated
   settings;
7. same-boot, connected, delayed, absent, later, backward, inactive, and capped
   clock scenarios produce the deterministic expected accounting once;
8. physical navigation/selector/chord behavior matches the complete consumption
   matrix;
9. 20 measured rebuilds satisfy the internal-heap/PSRAM thresholds with no lifecycle
   or handler failure;
10. Wi-Fi reconnect, portal saves/rejections, insight refresh, and OTA activity can
    overlap without UI, persistence, task, or secret-handling regressions;
11. no native-test claim exceeds what was actually configured and run;
12. the validation report contains no unresolved failure, blocker, or hard-requirement
    waiver and ends with `Release decision: PASS`.

If any item is missing, end the report with `FAIL` or `BLOCKED`, name the owning
phase and smallest next action, and do not describe the Tamagotchi feature as done.
