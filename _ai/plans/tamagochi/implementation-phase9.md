# Tamagotchi implementation Phase 9: trustworthy wall-clock support

## Outcome

Phase 9 introduces one long-lived `ClockService` as the firmware's sole SNTP owner,
shares it with OTA and the Tamagotchi runtime, and defines a once-only offline
catch-up protocol that cannot replay time already simulated during the current boot.

At the end of this phase:

- SNTP configuration starts asynchronously after `WIFI_CONNECTED`;
- no new continuously running FreeRTOS task exists;
- callers can distinguish a trustworthy epoch from an unset system clock;
- OTA still waits no more than approximately ten seconds for a valid clock;
- Tamagotchi uses `millis()` only for elapsed time within one boot;
- Tamagotchi uses epoch time only for persisted reboot/power-loss baselines;
- delayed synchronization cannot double-count same-boot progression;
- backward wall-clock changes never produce negative simulation;
- offline catch-up is applied at most once per loaded card session and capped at seven days;
- the card can show `TIME?` without any background LVGL access.

This is a planning phase document. Do not implement unrelated Tamagotchi UI,
gameplay, persistence schema, Wi-Fi, or OTA fixes while executing it.

## Preconditions and dependencies

Phase 9 assumes the following outputs from earlier phases exist:

- `src/tamagotchi/TamagotchiTypes.h` defines `TamagotchiState`;
- `TamagotchiState` has an integer `lastUpdatedEpoch` field;
- `src/tamagotchi/TamagotchiModel.h/.cpp` exposes allocation-free
  `advanceBy(uint64_t seconds)` behavior;
- `src/tamagotchi/TamagotchiStateStore.h/.cpp` can load and save the complete state;
- state-store saves return success/failure rather than clearing dirty state implicitly;
- the persisted simulation remainder belongs to the model and is not a wall-clock field.

If those names differ after Phases 7-8, adapt only identifiers; retain the ownership
and accounting invariants in this plan.

## Current repository facts that constrain the design

- `EventQueue` is constructed and started before `WiFiInterface` in `setup()`.
- `WiFiInterface::updateState(CONNECTED)` publishes `WIFI_CONNECTED`.
- That event currently originates from `ARDUINO_EVENT_WIFI_STA_CONNECTED`, before
  `ARDUINO_EVENT_WIFI_STA_GOT_IP` may have supplied an address.
- `EventQueue` delivers callbacks synchronously on its unpinned event task while
  holding the callback-list mutex.
- `EventQueue::publishEvent()` does not block and can drop an event when its
  20-element queue is full.
- `EventQueue` has no unsubscribe API.
- `lvglTask` on core 1 owns active-card `update()` and all new Tamagotchi LVGL work.
- `OtaManager::_ensureTimeSynced()` currently owns `configTime()`, a private
  `_timeSynced` flag, and a roughly ten-second polling loop in `otaCheckTask`.
- OTA TLS relies on valid UTC time, so an update check must still fail cleanly when
  the clock cannot become valid within its bounded wait.
- There is no documented battery-backed RTC; `millis()` resets at boot and must
  never be saved as an offline timestamp.

## Scope boundaries

In scope:

- a reusable clock service;
- one boot-lifetime event subscription;
- asynchronous SNTP initiation;
- thread-safe clock status/snapshot reads;
- OTA constructor/refactor and bounded clock wait;
- the Tamagotchi session-time state machine and persistence rules;
- `TIME?` state exposure for the later card renderer;
- focused deterministic tests if a native test target is explicitly introduced;
- production build and hardware validation instructions.

Out of scope:

- time zones, local civil time, daylight-saving time, calendars, or alarms;
- day/night pet behavior;
- a battery-backed RTC;
- a new timer task, pet task, or clock task;
- changing `WiFiInterface` to publish at got-IP;
- fixing existing direct provisioning UI calls in Wi-Fi callbacks;
- general `EventQueue` redesign;
- semantic-version, OTA mutex, download-length, or certificate changes;
- storing a timestamp outside the versioned Tamagotchi state record.

## Core ownership model

Keep three time concerns separate:

| Concern | Owner | Source | Persisted |
| --- | --- | --- | --- |
| SNTP setup and epoch validity | `ClockService` | `configTime()` and `time(nullptr)` | No |
| Current-boot pet progression | Tamagotchi card session | wrap-safe unsigned `millis()` deltas | Never |
| Reboot/power-loss baseline | `TamagotchiState` / state store | valid epoch snapshot | Yes |

