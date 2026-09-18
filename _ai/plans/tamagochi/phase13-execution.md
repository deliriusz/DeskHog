# Tamagotchi Phase 13 execution record

## Result

Phase 13 is implemented in commit `cb7fe3a` (`phase 13`). Host validation
completed successfully. No Feather upload, NVS fault injection, reset, OTA, or
deep-sleep hardware scenario was performed, so the physical durability matrix
remains outstanding.

## Persistence and lifecycle implementation

`TamagotchiCard` now owns the final persistence policy without adding a task,
mutex, event subscription, queued callback, or additional state-store owner.

- `_lastPersistenceAttemptMillis` is initialized from the constructor's
  `_lastAdvanceMillis` sample. A compile-time assertion protects the existing
  300-second checkpoint conversion to milliseconds, and the active-card
  checkpoint uses wrap-safe unsigned subtraction.
- `hasUnsavedState()` combines the model dirty mask, persistence failure state,
  and unfulfilled immediate request. `SAVE!` remains tied only to a load/write
  failure.
- `saveWithClockBaseline(bool)` is the sole direct caller of
  `TamagotchiStateStore::save()` and `TamagotchiModel::markPersisted()`. It
  advances from one `millis()` sample, applies the pending-clock decision, writes
  exactly one snapshot per attempt, and updates the cadence anchor on either
  result.
- Successful hatch and every applied action force a same-turn save. Current-call
  change masks also force one coalesced save for mess, sickness, and an actual
  Child-to-Adult transition. Passive changes wait for the five-minute active-card
  checkpoint.
- Failed immediate writes retain the dirty/request flags and do not retry on
  every update. Later immediate events, a due checkpoint, removal, or sleep can
  retry. A completed catch-up is not reapplied after a failed write.
- Missing or later-invalid wall-clock samples store epoch `0` for newly changed
  state rather than preserving a replayable stale baseline. Valid, missing, and
  backward pending-clock decisions remain nonblocking.

`prepareForRemoval()` advances and force-saves before stopping the LVGL
animation or clearing any UI pointers. It is idempotent and hands root deletion
to `CardNavigationStack`; neither the removal path nor the destructor erases
`tamagotchi/state`.

`InputHandler` has a default no-op `prepareForSleep()` hook. The controller walks
each `dynamicCards` handler once without touching the legacy Friend pointer,
allocating, queuing work, or modifying LVGL. The center+down chord now disables
GPIO deep-sleep wake, fans out that hook, then enters deep sleep. The Tamagotchi
sleep hook performs only nonblocking advancement and a forced save—no LVGL call.

The focused cards, input, power, configuration/state, and runtime documents were
updated alongside the implementation.

## Static review

- `git diff --check` completed with no whitespace errors before the record was
  added.
- `src/ui/TamagotchiCard.cpp` has one `_stateStore.save()` call and one
  `markPersisted()` call, both in `saveWithClockBaseline()`.
- `prepareForSleep()` contains only the removal guard, `millis()` advancement,
  and the baseline-aware save; it does not call LVGL, wait for time, delay, queue
  work, or publish an event.
- `CardController::prepareForSleep()` directly visits `dynamicCards` and calls
  each non-null handler once. It does not separately call `animationCard`.
- The build regenerated the portal and sprite sources; the generated files did
  not introduce working-tree changes.

## Production build and OTA budget

The configured production command completed successfully:

```sh
~/.platformio/penv/bin/pio run -e adafruit_feather_esp32s3_reversetft
```

| Measure | Result |
|---|---:|
| RAM | 81,492 / 327,680 bytes (24.9%) |
| Flash / OTA application slot | 1,988,462 / 2,031,616 bytes (97.9%) |
| OTA-slot headroom | 43,154 bytes |
| `firmware.bin` size | 1,989,120 bytes |
| `firmware.bin` SHA-256 | `3e8985234066efa5e5fcf7517e405a35cf9caa008c7d52a49509e634b784b2ad` |

The current build still reports the pre-existing `src_filter` deprecation. There
is no enabled native PlatformIO test environment, so no native tests were run.

## Remaining Feather validation

Run the Phase 13 durability matrix on an Adafruit ESP32-S3 Reverse TFT Feather:

1. Reset immediately after hatch and each applied/refused action.
2. Verify mess, sickness, recovery, clean, and evolution saves, including a
   forced-write failure followed by a permitted retry.
3. Confirm passive changes checkpoint once after five active minutes and do not
   write continuously.
4. Remove, reorder, re-add, and later restore the card; the durable pet must
   remain while no invalid-object or double-delete warning occurs.
5. Hold center+down after recent changes in Egg, normal, and selector modes;
   verify the pre-sleep snapshot restores after reset.
6. Repeat sleep/restart behavior with a valid clock, pending clock, epoch `0`,
   and a backward clock. Confirm no duplicated or negative simulation.
