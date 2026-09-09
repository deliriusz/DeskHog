# Tamagotchi implementation Phase 7: pure model

## Phase goal

Implement the deterministic, platform-neutral Tamagotchi state machine in:

```text
src/tamagotchi/TamagotchiModel.h
src/tamagotchi/TamagotchiModel.cpp
```

At the end of this phase, a caller can hatch the egg, perform all five care actions, advance simulation time, inspect state, and determine what changed and whether persistence is required. Given the same validated starting state, action sequence, and elapsed seconds, the model must always produce the same result.

This phase implements only the model. It does not implement the state store, wall clock, card, input policy, rendering, sprites, NVS, or lifecycle hooks.

## Required inputs and fixed contracts

Use `src/tamagotchi/TamagotchiTypes.h` from Phase 2 without renaming fields, changing enum values, or retuning constants. In particular:

- age is total simulated post-hatch seconds;
- simulation advances in complete 60-second quanta;
- `simulationRemainderSeconds` persists the unprocessed `0..59` seconds;
- `nextMessAtAgeSeconds` is a simulation-age deadline;
- Child evolves at age 86,400;
- need values are integers clamped to `0..100`;
- `lastUpdatedEpoch` is persisted metadata, not a model clock;
- `TamagotchiAction::Count` is not executable;
- no rule can kill, reset, or branch the pet.

The model accepts only a canonical fresh state or a state already validated against Phase 2. It must not duplicate Phase 8's raw JSON validation or silently repair malformed persisted state.

## Scope boundaries

The two model files may include only standard C++ headers and `tamagotchi/TamagotchiTypes.h`. They must not include or call:

- Arduino APIs, including `Arduino.h`, `millis()`, `time()`, or `String`;
- LVGL, card, button, animation, or sprite APIs;
- Preferences, NVS, ArduinoJson, `ConfigManager`, or filesystem APIs;
- Wi-Fi, SNTP, `ClockService`, epoch-delta calculations, or offline caps;
- FreeRTOS, event queues, mutexes, tasks, or callbacks;
- heap allocation, randomness, logging, or exceptions.

Phase 9 decides elapsed wall-clock time and enforces the seven-day offline cap before calling this model. Phase 7 merely consumes elapsed seconds.

## Ownership and lifetime

`TamagotchiModel` owns one `TamagotchiState` by value and one accumulated dirty mask. It owns no pointers or external resources.

- Construction copies a validated initial state; the default argument creates the canonical fresh egg.
- Callers receive only a `const` state reference. No mutable-state accessor is allowed.
- All gameplay mutation goes through `hatch()`, `performAction()`, or `advanceBy()`.
- Phase 9 may update the opaque persisted clock baseline only through `setLastUpdatedEpoch()`.
- The state reference remains valid for the model's lifetime but reflects later mutations.
- The card owns the model instance; the store only loads/saves snapshots and never retains a model pointer.
- The model is single-owner and single-threaded. Phase 13 keeps calls on the UI task rather than adding internal locking.

## Exact public API

Declare the transient outcome type in `TamagotchiModel.h`; do not persist it:

```cpp
struct TamagotchiActionOutcome {
    TamagotchiActionResult result = TamagotchiActionResult::InvalidAction;
    TamagotchiModelChange changes = TamagotchiModelChange::None;
};

class TamagotchiModel {
public:
    explicit TamagotchiModel(const TamagotchiState& initialState = TamagotchiState{});

    const TamagotchiState& getState() const;

    TamagotchiModelChange hatch();
    TamagotchiActionOutcome performAction(TamagotchiAction action);
    TamagotchiModelChange advanceBy(uint64_t elapsedSeconds);

    TamagotchiModelChange setLastUpdatedEpoch(uint64_t epochSeconds);

    bool isDirty() const;
    TamagotchiModelChange getDirtyChanges() const;
    void markPersisted();

private:
    TamagotchiModelChange processMinuteStep();
    TamagotchiModelChange fastForwardStableState(uint64_t minuteSteps);
    void recordChanges(TamagotchiModelChange changes);

    static uint8_t applyNeedDelta(uint8_t value, int16_t delta);
    static bool crossedBoundary(uint64_t oldAge, uint64_t newAge, uint64_t interval);
    static uint64_t saturatingAdd(uint64_t value, uint64_t increment);
    static uint64_t saturatingAddMinutes(uint64_t age, uint64_t minuteSteps);
    static bool canFastForward(const TamagotchiState& state);

    TamagotchiState _state;
    TamagotchiModelChange _dirtyChanges = TamagotchiModelChange::None;
};
```