`ClockService` must not call `TamagotchiModel::advanceBy()`, save NVS, know about
cards, or render `TIME?`. It reports trustworthy time only.

The Tamagotchi card session must not call `configTime()`, inspect SNTP internals,
or make OTA decisions. It combines model state, its own uptime accounting, and
epoch snapshots from the injected service.

## New service files

Create:

- `src/time/ClockService.h`
- `src/time/ClockService.cpp`

Use a neutral `time/` component rather than a Tamagotchi directory because OTA is
also a consumer. Do not place the implementation in `main.cpp` or `OtaManager`.

### Public types

Define a small state enum:

```cpp
enum class ClockSyncState : uint8_t {
    NotStarted,
    WaitingForNetwork,
    SyncRequested,
    Synchronized
};
```

Define a value-only snapshot:

```cpp
struct ClockSnapshot {
    ClockSyncState state;
    time_t epoch;
    bool valid;
};
```

Do not expose raw mutexes, SNTP callbacks, `tm`, formatted strings, or references
to service-owned storage.

### Public API

Use this bounded API, adjusting only include paths if project layout requires it:

```cpp
class ClockService {
public:
    static constexpr time_t minimumValidEpoch = 1704067200; // 2024-01-01 UTC

    explicit ClockService(EventQueue& eventQueue);

    void begin();
    void requestSync();
    ClockSnapshot snapshot();
    bool tryGetEpoch(time_t& epoch);
    bool waitForValidTime(uint32_t timeoutMs);

private:
    EventQueue& _eventQueue;
    SemaphoreHandle_t _stateMutex;
    ClockSyncState _state;
    bool _begun;
    bool _sntpConfigured;

    bool isSaneEpoch(time_t epoch) const;
    void handleEvent(const Event& event);
    void setState(ClockSyncState state);
};
```

If the project standardizes on `uint64_t` for persisted epochs, convert between
`time_t` and that type only after rejecting negative/invalid values. Do not narrow
an epoch to `uint32_t`.

### Validity contract

`tryGetEpoch()` and `snapshot()` must sample `time(nullptr)` and consider it valid
only when all of these hold:

1. the value is non-negative;
2. the value is at least `minimumValidEpoch`;
3. conversion to the persisted epoch type is lossless.

The 2024 threshold rejects the current implementation's extremely weak 16-hour
post-epoch threshold. It is a sanity floor, not a claim about accuracy.

A sane retained system time may be accepted even before this boot receives Wi-Fi;
this supports soft reset/deep-sleep retention without pretending that `millis()`
survives. An ordinary cold boot near 1970 remains invalid.

Once a sane epoch is observed, set service state to `Synchronized`. Loss of Wi-Fi
does not invalidate an already running system clock. A later `time(nullptr)` sample
that falls below the sanity floor returns invalid and moves the service back to
`WaitingForNetwork` or `SyncRequested`, as appropriate.

Do not invent a maximum acceptable epoch. Forward jumps are handled by the
Tamagotchi seven-day cap; TLS should receive the system clock ESP32 actually has.

### Synchronization behavior

`begin()` must:

1. be idempotent;
2. create/verify the small state mutex before registering a callback;
3. set `WaitingForNetwork` if no sane retained epoch exists;
4. subscribe once to `WIFI_CONNECTED` with a callback capturing `this`;
5. never block waiting for network or time.

The service is allocated in `setup()` and intentionally lives for the full firmware
boot. This lifetime is mandatory because `EventQueue` cannot unsubscribe callbacks.
Do not create it inside a removable card or capture a card in its subscription.

`handleEvent()` must filter immediately. For `WIFI_CONNECTED`, it calls
`requestSync()` and returns. It does not read/write NVS, wait for SNTP, publish a
second event, invoke OTA, touch the card, or call LVGL.

`requestSync()` must:

- return immediately if a sane epoch is already available;
- invoke `configTime(0, 0, "pool.ntp.org", "time.nist.gov")` as the one centralized
  SNTP configuration operation;
