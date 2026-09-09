# Tamagotchi implementation Phase 2: model and persistence contracts

## Phase goal

Define the stable, platform-neutral Tamagotchi vocabulary, state layout, tuning values, and version-1 persistence schema that Phases 7-13 will implement and consume.

At the end of this phase:

- later code has one authoritative `TamagotchiTypes.h` rather than UI, model, clock, and storage literals;
- every persisted enum value, JSON key, unit, default, range, and cross-field invariant is decided;
- the version-1 record has a measured worst-case serialized size below 384 bytes;
- the new header compiles in the production firmware without Arduino, LVGL, Preferences, or ArduinoJson includes;
- no gameplay, storage I/O, wall-clock synchronization, UI, card registration, or asset work has begun.

## Scope

This phase creates one production header:

```text
src/tamagotchi/TamagotchiTypes.h
```

It must contain:

- fixed-width enums for egg type, lifecycle stage, location, care action, action result, and model-change flags;
- `TamagotchiState` with explicit fixed-width field types and safe in-class defaults;
- the complete MVP simulation and action constants;
- persistence namespace/key, JSON capacity/length, epoch-sentinel, and checkpoint constants;
- small `constexpr` bitmask helpers for `TamagotchiModelChange`, if the compiler requires them at call sites;
- comments documenting units and stable numeric values.

The phase also records the exact JSON v1 mapping in this plan. Phase 8 must implement that mapping verbatim.

## Non-goals

Do not add any of the following in Phase 2:

- `TamagotchiModel.h/.cpp` or behavior methods;
- `TamagotchiStateStore.h/.cpp`, a `Preferences` instance, or an NVS write;
- ArduinoJson serialization/deserialization;
- `ClockService`, `millis()`, `time_t`, SNTP, or offline catch-up logic;
- `TamagotchiCard`, LVGL objects, input handling, sprites, or portal/card registration;
- new FreeRTOS tasks, `EventQueue` events, mutexes, or callbacks;
- a native PlatformIO test environment;
- inventory, currency, death, evolution branches, personalities, day/night, multiple eggs, or multiple locations.

## Dependencies and ordering

Phase 2 depends only on the Phase 1 baseline being recorded and buildable. It must precede:

- Phase 7, whose pure model uses these state fields, constants, results, and change bits;
- Phase 8, whose store implements the JSON/NVS contract;
- Phase 9, whose clock service owns the meaning of `lastUpdatedEpoch`;
- Phases 10-11, whose UI uses stage/action/result/change enums;
- Phase 13, whose save cadence uses the persistence checkpoint constant.

Artwork Phases 3-6 may proceed after this contract is accepted, but they must not redefine lifecycle or action enums.

## Concrete repository findings

- The production PlatformIO environment is `adafruit_feather_esp32s3_reversetft`; there is no enabled native test environment.
- The active firmware compiles as modern GNU C++, but this header should remain ordinary standard C++ and include only `<cstdint>` and `<type_traits>` as needed so it remains suitable for a future native target.
- Existing project code places component types in the global namespace, uses `#pragma once`, PascalCase type names, and camelCase fields/methods. Follow that convention.
- `ConfigManager` currently owns `wifi_config`, `insights`, and `cards` Preferences handles. Its `commit()` closes and reopens all three, and it has no mutex.
- Existing card configuration is a `DynamicJsonDocument(2048)` stored at `cards/config_list`. Tamagotchi state must not be added to it because a write rebuilds the dynamic card stack.
- Existing JSON code allocates dynamically and performs limited validation. Tamagotchi needs its own fixed-capacity, strict decoder in Phase 8.
- The installed ArduinoJson 6 configuration supports 64-bit integral values on this 32-bit target, so `uint64_t` age/deadline/epoch fields can remain exact JSON integers.
- The ESP32 Preferences API supports strings and has a 15-character namespace/key limit. `tamagotchi` and `state` fit.
- Current storage code does not establish safe concurrent writes. Tamagotchi state-store calls must remain on the LVGL/UI task after startup; no portal write path is part of the contract.
- A build runs asset generators and may touch tracked generated files. Phase 2 adds no assets; any generated diff during validation is unrelated and must be reviewed rather than silently included.

## Contract decisions that resolve master-plan ambiguities

