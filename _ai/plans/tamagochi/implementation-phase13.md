# Tamagotchi implementation Phase 13: persistence and lifecycle safety

## Outcome

Finish the persistence policy and lifecycle hooks for the singleton Tamagotchi card.
After this phase, every player-visible mutation and important passive transition is
saved promptly, passive progress is checkpointed at a bounded cadence, card-stack
rebuilds flush state before destroying the wrapper, and the global power-off chord
flushes every live card immediately before deep sleep.

This phase is the final integration of the contracts from Phases 7-12. It does not
change gameplay, the version-1 record, clock synchronization, singleton policy,
portal behavior, or LVGL ownership.

## Preconditions and contracts consumed

Begin only after Phases 7-12 are implemented and buildable:

- `TamagotchiModel` owns state and accumulated dirty changes. `markPersisted()`
  clears those changes and is legal only after a successful store write.
- `TamagotchiStateStore::save(const TamagotchiState&)` returns true only when the
  complete, validated JSON record was written. The boot-lifetime store is already
  open and is borrowed by removable cards.
- Phase 9's card-session adapter owns `millis()` advancement, once-only epoch
  catch-up, the RAM copy of the loaded epoch, and `saveWithClockBaseline(bool)`.
- `TamagotchiCard` already has `_persistenceDirty`, `_saveFailed`,
  `_immediateSaveRequested`, `_preparedForRemoval`, change-driven rendering, and
  no removable event subscription or queued callback capturing `this`.
- Phase 11 requests immediate persistence after a successful hatch and every
  `Applied` action, including `Applied` with no model change.
- Phase 12 creates at most one live Tamagotchi, stores it only in
  `dynamicCards`, and performs reconciliation on `lvglTask` by preparing each
  handler, deleting its stack-owned root, and then deleting its wrapper.

If an implemented identifier differs, adapt its spelling only. Do not create a
second dirty flag in the store, a second epoch path, or a second state-store owner.

## Repository constraints to preserve

- `lvglTask` on core 1 serializes active-card `update()`, button dispatch, UI queue
  reconciliation, and the center+down sleep chord.
- Only the active handler receives `update()`. Inactive Tamagotchi time is consumed
  by its next wrap-safe `millis()` advance; no background pet task is needed.
- `CardNavigationStack::removeCard()` unregisters the handler and deletes the LVGL
  root. `TamagotchiCard` must never delete that root after stack ownership begins.
- The current sleep branch disables GPIO deep-sleep wake and directly calls
  `esp_deep_sleep_start()` without a persistence hook.
- The Tamagotchi store has no mutex. All pet loads/saves after startup must remain
  on `lvglTask`; OTA, portal, Wi-Fi, and event tasks must not call it.
- The version-1 record remains in namespace `tamagotchi`, key `state`, independently
  of `cards/config_list`. Removing a card configuration is not a pet reset.

## Scope and exact files

Modify firmware files:

- `src/ui/InputHandler.h` — add a default no-op sleep hook;
- `src/ui/TamagotchiCard.h` — add the final persistence cadence/lifecycle surface;
- `src/ui/TamagotchiCard.cpp` — implement trigger detection, baseline-aware saves,
  checkpointing, removal flush, and sleep flush;
- `src/ui/CardController.h` — declare the controller sleep fanout;
- `src/ui/CardController.cpp` — fan out sleep preparation to live dynamic handlers;
- `src/main.cpp` — invoke the controller hook in the existing global chord path.

Update focused documentation after code behavior is verified:

- `docs/cards.md` — removal flush and persistent pet-record ownership;
- `docs/input-and-navigation.md` — controller pre-sleep fanout and chord priority;
- `docs/hardware-and-power.md` — best-effort NVS flush before explicit deep sleep;
- `docs/configuration-and-state.md` — the `tamagotchi` namespace and failure policy;
- `docs/runtime-and-tasks.md` — all pet save hooks execute on `lvglTask`.

Do not modify `TamagotchiTypes.h`, model rules, JSON fields, `ClockService`,
`TamagotchiStateStore`, `CardNavigationStack`, portal routes, OTA workers, sprites,
generated assets, task creation, or partition/dependency configuration unless an
actual compile mismatch proves a narrowly scoped declaration fix is necessary.