Do not add setters for needs, stage, sickness, mess, age, remainder, egg, or location. Tests needing a special valid state construct it before passing it to the model.

`setLastUpdatedEpoch()` is deliberately narrow. It stores an already-decided baseline but does not obtain time, calculate a delta, cap catch-up, or decide clock validity. Accept `0` or the Phase 2 valid epoch range; reject any other value by leaving state unchanged and returning `None`.

## Change flags and dirty semantics

Every mutating call returns the flags for the externally observable net state changes made by that call:

| Flag | Fields covered |
| --- | --- |
| `Needs` | hunger, happiness, health, energy, hygiene |
| `Stage` | stage |
| `SickState` | sick |
| `MessState` | mess |
| `SimulationTime` | age, remainder, next-mess deadline, last-updated epoch |

Flags describe field changes, not player intent. An applied Rest at already-max energy and health can return `Applied` with `None`. The UI still uses `result` to show feedback and Phase 13 may still request an immediate save after every applied action.

After each call, OR its changes into `_dirtyChanges`. Refused/invalid actions and true no-ops do not dirty the model. `markPersisted()` clears the complete mask and must be called only after Phase 8 reports a successful save. A failed save leaves the mask intact. Reading state never changes flags.

Within a call, compare final values with entry values so a transient internal toggle that returns to its original value does not emit a false change bit.

## Canonical construction and hatch

The constructor copies `initialState` exactly and starts with `None` dirty changes. Loading an existing record is not itself a model mutation.

`hatch()` follows this order:

1. If stage is not Egg, return `None` and change nothing.
2. Set stage to Child.
3. Reset all five needs to their Phase 2 starting values.
4. Set age and remainder to zero.
5. Clear sickness and mess.
6. Set `nextMessAtAgeSeconds` to `MESS_INTERVAL_SECONDS`.
7. Preserve egg type, location, schema version, and `lastUpdatedEpoch` exactly.
8. Derive, record, and return the net flags.

For a canonical fresh egg, the result is `Stage | SimulationTime`; `Needs` is included only if a valid pre-hatch state had different need values. Repeated hatch calls never reset a Child or Adult.

Elapsed time supplied while the pet is an Egg is ignored entirely: age and remainder stay zero, the state stays clean, and `advanceBy()` returns `None`.

## Care action algorithm

`performAction()` first rejects every action while Egg with `RefusedWhileEgg`, including otherwise valid actions. For a hatched pet, use this exact switch:

| Action | Result and effect |
| --- | --- |
| Feed | `Applied`; hunger `+35`, happiness `+5`, health `+3` |
| Play | if energy `<15`, `RefusedLowEnergy`; otherwise `Applied`, happiness `+20`, energy `-15` |
| Clean | `Applied`; hygiene becomes 100, health `+3`, mess clears, next mess is current age `+21,600` with saturation |
| Rest | `Applied`; energy `+35`, health `+2` |
| Doctor | if not sick, `RefusedNotSick`; otherwise `Applied`, sickness clears and health becomes `max(health, 60)` |
| Count or unrecognized enum value | `InvalidAction`; no changes |

For all deltas, widen to at least signed 32-bit before arithmetic, then clamp to `MIN_NEED..MAX_NEED` and narrow to `uint8_t`. Never add a signed delta directly to an unsigned field.

Clean reschedules from current age even when no mess is visible. If saturated addition yields a deadline equal to current saturated age, resolve that already-due deadline immediately: the final state remains/returns `mess == true`. This unreachable-in-practice edge preserves Phase 2's `mess`/deadline invariant without wrapping.

Actions never change age, remainder, stage, egg type, location, or epoch. Doctor is the only action that clears sickness. Feed, Clean, and Rest may improve health but never implicitly cure it.

## Elapsed-time accumulator