1. `ageSeconds` means total simulated post-hatch age, not current-stage age and not wall-clock age. It is `0` throughout the egg stage and continues increasing after adulthood.
2. `nextMessAtAgeSeconds` is a deterministic simulation-age deadline, not a Unix epoch. This makes mess behavior independent of clock source and repeatable under bulk catch-up.
3. `simulationRemainderSeconds` is the unprocessed remainder below the fixed 60-second simulation step. It is persisted so splitting an elapsed interval across saves cannot change results.
4. `lastUpdatedEpoch` is UTC Unix seconds. `0` is the only no-clock sentinel; `millis()` is never stored in this field.
5. Enums are serialized as their explicit numeric codes. Values may only be appended in a compatible future schema; existing values must never be reordered or renumbered.
6. JSON uses readable full field names. Its worst-case v1 representation is approximately 299 bytes, so abbreviated keys are unnecessary and would make recovery/debugging harder.
7. All 15 v1 fields are required. Unknown extra fields are ignored when `schemaVersion == 1`; a missing, mistyped, out-of-range, or inconsistent known field invalidates the record.
8. A fresh egg and a newly hatched child start with all five needs at `100`. Hatching resets age/remainder and schedules the first mess six hours later.
9. Care does not branch evolution. Child becomes Adult exactly when post-hatch `ageSeconds` reaches 86,400 seconds.
10. Doctor is refused while healthy. This prevents Doctor from becoming an unlimited health action; Feed, Clean, and Rest remain the only healthy-state health gains.
11. Play is allowed at exactly 15 energy and consumes 15, leaving zero. It is refused only when energy is below 15.
12. Clean is allowed even without a visible mess because it can restore hygiene and health; it always clears `mess` and reschedules the next mess from the current age.
13. Sickness has no separate recovery timer. Doctor clears it immediately and raises health to at least 60. If hygiene remains critically low, a later simulation step can make the pet sick again.
14. No field can cause death, reset, an alternate form, or a terminal state. Age additions saturate rather than wrap.

## Exact C++ types

Create `src/tamagotchi/TamagotchiTypes.h` with this public shape. The implementing phase may improve comments but must not rename or renumber the contract without updating every downstream phase plan first.