## Exact public hook additions

Add this default method to `InputHandler` beside `prepareForRemoval()`:

```cpp
virtual void prepareForSleep() {}
```

It is deliberately non-pure so every existing card continues to compile and cards
without volatile persistent state do nothing. It has no LVGL implication.

Add this public method to `CardController`:

```cpp
void prepareForSleep();
```

Add this override to `TamagotchiCard`:

```cpp
void prepareForSleep() override;
```

Do not add a Tamagotchi getter to the controller and do not expose the state store,
model, or a sleep callback from `main.cpp`.

## Tamagotchi persistence state

Retain the Phase 10/11 fields and add one cadence anchor:

```cpp
uint32_t _lastPersistenceAttemptMillis;
```

Initialize it from the same constructor-time `millis()` sample used for
`_lastAdvanceMillis`. It is a wrap-safe throttle for passive retry/checkpoint work,
not a persisted timestamp. Convert the existing constant only at the use site:

```cpp
constexpr uint32_t checkpointIntervalMillis =
    TamagotchiConstants::PERSISTENCE_CHECKPOINT_INTERVAL_SECONDS * 1000UL;
```

The Phase 2 value is exactly 300 seconds; add a compile-time assertion that the
seconds-to-milliseconds conversion fits `uint32_t`. Do not introduce a second
five-minute literal.

The fields have distinct meanings:

- `_model.isDirty()` means typed model fields changed since the last successful
  exact snapshot write.
- `_persistenceDirty` covers a load result whose returned state was not persisted,
  and a forced write that later fails even if the model itself was clean.
- `_immediateSaveRequested` records an unfulfilled hatch/action/transition
  durability request. It remains true after failure.
- `_saveFailed` means an actual load/write durability failure and alone drives
  `SAVE!`. Ordinary dirty state must not show `SAVE!`.

Use one predicate, named for example:

```cpp
bool hasUnsavedState() const;
```

It returns `_persistenceDirty || _immediateSaveRequested || _model.isDirty()`.
Do not derive persistence state from render flags, status text, or clock state.

## Authoritative save helper

Every Phase 13 write must flow through Phase 9's
`saveWithClockBaseline(bool force)`. No caller may call `_stateStore.save()` or
`_model.markPersisted()` directly.

Keep the helper allocation-free at the card layer and free of LVGL calls. It may
use a small private leaf such as `persistCurrentSnapshot()` to prevent recursion,
but there must still be exactly one path that prepares an epoch and exactly one
path that evaluates the store result.

The helper performs this order:

1. Sample `millis()` once and call `advanceFromMillis(nowMillis)` before taking the
   snapshot. Any important transition produced by that advance joins this save.
2. If `force == false` and no unsaved state exists after advancing, return true
   without sampling/stamping the clock or writing NVS.
3. If offline catch-up is still `PendingClock`, perform Phase 9's clock decision:
   - with no valid epoch, set the model epoch to `0`, retain
     `_pendingBaselineEpoch` in RAM, and continue to the write;
   - with a valid epoch, apply the once-only formula
     `max(0, nowEpoch - pendingEpoch - sameBootAppliedSeconds)`, cap after the
     subtraction, handle baseline `0` and backward time exactly as Phase 9
     specifies, set the non-pending catch-up state, and continue to one write.
4. If catch-up already completed, use a nonblocking `tryGetEpoch()` sample. Stamp
   the valid epoch through `setLastUpdatedEpoch()`. If no valid epoch is available
   for a snapshot containing current-boot changes, stamp `0` rather than preserving
   an old baseline that could replay those changes after reset. Never use or persist
   `millis()` as an epoch.
5. Call `_stateStore.save(_model.getState())` exactly once for this attempt.
6. On true, call `_model.markPersisted()`, clear `_persistenceDirty`,
   `_immediateSaveRequested`, and `_saveFailed`, and leave the in-memory epoch equal
   to the exact stored snapshot.