`advanceBy(elapsedSeconds)` must be independent of call partitioning. For a hatched state:

1. Compute `baseSteps = elapsedSeconds / 60` and `tail = elapsedSeconds % 60`.
2. Add `tail` to the existing `0..59` remainder in a widened integer.
3. If that sum is at least 60, increment `baseSteps` once and subtract 60 from the sum.
4. Store the resulting remainder.
5. Process exactly `baseSteps` minute steps, using the bounded fast path below when eligible.
6. Return and record net changes relative to entry state.

Never form `elapsedSeconds + oldRemainder`; that expression can overflow. `elapsedSeconds / 60` can safely receive at most one carry. Never multiply a large step count without a prior saturation check.

Examples:

- remainder 0 plus 59 seconds produces remainder 59 and no need decay;
- another 1 second produces one minute step and remainder 0;
- remainder 59 plus `UINT64_MAX` seconds produces the mathematically correct remainder 14 without wrapping;
- zero elapsed seconds is a no-op;
- sub-minute elapsed time changes only remainder and emits `SimulationTime`.

## One-minute simulation order

Implement all passive behavior in `processMinuteStep()` in this order. Do not combine or reorder predicates later without updating deterministic vectors.

1. Save `oldAge`; set `newAge = saturatingAdd(oldAge, 60)`.
2. If `newAge == oldAge`, age can no longer progress: return `None` for this step. No cadence is repeatedly fired at saturated age.
3. Store `newAge` and emit `SimulationTime`.
4. If no mess exists and the nonzero mess deadline is now due (`newAge >= nextMessAtAgeSeconds`), set mess before applying decay.
5. If the age crossed a 1,200-second boundary, decrement hunger by one.
6. If it crossed a 2,700-second boundary, decrement happiness by one.
7. If it crossed a 1,800-second boundary, decrement energy by one.
8. If it crossed a 2,700-second boundary, decrement hygiene by one.
9. If mess now exists and the age crossed a 900-second boundary, decrement hygiene by one more.
10. Compute one non-stacking condition: `sick || hunger <= 20 || hygiene <= 20`, using values after steps 5-9 and the sickness state from the start of this minute.
11. If that condition is true and the age crossed a 900-second boundary, decrement happiness by one. It is one point even when sickness and both neglected needs are present.
12. If that condition is true and the age crossed a 3,600-second boundary, decrement health by one. Its predicates likewise do not stack.
13. After all decay, set sickness if health is at or below 40 or hygiene is at or below 20. Never clear sickness here.
14. If stage is Child and age is at least 86,400, set stage to Adult. Adult stays Adult forever.
15. Return flags from final field differences for this step.

A mess becoming due on a 900-second boundary therefore applies its first hygiene penalty immediately. Newly entered sickness does not retroactively add another penalty; sickness affects later steps, while severe hunger/hygiene can already satisfy the same non-stacking predicate on the current step.

Use boundary crossing, not equality-only modulo checks:

```text
crossedBoundary = newAge / interval > oldAge / interval
```

All contract intervals exceed one minute, so one minute crosses at most one boundary. This definition remains deterministic for a valid imported state whose age is not accidentally aligned to 60, and it avoids firing a cadence repeatedly after age saturation.

## Large elapsed values and bounded execution

The normal Phase 9 caller passes at most 604,800 seconds, which is 10,080 minute steps. That bounded loop is acceptable and preserves simple reviewable ordering. The public pure API must nevertheless not hang on `UINT64_MAX`.

After each detailed step, `canFastForward()` may become true only when:

- stage is Adult;
- sick is true;
- all five needs are zero.

At that point further passive decay cannot change needs, sickness, or stage. Batch all remaining steps with `saturatingAddMinutes()` and then set mess if its unchanged deadline was crossed. Preserve the already-computed remainder. Emit only the net `SimulationTime` and possible `MessState` flags.

`saturatingAddMinutes(age, steps)` must compare `steps` with `(UINT64_MAX - age) / 60` before multiplication. If too large, return `UINT64_MAX`; otherwise return `age + steps * 60`.

If age saturates before the stable predicate is reached, stop processing remaining steps because later minute attempts cannot cross another age boundary and are defined as no-ops. This guarantees termination and bulk-versus-incremental equivalence at the numeric limit.