- mark the state `SyncRequested` only after configuration is issued;
- tolerate duplicate Wi-Fi-connected notifications and OTA requests;
- avoid holding `_stateMutex` across `configTime()`;
- never claim synchronization merely because `configTime()` returned.

Because `WIFI_CONNECTED` currently precedes got-IP, the call only starts the ESP32
SNTP machinery. The background stack may complete after IP/DNS become usable.
Do not block the event callback to wait for that completion.

`_sntpConfigured` prevents competing/redundant configuration owners. A reconnect
may refresh state and call `requestSync()`, but must not construct another task or
another service. If the underlying Arduino API requires reissuing `configTime()`
after an explicit stop, document and cover that path rather than adding a second
owner.

### Concurrency and synchronization

Service calls may arrive from:

- the unpinned `EventQueueTask` (`WIFI_CONNECTED` callback);
- core-0 `otaCheckTask` (`waitForValidTime()`);
- core-1 `lvglTask` (Tamagotchi polling);
- `setup()` during construction/begin.

Protect `_state`, `_begun`, and `_sntpConfigured` with the service mutex. Copy state
under the mutex and release it before logging, delaying, calling `configTime()`, or
calling `time(nullptr)` if the platform call could acquire its own locks.

Do not reuse `OtaManager::_dataMutex`; its scope remains OTA status/result data.
Do not expose a mutable cached epoch. Each snapshot reads the system clock, so the
service does not need a tick loop.

If `_stateMutex` allocation fails, log one focused error, leave the service unable
to start, make `tryGetEpoch()` return false, and make the OTA wait fail within its
normal deadline. Do not continue with unsynchronized shared flags.

`waitForValidTime(timeoutMs)` must:

1. call `requestSync()`;
2. check `tryGetEpoch()` immediately;
3. wait in 250 ms task delays while the wrap-safe elapsed `millis()` value is below
   `timeoutMs`;
4. return true as soon as an epoch is valid;
5. perform one final validity check at the deadline;
6. return false without changing the system clock or fabricating an epoch.

It is legal only from a worker/task context. Never call it from `setup()`, an event
subscriber, an HTTP callback, an input handler, or `lvglTask`.

## Startup wiring

Update `src/main.cpp` only for dependency wiring:

1. include `time/ClockService.h`;
2. add one boot-lifetime `ClockService* clockService` global beside the other
   component pointers;
3. after `eventQueue->begin()` and before `wifiInterface->begin()`, allocate
   `ClockService(*eventQueue)` and call `begin()`;
4. keep `WiFiInterface` as the publisher of `WIFI_CONNECTED`;
5. construct `OtaManager` with `*clockService`;
6. inject `*clockService` into the Tamagotchi/CardController construction path once
   that path exists in Phase 10/12;
7. do not call `requestSync()` directly from `main.cpp`;
8. do not create a clock task or add clock polling to `wifiTask`.

This ordering guarantees the subscription exists before the initial credential
event can lead to a connection. If `ClockService::begin()` fails, startup continues;
OTA reports its existing time-sync error and Tamagotchi remains in safe `TIME?`
mode.

## OTA refactor

Modify `src/OtaManager.h`:

- forward-declare or include `ClockService` as appropriate;
- change the constructor to accept `ClockService& clockService`;
- store `ClockService& _clockService`;
- remove `_timeSynced`;
- remove `_ensureTimeSynced()`;
- remove `time.h` if no other declaration requires it.

Modify `src/OtaManager.cpp`:

- initialize `_clockService` in the constructor;
- replace `_ensureTimeSynced()` in `_checkUpdateTaskRunner()` with
  `_clockService.waitForValidTime(10000)`;
- keep that wait inside `otaCheckTask` on core 0;
- keep the existing failure result/status and task cleanup behavior;
- update messages from ownership language such as "attempting NTP sync" to
  "waiting for valid network time";
- do not call `configTime()` anywhere in OTA;
- do not cache a second synchronized flag;
- do not change GitHub request, parsing, version comparison, update download, WDT,
  or status-locking behavior in this phase.

The timeout is exactly 10,000 ms at the service boundary. Polling granularity may
make wall duration slightly over ten seconds, but it must remain bounded and below
the OTA task's existing 30-second watchdog window.

If time was synchronized asynchronously before the portal starts an OTA check, the
wait returns immediately. If network time remains unavailable, OTA preserves its
current user-visible failure semantics rather than attempting TLS with an invalid
clock.