7. On false, do not call `markPersisted()`. Set `_persistenceDirty` and
   `_saveFailed`, retain `_immediateSaveRequested`, and retain every dirty model bit.
8. Set `_lastPersistenceAttemptMillis` to the attempt's `millis()` sample on both
   success and failure. This prevents a failed checkpoint from retrying every
   5-ms/1-second update; a new immediate event may still override the throttle.

Refactor `tryApplyOfflineCatchUp()` only as needed to avoid a recursive or double
write. Its active-update path may detect a valid clock and delegate the complete
decision/write to `saveWithClockBaseline(true)`. Do not allow both helpers to save
the same catch-up snapshot. Once the catch-up state leaves `PendingClock`, a failed
write retries persistence but never reapplies catch-up simulation.

If a live clock moves backward after catch-up, save the new valid baseline without
applying negative simulation. If clock validity is later lost, epoch `0` is the
safe persistence value for newly advanced stats; exact offline history is forfeited
rather than replayed.

The save helper returns durability only. Rendering callers update the existing
status afterward when the root is still live. Removal and sleep callers deliberately
do not render.

## Exact trigger and cadence matrix

| Trigger | Detection point | Save policy | Failure retry |
| --- | --- | --- | --- |
| Initial load returned `persisted == false` | First active `update()`; removal/sleep can reach it sooner | One forced attempt; keep `SAVE!` until success | Next new immediate event, five-minute active checkpoint, removal, or sleep |
| Successful hatch | Egg-center handler after `hatch()` and immediate render bookkeeping | `saveWithClockBaseline(true)` in the same handler turn | Retain request; retry as above |
| Any `Applied` action | Selector-center handler, including `Applied/None` | Forced save after model/action bookkeeping; exactly one write even if Clean/Doctor also changes mess/sick | Retain request; a later applied action attempts again immediately |
| Mess changes either direction | Change mask from the current advance/action | Forced save; coalesce with the action or same advance | Retain request; retry as above |
| Sickness changes either direction | Change mask from the current advance/action | Forced save; coalesce with Doctor or the same advance | Retain request; retry as above |
| Child becomes Adult | `Stage` change whose old/new states are Child/Adult | Forced save once; evolution visual remains independent | Retain request; retry as above |
| Clock catch-up, missing-baseline rebaseline, or backward-clock rebaseline | Phase 9 pending-clock decision | One forced baseline-aware save | Retry the already-applied snapshot; never replay catch-up |
| Passive Needs/SimulationTime changes only | One-second active update | Save only when 300,000 wrap-safe ms have elapsed since the last persistence attempt | A failed checkpoint waits another five minutes unless an immediate/removal/sleep trigger occurs |
| Navigation away/inactive card | No update/fanout | No save solely for navigation; elapsed time is consumed on return | Not applicable |
| Reorder, removal, or full reconciliation | `prepareForRemoval()` while root is valid | Advance and force-save before animation/pointer cleanup | Log failure safely, retain prior durable NVS, then finish required cleanup |
| Center+down deep-sleep chord | Controller hook immediately before sleep | Nonblocking advance plus forced save for every live handler; Tamagotchi writes once | Record/log failure but continue to deep sleep |
| Refused/invalid action, selector movement/open, result expiry, animation expiry, render-only change | Corresponding input/update path | No save | Not applicable |
| OTA worker or unexpected reset/power loss | No direct card callback | No cross-task save; rely on immediate writes, checkpoint, NVS retention, and next-session epoch catch-up | Normal load/catch-up policy |

“Five-minute checkpoint” is a maximum passive write frequency, not a delay applied
to important events. It is evaluated only from the active card's update path. The
wrap-safe test is:

```cpp
static_cast<uint32_t>(nowMillis - _lastPersistenceAttemptMillis) >=
    checkpointIntervalMillis
```

An immediate save attempt resets the passive anchor. Therefore frequent actions do
not cause redundant checkpoint writes. A new hatch/action/transition still attempts
immediately even if another attempt occurred seconds earlier.

## Transition detection and update integration

Important transition decisions must use the change mask returned by the current
model call, not `_model.getDirtyChanges()`, because the accumulated mask may contain
an old already-attempted transition.