Age, deadlines, and counters never wrap. Saturation does not reset the pet, create another evolution, or cause death.

## Bulk-versus-incremental equivalence

For any validated starting state and elapsed pieces whose mathematical sum is representable, these must finish identically:

```text
one.advanceBy(a + b)
two.advanceBy(a); two.advanceBy(b)
```

Compare every persisted field, not just visible needs. The accumulated dirty masks must also match after combining call results. This property depends on preserving remainder, using absolute post-hatch-age cadence boundaries, deterministic step ordering, and not reading external time.

The stable-state fast path must be tested against the minute loop at its entry boundary. It is an optimization of identical final state, not a different gameplay rule.

## File-by-file implementation order

1. Confirm Phase 2's header and constants are present and unchanged.
2. Add `TamagotchiModel.h` with the exact public API, outcome struct, ownership comments, and private helper declarations.
3. Add constructor, state/dirty accessors, `recordChanges()`, and `markPersisted()`.
4. Add widened need clamping and saturating integer helpers.
5. Implement hatch and its net change mask.
6. Implement action validation, refusal branches, clamped effects, and result masks.
7. Implement boundary detection and the ordered minute-step routine.
8. Implement the overflow-safe remainder accumulator.
9. Add stable-state fast-forward and numeric-limit termination.
10. Add the narrow epoch-baseline setter without any clock lookup/delta behavior.
11. Exercise deterministic vectors in a temporary host harness if available.
12. Run the production PlatformIO build and review only the two model files plus expected integration effects.

## Deterministic validation vectors

Use exact full-state comparisons. These are specifications for future automated tests; do not claim a native test environment exists.

1. Fresh default: Egg, age/remainder/deadline zero, all needs 100, no sickness/mess, epoch zero, clean dirty mask.
2. Hatch: Child, age 0, next mess 21,600, all needs 100, no sickness/mess; original epoch preserved.
3. Hatch twice: second call returns `None` and its full state is byte-for-field unchanged.
4. Egg advance: `advanceBy(UINT64_MAX)` changes nothing and does not dirty state.
5. Remainder split: `advanceBy(59)` then `advanceBy(1)` equals `advanceBy(60)`; final age 60 and remainder 0.
6. One-hour child: from hatch, 3,600 seconds yields hunger 97, happiness 99, health 100, energy 98, hygiene 99, sick false, mess false.
7. First mess: from hatch, 21,600 seconds yields hunger 82, happiness 92, health 100, energy 88, hygiene 91, mess true, deadline still 21,600.
8. Threshold ordering: valid Child at age 2,640, hygiene 21, otherwise-max needs, no mess/sickness; 60 seconds yields hygiene 20, happiness 98, and sick true.
9. Evolution edge: Child at age 86,340 becomes Adult after 60 seconds; a further large advance never changes stage again.
10. Play boundary: energy 14 refuses with no mutation; energy 15 applies and becomes zero while happiness clamps at 100.
11. Doctor: healthy refuses; sick with health 10 becomes not sick with health 60.
12. Relapse: after Doctor, if hygiene remains 20, the next completed minute sets sick again even if no decay boundary is crossed.
13. Clean: at age 10,000 with mess true and deadline 9,000, Clean sets hygiene 100, adds/clamps health, clears mess, and sets deadline 31,600.
14. Clamp: Feed at hunger 90/happiness 99/health 99 yields all three at 100; no unsigned wrap occurs.
15. Partition matrix: compare one bulk call with 1-second, 59/1, 60-second, uneven, and minute-by-minute partitions across 900, 1,200, 1,800, 2,700, 3,600, 21,600, and 86,400 boundaries.
16. Huge elapsed: from hatch, `UINT64_MAX` seconds terminates promptly, saturates age rather than wrapping, leaves remainder 15, reaches Adult, zeroes needs, sets sick and mess, and preserves epoch.
17. Huge elapsed with remainder 59: final remainder is 14 and no counter arithmetic overflows.
18. Dirty clearing: successful-save simulation via `markPersisted()` clears flags; refused actions do not restore them; a later mutation starts a new mask.
19. Epoch metadata: same value is a no-op; 0 and in-range values update with `SimulationTime`; out-of-range nonzero values are rejected.