```cpp
#pragma once

#include <cstdint>

enum class TamagotchiEggType : uint8_t {
    Default = 0
};

enum class TamagotchiStage : uint8_t {
    Egg = 0,
    Child = 1,
    Adult = 2
};

enum class TamagotchiLocation : uint8_t {
    Room = 0
};

enum class TamagotchiAction : uint8_t {
    Feed = 0,
    Play = 1,
    Clean = 2,
    Rest = 3,
    Doctor = 4,
    Count = 5
};

enum class TamagotchiActionResult : uint8_t {
    Applied = 0,
    RefusedWhileEgg = 1,
    RefusedLowEnergy = 2,
    RefusedNotSick = 3,
    InvalidAction = 4
};

enum class TamagotchiModelChange : uint16_t {
    None = 0,
    Needs = 1U << 0,
    Stage = 1U << 1,
    SickState = 1U << 2,
    MessState = 1U << 3,
    SimulationTime = 1U << 4
};

namespace TamagotchiConstants {

constexpr uint8_t SCHEMA_VERSION = 1;
constexpr uint8_t MIN_NEED = 0;
constexpr uint8_t MAX_NEED = 100;

constexpr uint32_t SIMULATION_STEP_SECONDS = 60;
constexpr uint64_t CHILD_TO_ADULT_AGE_SECONDS = 24ULL * 60ULL * 60ULL;
constexpr uint32_t MAX_OFFLINE_CATCH_UP_SECONDS = 7UL * 24UL * 60UL * 60UL;
constexpr uint64_t MESS_INTERVAL_SECONDS = 6ULL * 60ULL * 60ULL;

constexpr uint32_t HUNGER_DECAY_INTERVAL_SECONDS = 20UL * 60UL;
constexpr uint32_t HAPPINESS_DECAY_INTERVAL_SECONDS = 45UL * 60UL;
constexpr uint32_t SICK_OR_NEGLECT_HAPPINESS_PENALTY_INTERVAL_SECONDS = 15UL * 60UL;
constexpr uint32_t ENERGY_DECAY_INTERVAL_SECONDS = 30UL * 60UL;
constexpr uint32_t HYGIENE_DECAY_INTERVAL_SECONDS = 45UL * 60UL;
constexpr uint32_t MESS_HYGIENE_PENALTY_INTERVAL_SECONDS = 15UL * 60UL;
constexpr uint32_t HEALTH_DECAY_INTERVAL_SECONDS = 60UL * 60UL;

constexpr uint8_t SEVERE_NEED_THRESHOLD = 20;
constexpr uint8_t SICKNESS_HEALTH_THRESHOLD = 40;
constexpr uint8_t SICKNESS_HYGIENE_THRESHOLD = 20;

constexpr uint8_t STARTING_HUNGER = 100;
constexpr uint8_t STARTING_HAPPINESS = 100;
constexpr uint8_t STARTING_HEALTH = 100;
constexpr uint8_t STARTING_ENERGY = 100;
constexpr uint8_t STARTING_HYGIENE = 100;

constexpr int16_t FEED_HUNGER_DELTA = 35;
constexpr int16_t FEED_HAPPINESS_DELTA = 5;
constexpr int16_t FEED_HEALTH_DELTA = 3;
constexpr int16_t PLAY_HAPPINESS_DELTA = 20;
constexpr int16_t PLAY_ENERGY_DELTA = -15;
constexpr uint8_t PLAY_MINIMUM_ENERGY = 15;
constexpr uint8_t CLEAN_HYGIENE_VALUE = 100;
constexpr int16_t CLEAN_HEALTH_DELTA = 3;
constexpr int16_t REST_ENERGY_DELTA = 35;
constexpr int16_t REST_HEALTH_DELTA = 2;
constexpr uint8_t DOCTOR_MINIMUM_HEALTH = 60;

constexpr uint32_t PERSISTENCE_CHECKPOINT_INTERVAL_SECONDS = 5UL * 60UL;
constexpr uint16_t MAX_SERIALIZED_STATE_BYTES = 384;
constexpr uint16_t JSON_DOCUMENT_CAPACITY_BYTES = 512;
constexpr uint64_t MIN_VALID_EPOCH = 1577836800ULL; // 2020-01-01 UTC
constexpr uint64_t MAX_VALID_EPOCH = 253402300799ULL; // 9999-12-31 UTC

constexpr char NVS_NAMESPACE[] = "tamagotchi";
constexpr char NVS_STATE_KEY[] = "state";

} // namespace TamagotchiConstants

struct TamagotchiState {
    uint8_t schemaVersion = TamagotchiConstants::SCHEMA_VERSION;
    TamagotchiEggType eggType = TamagotchiEggType::Default;
    TamagotchiStage stage = TamagotchiStage::Egg;
    TamagotchiLocation location = TamagotchiLocation::Room;
    uint64_t ageSeconds = 0;
    uint8_t hunger = TamagotchiConstants::STARTING_HUNGER;
    uint8_t happiness = TamagotchiConstants::STARTING_HAPPINESS;
    uint8_t health = TamagotchiConstants::STARTING_HEALTH;
    uint8_t energy = TamagotchiConstants::STARTING_ENERGY;
    uint8_t hygiene = TamagotchiConstants::STARTING_HYGIENE;
    bool sick = false;
    bool mess = false;
    uint64_t nextMessAtAgeSeconds = 0;
    uint8_t simulationRemainderSeconds = 0;
    uint64_t lastUpdatedEpoch = 0;
};
```

Add `constexpr operator|`, `operator|=`, and `hasModelChange(changes, flag)` helpers only for `TamagotchiModelChange`. Do not define arithmetic or implicit-conversion helpers for the persisted enums. Keep `TamagotchiAction::Count` as a selector bound only; reject it as an executable action.

Add compile-time assertions in the header:

- every enum has the intended underlying type;
- `TamagotchiState` is standard-layout and trivially copyable;
- `sizeof(TamagotchiState) <= 64` bytes on the firmware target;
- every interval used by the minute-step model is divisible by `SIMULATION_STEP_SECONDS`;
- every starting need and action threshold is within `MIN_NEED..MAX_NEED`;
- the child-to-adult and mess thresholds are nonzero multiples of the simulation step;
- `MAX_OFFLINE_CATCH_UP_SECONDS` is a multiple of the simulation step;
- the NVS namespace and key are no longer than 15 characters.

Do not assert an exact struct size because padding and 64-bit alignment may differ on a future host test target.

## Tuning semantics and units

All durations above are seconds. All needs are inclusive integer values from 0 through 100. Action deltas are signed 16-bit values so Phase 7 can perform arithmetic in a widened temporary before clamping back to `uint8_t`.

The Phase 7 implementation must interpret the values as follows:

| Rule | Exact contract |
| --- | --- |
| Simulation quantum | Process only complete 60-second steps and persist `0..59` remaining seconds. |
| Hunger | Minus 1 every 1,200 simulated seconds after hatch. |
| Happiness | Minus 1 every 2,700 seconds; additionally minus 1 every 900 seconds while sick or while hunger/hygiene is at or below 20. Sick and neglect use one non-stacking bonus condition. |
| Energy | Minus 1 every 1,800 seconds. Sickness does not add a second energy rule in v1. |
| Hygiene | Minus 1 every 2,700 seconds; additionally minus 1 every 900 seconds while a mess exists. |
| Health | Minus 1 every 3,600 seconds while sick or while hunger/hygiene is at or below 20. The conditions do not stack. |
| Sickness entry | After decay for a minute-step, set sick when health is at or below 40 or hygiene is at or below 20. Only Doctor clears it. |
| Mess | First due at age 21,600; once due, `mess` remains true and the due age remains unchanged until Clean. Clean sets the next due age to current age plus 21,600. |
| Evolution | Child changes to Adult at age 86,400 after processing the step that reaches the threshold. Adult never evolves again. |
| Saturation | Need changes clamp to 0..100; age and next-mess addition saturate instead of wrapping. No saturation condition resets or kills the pet. |

These behavior statements are contract decisions, not Phase 2 code. Phase 7 remains responsible for the ordered minute-step algorithm and bulk-versus-incremental equivalence.

## State defaults and invariants

### Fresh egg

`TamagotchiState{}` is the canonical absent/corrupt/unsupported-record fallback:

- schema 1, Default egg, Egg stage, Room location;
- all five needs equal 100;
- age and remainder equal 0;
- not sick, no mess, and next-mess age equal 0;
- last-updated epoch equal 0 until a trusted clock baseline is available.

### Hatch transition contract

Phase 7's `hatch()` must accept only Egg and produce:

- Child stage;
- all five starting needs reset to 100;
- age 0, remainder 0;
- not sick and no mess;
- `nextMessAtAgeSeconds = MESS_INTERVAL_SECONDS`;
- unchanged `lastUpdatedEpoch`, because only the card/clock integration owns epoch baselining.

A second hatch request is a no-op. It must not reset a Child or Adult.

### Load-time validation

Phase 8 must validate raw JSON values before narrowing them into these C++ types:

- schema must equal exactly 1;
- enum numbers must be integral and one of the declared persisted values; `Action::Count` is never persisted;
- each need must be an integral `0..100` value;
- booleans must be JSON booleans, not strings or numeric aliases;
- remainder must be integral and less than 60;
- last epoch must be 0 or lie in `MIN_VALID_EPOCH..MAX_VALID_EPOCH`;
- Egg requires age 0, remainder 0, `sick == false`, `mess == false`, and next-mess age 0;
- Child requires age below 86,400 and a nonzero next-mess deadline;
- Adult requires age at least 86,400 and a nonzero next-mess deadline;
- when a hatched state has no mess, its next-mess deadline must be greater than age;
- when a hatched state has a mess, its next-mess deadline must be less than or equal to age;
- location must be Room and egg type must be Default in v1.

Do not silently clamp persisted corruption. Clamping belongs to valid model transitions; invalid stored data falls back to a fresh egg so corruption is visible and deterministic.

## Version-1 JSON and NVS schema

Store one compact JSON object as an NVS string:

```text
namespace: tamagotchi
key:       state
type:      Preferences string
```

The stable key mapping is:

| JSON key | C++ field | JSON type | Unit/range |
| --- | --- | --- | --- |
| `schemaVersion` | `schemaVersion` | unsigned integer | exactly 1 |
| `eggType` | `eggType` | unsigned integer | 0 = Default |
| `stage` | `stage` | unsigned integer | 0 Egg, 1 Child, 2 Adult |
| `location` | `location` | unsigned integer | 0 = Room |
| `ageSeconds` | `ageSeconds` | unsigned integer | post-hatch simulated seconds |
| `hunger` | `hunger` | unsigned integer | 0..100 |
| `happiness` | `happiness` | unsigned integer | 0..100 |
| `health` | `health` | unsigned integer | 0..100 |
| `energy` | `energy` | unsigned integer | 0..100 |
| `hygiene` | `hygiene` | unsigned integer | 0..100 |
| `sick` | `sick` | Boolean | true/false |
| `mess` | `mess` | Boolean | true/false |
| `nextMessAtAgeSeconds` | `nextMessAtAgeSeconds` | unsigned integer | simulation-age deadline; 0 only for Egg |
| `simulationRemainderSeconds` | `simulationRemainderSeconds` | unsigned integer | 0..59 |
| `lastUpdatedEpoch` | `lastUpdatedEpoch` | unsigned integer | UTC Unix seconds; 0 means unavailable |