If Phase 9's private `advanceFromMillis()` currently returns `void`, either return
the just-produced `TamagotchiModelChange` from that private helper or pass it to a
focused `handlePersistenceChanges(changes)` helper. Preserve all Phase 9 accounting
and Phase 10 render accumulation. For each current call:

1. OR the mask into `_pendingRenderChanges` as before.
2. Request an immediate save when it contains `MessState` or `SickState`.
3. Request an immediate save only for the actual Child-to-Adult `Stage` transition;
   Hatch already owns its explicit save and must not play/save as evolution twice.
4. Coalesce all qualifying bits from one advance into one forced write.

At the one-second `update()` gate:

1. expire visual/result deadlines using the existing faster cadence;
2. advance current-boot time;
3. poll/complete pending catch-up nonblockingly;
4. if that work generated an important transition or an unattempted initial save,
   force-save once;
5. otherwise, if `hasUnsavedState()` and the checkpoint interval is due, call the
   helper with `force == false` once;
6. render model/status changes while the root is valid.

Do not service a failed `_immediateSaveRequested` on every update. Its original
event gets one immediate attempt; after failure the retained Boolean expresses
undurable state, while the cadence anchor controls passive retry.

In Phase 11 input handlers, advancement can itself cross an important boundary
before the requested button action. Service that boundary before returning from the
handler. If evolution closes the selector and suppresses the old action, save the
evolution only. If an action then applies, coalesce its transition flags and action
request into one write.

## `SAVE!` behavior

Keep Phase 10's priority `SAVE! > TIME? > SICK > MESS > empty`.

- Show `SAVE!` after an unpersisted load or any failed write.
- Do not show it merely because passive state is dirty between checkpoints.
- Clear it only after a successful save of the exact current model snapshot.
- A refusal, clock failure, navigation, or render pass cannot clear it.
- A successful later action/checkpoint/removal-preparation write clears the internal
  failure state; removal/sleep paths do not spend work repainting a disappearing UI.

Log only a short component/category message on write failure. Do not print the JSON
record, full state, arbitrary NVS values, Wi-Fi credentials, or API keys.

## Removal ordering

Extend `TamagotchiCard::prepareForRemoval()` in exactly this order:

1. Return immediately if `_preparedForRemoval` is already true.
2. Sample `millis()` and advance same-boot simulation before invalidating anything.
3. Resolve a currently available pending clock decision nonblockingly and call
   `saveWithClockBaseline(true)`. This is the removal force-save; catch-up and the
   removal snapshot must coalesce to one write.
4. Regardless of write success, call `lv_animimg_delete(_petImage)` only when the
   animimg pointer is non-null and valid.
5. Clear one-shot/result deadlines and transient visual/input state. Set the current
   visual to `None` and prevent further gameplay/update work.
6. Set `_preparedForRemoval = true`.
7. Null `_card` and every child pointer through `clearUiPointers()`.
8. Return without calling `lv_obj_delete()`, `lv_obj_del()`, asynchronous deletion,
   or deleting a child.

The controller then calls `CardNavigationStack::removeCard(lvglCard)`, which
unregisters the handler and deletes the root/children, and only afterward deletes
the C++ wrapper. The wrapper destructor must not save or touch nulled LVGL objects;
its construction-failure root cleanup remains the only wrapper-owned delete path.

Write failure does not block stack reconciliation because the existing void removal
contract cannot safely retain half-removed UI. It preserves the last successfully
stored record and logs the failure, but the newest RAM-only mutation may be lost
when the wrapper is destroyed. Do not claim otherwise in validation.

Do not erase the `tamagotchi/state` key in removal, reconciliation, destructor,
factory failure, or an empty card-configuration save. Re-adding constructs a new
wrapper and loads the same durable pet.

## Controller sleep fanout

Implement `CardController::prepareForSleep()` as a direct, allocation-free walk of
`dynamicCards`:

1. Iterate each `CardInstance` exactly once.
2. If `handler` is non-null, call `handler->prepareForSleep()`.
3. Do not call the legacy `animationCard` pointer separately; it aliases an entry
   and would double-dispatch.