## Build and review checkpoints

Host-isolation checkpoint, if a host compiler is available:

- compile `TamagotchiModel.cpp` and a temporary assertion harness with the repository include path;
- confirm the include graph has no Arduino, LVGL, Preferences, or ArduinoJson headers;
- exercise the vectors above without adding a fake PlatformIO native environment;
- run sanitizer/undefined-behavior checks only if already available, and report exactly what ran.

Production checkpoint:

```sh
pio run -e adafruit_feather_esp32s3_reversetft
```

- build `adafruit_feather_esp32s3_reversetft` without new warnings;
- confirm no generated assets are intentionally changed by this phase;
- inspect any generator-touched files before excluding them as unrelated;
- expect negligible flash/RAM movement and no heap allocation from model operations.

The repository has no enabled native PlatformIO test environment. Do not report native tests as passing unless a separate environment is explicitly configured and run. Hardware UI, storage, clock, and button validation belong to later phases.

## Failure branches and defensive behavior

- Non-Egg hatch: no-op, not a reset.
- Egg action: `RefusedWhileEgg`, regardless of selected valid action.
- Low-energy Play: `RefusedLowEnergy`, with no partial happiness gain.
- Healthy Doctor: `RefusedNotSick`, with no health gain.
- `Count` or forged enum: `InvalidAction`, no mutation or dirty bit.
- Zero/sub-minute advance: no decay; only an actual remainder change dirties simulation time.
- Invalid epoch setter input: no mutation; Phase 9 must not treat it as accepted.
- Need underflow/overflow: widened clamp, never unsigned rollover.
- Age/deadline overflow: saturate, never wrap.
- Numeric-age exhaustion: stop age-driven cadence; preserve a valid deterministic terminal numeric state.
- Save failure: model remains dirty because the caller must not invoke `markPersisted()`.
- Invalid loaded state: Phase 8 replaces it before construction; Phase 7 does not normalize it.

## Deliverables

- `src/tamagotchi/TamagotchiModel.h` with the exact host-compatible API and no platform dependencies;
- `src/tamagotchi/TamagotchiModel.cpp` with hatch, actions, minute simulation, fast-forward, saturation, flags, and dirty tracking;
- deterministic vector results recorded from any temporary host harness actually run;
- successful production build, or an exact build blocker report;
- reviewed diff with no LVGL, Arduino, NVS, clock, portal, asset, or task changes.

## Exit criteria

Phase 7 is complete only when:

- the model owns all gameplay state and exposes no mutable reference;
- fresh state, hatch, all action effects/refusals, decay, mess, sickness, Doctor, and evolution follow Phase 2 exactly;
- elapsed time uses fixed 60-second steps and persisted remainder;
- ordered predicates and cadence crossings are explicit and deterministic;
- bulk and partitioned advancement produce identical complete state;
- zero and `UINT64_MAX` elapsed inputs terminate safely;
- all needs clamp, age/deadlines saturate, and no arithmetic wraps;
- per-call change flags and accumulated dirty semantics are verified;
- model code remains allocation-free and host-compatible;
- the production firmware builds without introducing unrelated changes;
- no behavior from Phases 8-13 has been implemented early.

## Handoff to Phases 8-10 and 13

- Phase 8 constructs the model only from a fully validated `TamagotchiState`, saves `getState()`, and calls `markPersisted()` only after successful NVS persistence.
- Phase 9 owns uptime/wall-clock delta calculation, wrap-safe `millis()` handling, synchronization validity, backward-clock handling, seven-day catch-up capping, and the timing of `setLastUpdatedEpoch()`.
- Phase 10 renders only returned/current change flags and state; transient animation/result text does not enter the model.
- Phase 11 maps selector input to `performAction()` and uses `TamagotchiActionOutcome::result` independently of whether a saturated action changed state.
- Phase 13 requests immediate persistence after hatch, applied actions, and important transitions; passive changes remain dirty until checkpointed. A failed write keeps `SAVE!` active and does not clear model flags.
- Card removal and deep-sleep hooks may advance and save the model, but they do not add clock, NVS, or LVGL dependencies to it.