## Tamagotchi session-time state machine

The future `TamagotchiCard` owns a boot/session adapter; do not add `millis()` or
epoch logic to `TamagotchiModel`.

Add these private session fields when the card is created:

```cpp
enum class OfflineCatchUpState : uint8_t {
    PendingClock,
    Applied,
    RebaselinedWithoutHistory,
    RebaselinedAfterBackwardClock
};

uint32_t _lastAdvanceMillis;
uint32_t _millisRemainder;
uint64_t _sameBootAppliedSeconds;
uint64_t _pendingBaselineEpoch;
OfflineCatchUpState _catchUpState;
bool _timeUnknown;
```

Required helpers:

```cpp
void advanceFromMillis(uint32_t nowMillis);
void tryApplyOfflineCatchUp();
bool saveWithClockBaseline(bool force);
```

Use `TamagotchiConstants::MAX_OFFLINE_CATCH_UP_SECONDS` (`7 * 24 * 60 * 60`)
from Phase 2; do not introduce a second differently named UI/time literal.

### Session initialization

Immediately after loading state:

1. set `_lastAdvanceMillis = millis()`;
2. set `_millisRemainder = 0`;
3. set `_sameBootAppliedSeconds = 0`;
4. copy the loaded `lastUpdatedEpoch` into `_pendingBaselineEpoch`;
5. set `_catchUpState = PendingClock`;
6. set `_timeUnknown = true` until catch-up or safe rebaseline completes;
7. call `tryApplyOfflineCatchUp()` from the LVGL-owned initialization/update path.

Do not alter model stats merely because the clock is not ready.

### Same-boot progression

`advanceFromMillis(nowMillis)` must use unsigned subtraction:

```text
elapsedMs = nowMillis - _lastAdvanceMillis
```

This remains correct across one `millis()` wrap. Add `_millisRemainder`, convert
whole seconds only, retain sub-second remainder, and pass only those whole seconds
to `TamagotchiModel::advanceBy()`.

Increment `_sameBootAppliedSeconds` by the exact number of seconds passed to the
model. Do not infer it later from `millis()`, model age, or render cadence.

Call `advanceFromMillis(millis())` before evaluating a newly valid epoch. That
ordering accounts for current-boot time exactly once before offline subtraction.

### Once-only catch-up algorithm

`tryApplyOfflineCatchUp()` runs only while state is `PendingClock`:

1. advance current-boot time through the current `millis()` sample;
2. ask `ClockService::tryGetEpoch(nowEpoch)`;
3. if false, leave the state pending, keep `_timeUnknown = true`, and return;
4. if `_pendingBaselineEpoch == 0`, apply no offline seconds, call
   `model.setLastUpdatedEpoch(nowEpoch)`, mark `RebaselinedWithoutHistory`, and
   force-save;
5. if `nowEpoch < _pendingBaselineEpoch`, apply no seconds, replace the persisted
   baseline with `nowEpoch`, mark `RebaselinedAfterBackwardClock`, and force-save;
6. otherwise compute `rawEpochElapsed = nowEpoch - _pendingBaselineEpoch`;
7. subtract `_sameBootAppliedSeconds`, saturating at zero;
8. cap the remaining value at
   `TamagotchiConstants::MAX_OFFLINE_CATCH_UP_SECONDS`;
9. call `model.advanceBy(catchUpSeconds)` once if nonzero;
10. call `model.setLastUpdatedEpoch(nowEpoch)`;
11. mark `Applied`, set `_timeUnknown = false`, and force-save immediately;
12. never enter the algorithm again for this loaded card session.

The central invariant is:

```text
epoch catch-up = max(0, nowEpoch - loadedEpoch - seconds already advanced by millis)
```

Apply the seven-day cap after subtracting same-boot seconds. The cap limits the
unapplied offline interval, not healthy current-boot play time.

After any completion/rebaseline state, same-boot `millis()` remains the sole
simulation source. Later SNTP corrections do not advance or rewind the model.
Epoch is sampled again only to anchor a save.

### Saves before delayed synchronization

This branch is required to avoid a subtle double count:

- The loaded state may contain epoch `E0`.
- The card can advance with `millis()` while time remains invalid.
- An action or checkpoint can save those advanced stats before SNTP succeeds.
- Persisting the old `E0` with the advanced stats would replay that interval after
  the next reboot.