4. Do not prepare the provisioning card unless it later gains explicit persistent
   sleep work; it is not in `dynamicCards` today.
5. Do not remove cards, clear tracking, refresh LVGL, take the display mutex, enqueue
   a callback, publish an event, wait for network time, or allocate a copied list.

The fanout is synchronous. It runs from the same `lvglTask` turn as the chord, after
the UI queue was processed and before the task can process another queued reconcile,
so `dynamicCards` and handler lifetimes remain stable for the walk.

## Tamagotchi sleep hook

`TamagotchiCard::prepareForSleep()` must:

1. Return if removal already invalidated the wrapper.
2. Advance through a current `millis()` sample.
3. Resolve pending clock state only through nonblocking `tryGetEpoch()` behavior.
4. Call `saveWithClockBaseline(true)` once.
5. Perform no LVGL call: no label/status update, animation stop, pointer validity
   query, refresh, backlight change, or object deletion.

It must not call `ClockService::waitForValidTime()`, delay, retry in a loop, or wait
for Wi-Fi/SNTP. A bounded Preferences write is the only synchronous persistence
operation. If it fails, retain flags and emit one safe log; explicit deep sleep
still proceeds because trapping the device awake is not a reliable recovery path.

## Global chord integration

In the existing two-second center+down branch in `lvglHandlerTask`, retain chord
priority and edge suppression. The final order is:

```cpp
Serial.println("Simultaneous CENTER and DOWN hold for 2s detected. Entering deep sleep.");
esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
cardController->prepareForSleep();
esp_deep_sleep_start();
```

The controller call must be the final application hook immediately before
`esp_deep_sleep_start()`. Check `cardController` defensively if task startup can
ever precede its construction. Do not put Tamagotchi-specific includes, store/model
calls, LVGL refreshes, or clock waits in `main.cpp`.

`esp_deep_sleep_start()` is expected not to return. If it does, log the platform
error once and reset the chord latch before accepting new input; do not write NVS
every 50 ms. This defensive branch must not alter the normal chord priority.

## Reset, power-loss, and OTA rationale

Do not call a removable card from `otaCheckTask`, `otaUpdateTask`, an HTTP callback,
or an event subscriber. Those contexts cannot safely own the card lifetime and the
store has no cross-task synchronization.

Durability is instead bounded as follows:

- hatch, applied actions, mess/sickness transitions, and evolution are saved at the
  event that creates them;
- active passive progress is checkpointed at most once per five minutes;
- explicit deep sleep has a synchronous UI-task flush;
- an unexpected reset or power loss can lose only work after the last successful
  write; with a valid next-boot clock, the persisted epoch catch-up reconstructs
  elapsed passive simulation without replaying already-saved uptime;
- without a valid wall clock, the firmware preserves the last durable stats and
  uses epoch `0` where necessary rather than inventing elapsed time;
- OTA replaces the application slot but preserves NVS, so the new firmware loads
  the last successful record and applies the same bounded catch-up protocol.

There is intentionally no pre-OTA card hook in Phase 13. Adding one would require a
UI-task coordination/acknowledgement protocol and lifetime cancellation contract,
not a direct call from the OTA worker.

## Thread, UI, and lifetime guarantees

- All model mutations, save attempts, removal preparation, and sleep fanout occur
  on `lvglTask` after startup.
- `ClockService::tryGetEpoch()` is nonblocking/thread-safe; no clock wait occurs on
  the UI task.
- `TamagotchiStateStore` has exactly one post-startup caller context and requires no
  new mutex.
- No Phase 13 callback captures a card; no task, timer, event, or queue item retains
  a removable pointer.
- The active update and input paths may render. The removal path renders nothing
  after preparation begins. The sleep path performs no LVGL operation at all.
- Controller reconciliation and sleep fanout cannot interleave because both execute
  on the same task. An event task may enqueue a later reconcile but cannot mutate
  `dynamicCards` during fanout.
- Store and clock references outlive every card and are never ended/deleted by it.
- `prepareForRemoval()` is idempotent, and no work occurs after its pointer nulling.