Canonical fresh-state JSON, with no whitespace, is:

```json
{"schemaVersion":1,"eggType":0,"stage":0,"location":0,"ageSeconds":0,"hunger":100,"happiness":100,"health":100,"energy":100,"hygiene":100,"sick":false,"mess":false,"nextMessAtAgeSeconds":0,"simulationRemainderSeconds":0,"lastUpdatedEpoch":0}
```

Phase 8 must serialize keys in the table order for deterministic inspection, although JSON object order is not part of decoding semantics.

### Size budget

- Hard acceptance rule: `measureJson(document) < 384` bytes; the limit excludes any `String` bookkeeping and includes every serialized JSON byte but not an implicit C-string terminator.
- Use `StaticJsonDocument<512>` (or `StaticJsonDocument<TamagotchiConstants::JSON_DOCUMENT_CAPACITY_BYTES>`) in the store. Check `overflowed()` after population/deserialization.
- A full valid record using maximum 64-bit age/deadline values, five 100-valued needs, `false` booleans, and the largest supported epoch is below 300 serialized bytes with the specified keys.
- Do not increase either capacity or record limit merely to accept unknown/unbounded fields. Reject an NVS string whose length is 384 bytes or greater before parsing.
- The stored JSON contains no user strings and therefore needs no independent string-value budget.

## Forward migration boundary

Phase 8 must decode through an explicit version switch:

```text
read bounded record
  -> parse envelope and schemaVersion
  -> switch(schemaVersion)
       case 1: decode and validate every v1 field
       default: unsupported-version fallback
```

Rules for future changes:

- retain the v1 decoder when adding v2;
- deserialize into a temporary candidate, validate it fully, then publish it to the caller;
- migrate in memory from a valid older version, set the current schema version, validate again, and save only after successful conversion;
- bump the schema version for renamed keys, changed units/semantics, changed enum meanings, or new required fields;
- never reinterpret an existing enum number;
- unknown fields in a v1 object may be ignored, enabling harmless producer metadata, but cannot replace required fields;
- malformed v1, version 0, and versions newer than the firmware are not partially recovered;
- fallback logs may include only an error category and parsed schema version. Never log the raw record or arbitrary NVS contents.

For the MVP, there is no v0 migration because no Tamagotchi state has shipped. Missing, malformed, inconsistent, or unsupported data returns the canonical fresh egg. Phase 8 should attempt to persist that replacement and report save failure without discarding the in-memory fresh state.

## Implementation sequence

1. Confirm Phase 1 recorded a clean or understood baseline and preserve unrelated changes.
2. Create `src/tamagotchi/` if absent.
3. Add `TamagotchiTypes.h` with only standard-library includes.
4. Add enums with explicit underlying types and numeric assignments exactly as above.
5. Add `TamagotchiConstants` values with explicit units in comments.
6. Add `TamagotchiState` in the JSON mapping order with canonical fresh defaults.
7. Add only the model-change bit helpers required for typed flag composition/tests.
8. Add the compile-time assertions listed above.
9. Create a temporary compile-only translation unit outside the repository or use a compiler stdin invocation to instantiate the state, combine/test change flags, and assert default values where possible. Do not add a fake native PlatformIO environment.
10. Run the production PlatformIO build.
11. Review `git diff -- src/tamagotchi/TamagotchiTypes.h` and all other diffs. Phase 2 should have no generated asset, portal, LVGL, NVS, or configuration changes.
12. Record the actual `sizeof(TamagotchiState)` reported by a safe compile-time assertion/build if desired; do not turn it into a persisted binary-layout contract.

## Compile and build checkpoints

Checkpoint A — header isolation:

- a translation unit including only `TamagotchiTypes.h` compiles;
- no `Arduino.h`, `lvgl.h`, `Preferences.h`, or `ArduinoJson.h` appears in its include graph by direct inclusion;
- default construction needs no heap allocation.

Checkpoint B — contract assertions:

- enum underlying-type, interval, range, NVS-name-length, standard-layout, trivial-copy, and state-size assertions pass;
- a representative combined `TamagotchiModelChange` mask can be created and queried without integer casts at callers;
- invalid `TamagotchiAction::Count` remains distinguishable from Doctor.

Checkpoint C — firmware integration:

```sh
pio run -e adafruit_feather_esp32s3_reversetft
```

- production firmware compiles and links;
- no new warnings arise from the header;
- no unexplained generated-file modifications are accepted;
- binary-size movement should be negligible because this phase adds types/constants only.

Do not claim native tests passed. The repository has no enabled native test environment.

## Edge cases and failure handling reserved by the contract

- Missing `tamagotchi/state`: return a fresh egg; absence is normal, not corruption.
- Empty/oversized/truncated/non-object JSON: reject before field conversion.
- Integral overflow, negative number, floating-point number, stringified number, or invalid enum: reject the entire record.
- Duplicate JSON keys: Phase 8 must reject them if its parser path can detect them; otherwise document ArduinoJson's effective last-value behavior and validate the resulting candidate. Do not merge values from multiple objects.
- Unknown v1 keys: ignore, but still enforce the 384-byte record limit.
- Unsupported schema: preserve no partial gameplay state; return fresh and log only `unsupported schema <number>`.
- Invalid cross-field combination: return fresh; do not promote Child to Adult or repair a deadline during load.
- `lastUpdatedEpoch == 0`: valid pending-clock state. Phase 9 performs no offline catch-up until synchronized time exists.
- Clock moved backward: Phase 9 applies zero offline duration and safely rebaselines; the persisted state contract does not encode negative elapsed time.
- Offline interval over seven days: Phase 9 passes at most 604,800 seconds to the model, exactly once, then updates the epoch baseline.
- Age/deadline arithmetic near `uint64_t` maximum: Phase 7 saturates; it never wraps to Egg or resets needs.
- Failed save: Phase 8 returns failure; later owners retain the dirty flag and show `SAVE!` as planned rather than claiming persistence.
- Card removal/re-add: the record remains. Phase 2 defines no delete/reset API.
- Unexpected power loss between checkpoints: the last successful state plus capped epoch catch-up is authoritative.

## Deliverables

- `src/tamagotchi/TamagotchiTypes.h` with the exact stable contract and assertions;
- a successful production PlatformIO build;
- reviewed diff showing no out-of-scope source or generated changes;
- any deliberate deviation from this plan recorded before Phase 7 or Phase 8 begins.

## Exit criteria

Phase 2 is complete only when all of the following are true:

- all six enums exist with explicit underlying types and fixed numeric values;
- `TamagotchiState` includes all 15 v1 fields with the types/defaults above;
- constants cover simulation step, evolution, catch-up cap, mess cadence, all decay/penalty rules, sickness thresholds, starting values, action effects, JSON/NVS limits, and checkpoint cadence;
- fresh Egg, Child, Adult, mess, sickness, remainder, and epoch invariants are unambiguous;
- the JSON table and canonical fresh record agree exactly with the C++ fields;
- the measured worst-case valid v1 record is below 384 bytes and the 512-byte ArduinoJson capacity is retained for Phase 8;
- malformed and unsupported-version behavior is defined without raw-state logging;
- the header remains free of Arduino/LVGL/storage dependencies and within the 64-byte state budget;
- the production firmware builds in `adafruit_feather_esp32s3_reversetft`;
- no code from Phases 7-13 has been implemented early.

## Handoff assumptions

- Phase 7 may add `TamagotchiModel.h/.cpp` and outcome structs, but it must use these enums/state fields and must not change persisted meanings.
- Phase 7 owns exact step ordering and proves bulk `advanceBy(seconds)` equivalence to repeated minute steps.
- Phase 8 owns Preferences lifetime, bounded JSON parsing, strict raw-type/range validation, load/save result types, replacement writes, and error logging.
- Phase 8 opens one dedicated `tamagotchi` Preferences handle once during startup rather than adding it to `ConfigManager::commit()`.
- Phase 9 is the only authority that decides whether the live wall clock is synchronized, applies capped offline catch-up once, and updates `lastUpdatedEpoch`.
- The card/UI owns transient selector mode, result text, animation state, and save-warning state; none belongs in `TamagotchiState`.
- State writes remain on the UI task. Any future portal or background writer requires explicit synchronization and is outside MVP.
- Card removal never clears the `tamagotchi/state` key. A future explicit reset feature would require a separately authorized contract and UI.