Therefore, when saving while catch-up is pending and no valid epoch exists:

1. keep `_pendingBaselineEpoch` in RAM for a possible later sync in this boot;
2. call `model.setLastUpdatedEpoch(0)` so the in-memory model and the snapshot sent
   to `stateStore.save(model.getState())` remain identical;
3. do not replace `_pendingBaselineEpoch` solely because the model/persisted field
   was invalidated;
4. call `model.markPersisted()` only after that exact model snapshot saves
   successfully; retain dirty state if the save fails.

If SNTP succeeds later in the same boot, the session can still use the RAM copy of
`E0`, subtract `_sameBootAppliedSeconds`, catch up once, and save a new baseline.

If power is lost before synchronization, the next boot sees baseline `0`, preserves
the already-saved stats, applies no unknowable offline duration, and establishes a
fresh baseline when time becomes valid. This sacrifices unknowable history rather
than double-counting it.

When saving after catch-up/rebaseline and the clock is valid, first advance from
`millis()`, then update the model through `setLastUpdatedEpoch(nowEpoch)` and save
that exact `model.getState()` snapshot. If the sampled epoch is behind the existing
persisted baseline, apply no negative time and replace the baseline safely. A
failed save leaves the model dirty flag set.

Never update `lastUpdatedEpoch` on every render tick. Stamp it only on an existing
immediate save, periodic checkpoint, catch-up/rebaseline save, removal flush, or
sleep flush.

## `TIME?` and UI-thread rules

`_timeUnknown` is true while the current loaded state has no completed trustworthy
baseline decision. The Phase 10 renderer displays `TIME?` from that flag.

`TIME?` means exact reboot/power-loss catch-up is pending or unavailable. It does
not pause same-boot simulation, block input, reset the pet, or imply a save failure.

No UI callback is needed in this design:

- the Wi-Fi event callback only starts asynchronous SNTP;
- `ClockService` never mutates LVGL;
- the active card polls `tryGetEpoch()` from its normal `update()` on `lvglTask`;
- catch-up, status-flag changes, and rendering then all occur on the LVGL owner.

A UI callback would be required only if a future background clock notification
directly requested a visual update. In that case the callback must copy primitive
status/epoch data and use `CardController::dispatchToLVGLTask()`; it must never
capture a removable `TamagotchiCard` unless cancellation/lifetime protection is
added. Do not add that event/callback in Phase 9.

## Event delivery and lifetime policy

Use the existing `WIFI_CONNECTED` event only. Do not add `TIME_SYNCED` or
`CLOCK_VALID` to `EventType` for this phase.

Reasons:

- the card can observe clock validity through polling on its owner task;
- a new event could be dropped when the queue is full;
- dynamic card subscribers cannot unsubscribe safely;
- a notification would add UI marshalling without improving correctness;
- OTA already has a bounded synchronous wait within its own worker.

If the one `WIFI_CONNECTED` publication is dropped, OTA's later `requestSync()` is
a recovery path. For Tamagotchi-only use, `ClockService::begin()` may also recognize
a sane retained clock, but it must not poll Wi-Fi or create a task. Log failed
critical event publication at the Wi-Fi producer in a later queue-hardening phase;
do not broaden Phase 9 into an `EventQueue` rewrite.

## Exact file change list

Create:

- `src/time/ClockService.h` — public state, snapshot, nonblocking request, bounded wait;
- `src/time/ClockService.cpp` — event subscription, centralized `configTime()`,
  epoch sanity check, synchronization, and bounded wait.

Modify:

- `src/main.cpp` — create the boot-lifetime service in the required order and inject it;
- `src/OtaManager.h` — constructor dependency, member reference, removal of local time owner;
- `src/OtaManager.cpp` — use the shared 10-second wait and remove `configTime()` logic;
- `src/ui/CardController.h/.cpp` — carry `ClockService&` to the Tamagotchi factory once
  the Phase 10/12 card construction path exists; no clock behavior belongs here;
- `src/ui/TamagotchiCard.h/.cpp` — add the session fields and three helper methods
  above when these files are created in Phase 10;
- `src/tamagotchi/TamagotchiTypes.h` — confirm epoch is wide enough and add the named
  seven-day constant if it is not already defined;