## Heap, stack, and leak checks

Phase 13 adds no recurring heap allocation, container copy, task, queue item, or
LVGL object. Reuse the state store's existing fixed-capacity JSON document/buffer.
Review the resulting `lvglTask` stack use because a save places those fixed buffers
on the call stack; confirm the existing 8192-byte task retains safe high-water headroom.

On hardware, record internal heap and PSRAM before and after at least 20 cycles of:

```text
add -> hatch/action -> reorder -> remove -> re-add
```

After transient LVGL cleanup settles, free heap/PSRAM must not trend downward,
input-handler registrations must not accumulate, and no invalid-object warning,
double deletion, stale callback, or watchdog reset may occur. Also repeat the
center+down sleep path after recent action and passive transition saves.

## Implementation order

1. Record the dirty worktree and preserve all unrelated changes.
2. Add the default `InputHandler::prepareForSleep()` and controller declaration.
3. Add the cadence anchor, unsaved predicate, and single store-result finalizer.
4. Consolidate Phase 9 epoch preparation behind `saveWithClockBaseline()` without
   changing its once-only accounting.
5. Connect hatch/applied-action and current-call transition masks to one immediate
   save attempt.
6. Add the five-minute active checkpoint and failed-attempt throttle.
7. Prepend advance/force-save to removal before existing animation/pointer cleanup.
8. Implement the allocation-free controller fanout and LVGL-free card sleep hook.
9. Insert the controller hook immediately before deep sleep in `main.cpp`.
10. Update focused documentation, run static searches/build, then perform hardware
    durability and leak scenarios.

## Failure branches

| Failure/edge | Required result |
| --- | --- |
| Store was unavailable at load | Fresh in-memory pet remains usable, `SAVE!` stays visible, first active/removal/sleep save retries |
| Immediate write fails | Model and request remain dirty; no `markPersisted()`; no tight-loop retry |
| Passive checkpoint fails | One attempt per five-minute cadence; next important event may retry sooner |
| Applied action changes no field | Forced save still occurs because player action durability is the contract |
| One advance creates mess, sickness, and evolution | Coalesce all flags into one write and preserve visual priority |
| Catch-up save fails | Catch-up is not reapplied; the exact resulting snapshot stays dirty for retry |
| Clock absent while saving | Save epoch `0`, keep pending loaded baseline in RAM only while catch-up is pending |
| Clock moves backward | Apply zero negative elapsed and persist a safe new baseline |
| Store rejects invalid live state | Treat as save failure; log category only; do not bypass validation |
| Removal flush fails | Finish animation cleanup/root handoff; prior NVS survives; newest RAM-only state is not claimed durable |
| Sleep flush fails | Log once and continue deep sleep; do not block or wait for network |
| Reconciliation queued during sleep fanout | It remains queued; same-task fanout sees stable current handlers |
| Hook sees null handler | Skip safely; continue other handlers |
| `prepareForRemoval()` called twice | Second call is a complete no-op |
| Card removed while clock is pending | No callback survives it; force-save uses current nonblocking clock result |
| OTA restarts firmware | No worker/card call; NVS load and epoch catch-up restore from last durable baseline |
| Unexpected reset before checkpoint with no clock | Last durable record loads; unsaved passive interval may be lost and must not be fabricated |
| `millis()` wraps | Unsigned advancement and checkpoint subtraction remain correct |

## Verification plan

### Static review

- Search for Tamagotchi `_stateStore.save(` calls; only the authoritative helper's
  leaf may contain one.
- Search for `markPersisted()`; it must be reachable only after a true save result.
- Confirm every save trigger in the matrix reaches `saveWithClockBaseline()`.
- Confirm no Phase 13 sleep method calls `lv_`, `waitForValidTime`, `vTaskDelay`,
  queue/event APIs, or a portal/OTA method.
- Confirm controller fanout walks `dynamicCards` once and does not double-call the
  legacy Friend pointer.