- `docs/runtime-and-tasks.md` — document `ClockService` contexts and confirm no task added;
- `docs/event-queue.md` — list `ClockService` as a `WIFI_CONNECTED` consumer;
- `docs/ota.md` — replace OTA-owned NTP wording with shared-service bounded wait;
- time/persistence design documentation from Phase 8 — document invalidated baseline `0`.

Do not modify:

- `platformio.ini` merely to implement time;
- `include/EventQueue.h` or `src/EventQueue.cpp`;
- Wi-Fi credential storage;
- generated assets;
- portal HTML or OTA status enums unless an existing compile dependency demands it.

## Implementation order

1. Add `ClockService` header and implementation with no card dependency.
2. Add compile-time/static assertions for epoch-width conversions where practical.
3. Wire service construction before Wi-Fi startup.
4. Refactor `OtaManager` constructor and remove its duplicated owner state.
5. Build to prove the shared clock compiles before touching Tamagotchi session logic.
6. Add the session state machine at the card boundary.
7. Route all card save sites through `saveWithClockBaseline()`.
8. Add `TIME?` state rendering only from the LVGL task.
9. Update architecture documentation.
10. Run deterministic and hardware scenarios below.

## Failure branches and required behavior

| Failure/edge case | Required behavior |
| --- | --- |
| Clock mutex allocation fails | Service remains invalid; no unsafe access; OTA times out; card shows `TIME?` |
| `WIFI_CONNECTED` arrives twice | Sync request is idempotent; no task or duplicate owner |
| Event arrives before got-IP | `configTime()` starts asynchronously; event task returns immediately |
| Wi-Fi event is dropped | OTA request can trigger sync; card remains safe and shows `TIME?` |
| No saved epoch | Apply no guessed offline time; baseline once valid; force-save |
| No network this boot | Preserve state; use same-boot `millis()` only; save invalid epoch baseline safely |
| SNTP succeeds after minutes | Subtract exact same-boot seconds; apply remainder once; save baseline |
| Epoch equals baseline | Apply zero catch-up; baseline/save normally |
| Epoch behind baseline | Apply zero; rebaseline; never decrement age/needs |
| Epoch far ahead | Apply at most seven days; save actual current epoch so excess is discarded once |
| Clock adjusts after catch-up | Do not resimulate; `millis()` remains same-boot owner |
| Save fails after catch-up | Do not run catch-up again in RAM; retain dirty flag and retry baseline save |
| Reset after unsynced save | Baseline `0` prevents replay; later sync rebaselines without guessed history |
| `millis()` wraps | Unsigned subtraction and remainder preserve elapsed time |
| Card inactive then active | First update advances one same-boot delta; no background pet task |
| Card removed during sync | Service survives; card has no subscription/callback; no use-after-free |
| OTA waits while card updates | Both read service snapshots safely; neither owns the other's state |

For the save-failure-after-catch-up row, retain a session flag indicating simulation
was already applied. A storage retry may restamp/save, but it must never re-enter
`model.advanceBy(catchUpSeconds)`.

## Validation scenarios

### Static/code review

- Search the repository and confirm `configTime(` appears only in `ClockService.cpp`.
- Confirm `OtaManager` no longer contains `_timeSynced` or `_ensureTimeSynced()`.
- Confirm no new `xTaskCreate*` call was added for clock or Tamagotchi time.
- Confirm no clock callback calls LVGL, state store, or `model.advanceBy()`.
- Confirm no removable card subscribes to `EventQueue` for time.
- Confirm all epoch arithmetic uses non-negative checked/wide types.
- Confirm all `millis()` arithmetic uses unsigned subtraction.
- Confirm every Tamagotchi save path uses the baseline-aware helper.
- Confirm `lastUpdatedEpoch` is never populated from `millis()`.

### Deterministic accounting tests

Use a fake clock/millis seam or a host-compatible session-time helper; do not make
the pure model depend on Arduino time APIs.

1. Valid clock at load: baseline 1,000, now 1,600, same-boot 0 -> advance 600 once.
2. Repeated update after case 1 -> no second 600-second advance.
3. Delayed sync: baseline 1,000, 120 same-boot seconds already applied, now 1,720 ->
   offline advance is 600, not 720.