- Confirm the removal save precedes `lv_animimg_delete()` and pointer nulling.
- Confirm no root deletion exists in Tamagotchi's normal removal path.
- Confirm there is no erase/remove/clear operation for `tamagotchi/state`.
- Confirm checkpoint math is unsigned/wrap-safe and uses the Phase 2 constant.
- Confirm no new task, mutex, event type, removable subscription, or callback was
  introduced.

### Production build

Run the configured environment only:

```sh
pio run -e adafruit_feather_esp32s3_reversetft
```

Report warnings, RAM/flash usage, and final size against the `0x1F0000` application
slot. Review generated diffs even though this phase intentionally changes no assets.
There is no enabled native PlatformIO test environment; do not claim native tests
passed unless one is explicitly configured and run.

### Feather durability matrix

1. Start with clean NVS, add the card, hatch, reset immediately, and verify Child.
2. For all five actions, reset immediately after execution; applied effects survive,
   while refused actions create no save.
3. Trigger mess, sickness, Doctor recovery, Clean, and accelerated evolution; reset
   immediately after each transition and verify the exact saved state.
4. Let passive needs change for less than five minutes and prove no periodic write;
   cross five minutes and prove one checkpoint, with no repeated write each update.
5. Force a write failure, verify `SAVE!`, retained dirty/request state, bounded retry,
   then restore storage and verify the next permitted trigger clears `SAVE!`.
6. Remove and re-add the card after an action; verify the same pet. Repeat with a
   reorder/full rebuild and with no configuration entry, then re-add later.
7. Hold center+down in Egg, Normal, and selector modes. No pet button action occurs;
   after reset, the state present immediately before sleep is restored.
8. Repeat sleep with valid time, pending time, epoch `0`, and backward clock. No
   negative or duplicated simulation occurs.
9. Reset unexpectedly just before/after a checkpoint, both with valid time and no
   network, and record the explicitly bounded no-clock loss behavior.
10. Run an OTA after a recent action and during passive dirty progress. Confirm the
    OTA worker never calls the card, NVS survives slot replacement, and boot catch-up
    applies once.
11. Execute at least 20 rebuild cycles while Wi-Fi reconnects, insight events arrive,
    and portal work runs; monitor heap, PSRAM, LVGL validity, stack high-water mark,
    and watchdog behavior.

Mark reset/deep-sleep/OTA/NVS-failure and leak observations as hardware-only until
they are actually run on the Feather.

## Exit criteria

Phase 13 is complete only when:

1. Hatch, every Applied action, mess/sickness changes, and evolution each receive
   one immediate baseline-aware save attempt.
2. Applied/None saves, refusals do not, and overlapping reasons coalesce to one write.
3. Passive dirty state is checkpointed only from active update and no more often
   than once per five minutes between persistence attempts.
4. Every successful write clears model/card dirty state and `SAVE!`; every failed
   write preserves dirty/request state and never calls `markPersisted()`.
5. Delayed/missing/backward clock handling still obeys Phase 9, including epoch `0`
   invalidation and once-only capped catch-up.
6. Removal advances and force-saves before animation cleanup and pointer nulling,
   never deletes the stack-owned root, and never erases the pet record.
7. Re-add and reorder load the same successfully persisted pet.
8. `InputHandler` has a compatible default sleep hook and `CardController` fans it
   out once to each live dynamic handler.
9. The chord invokes that hook immediately before deep sleep, and Tamagotchi's
   sleep path contains no LVGL, network wait, callback, or cross-task access.
10. OTA/background tasks never call the removable card; restart recovery relies on
    NVS and epoch catch-up.
11. Repeated rebuilds show no double delete, stale handler/callback, heap/PSRAM
    trend, or unsafe stack growth.
12. The production firmware builds within the OTA slot, and all hardware-only gaps
    are reported honestly.

## Handoff to Phase 14

Phase 14 should treat the trigger matrix, failure semantics, and ordering above as
the durability contract. It must run the full reset/removal/re-add/reorder/deep-sleep
and OTA scenarios on the Feather, compare final firmware size to the Phase 1/6
baselines, and record any intentionally unverified fault-injection case. It should
not weaken failed-write dirty retention, add direct OTA/card calls, or reinterpret
an unavailable wall clock to make a test appear to pass.