4. Raw epoch elapsed smaller than same-boot elapsed -> offline advance saturates at 0.
5. Eight-day gap -> advance exactly seven days and save the real current epoch.
6. Backward clock -> advance 0, mark backward rebaseline, save new baseline.
7. Baseline 0 -> advance 0, mark no-history rebaseline, save current epoch.
8. Unsynced action save -> saved copy has epoch 0; in-RAM pending epoch is retained.
9. Sync after case 8 in same boot -> old RAM baseline remains usable and no same-boot
   time is replayed.
10. Reboot after case 8 before sync -> loaded epoch 0 prevents double count.
11. `millis()` near `UINT32_MAX` then wrapped -> correct positive delta.
12. Catch-up save failure then retry -> model advances once; dirty state remains until save.
13. Clock valid then Wi-Fi disconnects -> valid system epoch remains usable.
14. OTA wait with pre-synchronized time -> returns immediately.
15. OTA wait with no time -> returns false at approximately 10 seconds, not indefinitely.

### Firmware build

Run:

```sh
pio run -e adafruit_feather_esp32s3_reversetft
```

Report the result without claiming a native test environment unless one was
explicitly configured. Check final application size against the `0x1F0000` OTA slot.

### Feather hardware validation

1. Cold boot with valid Wi-Fi and observe asynchronous time becoming valid.
2. Start OTA before sync completes and verify its bounded wait succeeds or fails cleanly.
3. Cold boot without credentials, play long enough for visible model progression,
   and verify `TIME?` while same-boot progression continues.
4. Save an action while unsynchronized, reset, later connect, and verify no replay.
5. Boot offline, wait, then provision/connect in the same boot; verify one catch-up
   with current-boot seconds subtracted.
6. Set persisted baseline eight days behind and verify exactly seven days applied.
7. Set persisted baseline ahead of network time and verify zero negative progression.
8. Navigate away and return after several minutes; verify the inactive interval is
   accounted by the card's `millis()` delta once.
9. Remove/re-add the card while SNTP is pending; verify no crash or stale callback.
10. Run OTA and Tamagotchi together; verify UI responsiveness and valid TLS behavior.
11. Inspect serial logs for one clock owner and no high-frequency sync/status spam.

## Deliverables

- `ClockService` exists as the only `configTime()` owner.
- Startup constructs it before Wi-Fi can publish connection state.
- OTA uses the injected service and retains its bounded ten-second wait.
- Tamagotchi session accounting follows the once-only state machine.
- Unsynchronized saves invalidate the persisted epoch without discarding live state.
- `TIME?` is driven on the UI task with no background LVGL mutation.
- Relevant runtime, event, OTA, and persistence documentation is current.
- Production firmware builds and remaining hardware-only checks are recorded.

## Exit criteria

Phase 9 is complete only when all of the following are true:

1. There is exactly one SNTP/configuration owner in source.
2. No continuously running clock or pet task was introduced.
3. `WIFI_CONNECTED` starts synchronization without blocking `EventQueueTask`.
4. Clock state is safe across event, OTA, setup, and LVGL contexts.
5. OTA succeeds with an already/soon-valid clock and fails after a bounded wait otherwise.
6. A valid epoch is never inferred from `millis()` or the weak 1970-era threshold.
7. Current-boot simulation uses only wrap-safe uptime deltas.
8. Offline simulation uses only a checked persisted epoch delta.
9. Delayed sync subtracts same-boot applied seconds before the seven-day cap.
10. Each loaded session applies offline catch-up no more than once.
11. Backward time applies zero elapsed and establishes a safe new baseline.
12. Unsynced persistence cannot cause already-applied same-boot time to replay.
13. No service/event callback accesses LVGL; no UI dispatch is needed for polling.
14. Removing the card cannot leave a time callback capturing it.
15. Build and documented hardware checks show no regression in OTA, Wi-Fi, or persistence.

## Handoff to later phases

Phase 10 consumes `ClockService&` and implements the declared card-session fields,
helpers, and `TIME?` rendering. Phase 12 carries the dependency through the
`CardController` factory if registration does not yet exist. Phase 13 must route
action saves, checkpoints, removal flush, and deep-sleep flush through
`saveWithClockBaseline()`; it must not create an alternate epoch path.

If later work adds a background time event, it must first solve unsubscribe/card
lifetime and queue-loss semantics. The polling design here remains the MVP contract.
