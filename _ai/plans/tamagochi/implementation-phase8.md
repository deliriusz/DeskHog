# Tamagotchi implementation Phase 8: dedicated state store

## Phase goal

Implement the bounded, versioned persistence boundary for `TamagotchiState` without coupling pet state to card configuration, Wi-Fi, API settings, LVGL, or wall-clock policy.

At the end of this phase:

- one long-lived store owns the `tamagotchi` Preferences handle;
- all 15 Phase 2 fields round-trip through the exact compact JSON v1 schema;
- malformed, inconsistent, oversized, and unsupported records become a canonical fresh egg;
- replacement-write failure is visible to callers, so the card never clears its dirty flag incorrectly;
- raw NVS text is neither exposed by the API nor printed in logs;
- no clock catch-up, model tuning, card UI, reset feature, or portal route is added.

## Scope and files

Create:

```text
src/tamagotchi/TamagotchiStateStore.h
src/tamagotchi/TamagotchiStateStore.cpp
```

Modify only startup/dependency wiring needed to give the store one firmware-long lifetime:

```text
src/main.cpp
```

Do not change:

- `src/ConfigManager.h` or `src/ConfigManager.cpp`;
- `cards/config_list` or any existing Preferences namespace;
- `TamagotchiTypes.h` field names, enum numbers, defaults, limits, or invariants;
- card factories, portal handlers, `EventQueue`, LVGL code, sprites, or generated assets.

If Phase 12 will introduce the final owner through a revised dependency container before Phase 8 is implemented, keep the component API below and move only the three startup wiring operations: construct, `begin()`, and pass by reference. Do not create two store instances.

## Dependencies and prerequisites

Phase 8 starts only after:

1. Phase 2's `src/tamagotchi/TamagotchiTypes.h` exists with the exact v1 fields and constants.
2. Phase 7's model accepts and returns `TamagotchiState` without changing persisted meanings.
3. The baseline worktree is recorded and unrelated user changes are understood.
4. The production environment still resolves ArduinoJson 6.21.x and ESP32 `Preferences`.

This phase does not require the card or clock service to exist. The store can compile and open its namespace before a consumer is connected. Phase 12 will inject it into the card factory; Phase 13 will apply save cadence and sleep/removal flushes.

## Public API contract

Declare the complete public surface in `TamagotchiStateStore.h`:

```cpp
#pragma once

#include <cstdint>
#include <Preferences.h>

#include "tamagotchi/TamagotchiTypes.h"

enum class TamagotchiLoadOrigin : uint8_t {
    StoredV1 = 0,
    Missing = 1,
    InvalidRecord = 2,
    UnsupportedSchema = 3,
    StorageUnavailable = 4
};

struct TamagotchiLoadResult {
    TamagotchiState state{};
    TamagotchiLoadOrigin origin = TamagotchiLoadOrigin::StorageUnavailable;
    bool persisted = false;
};

class TamagotchiStateStore {
public:
    TamagotchiStateStore() = default;
    ~TamagotchiStateStore();

    TamagotchiStateStore(const TamagotchiStateStore&) = delete;
    TamagotchiStateStore& operator=(const TamagotchiStateStore&) = delete;
    TamagotchiStateStore(TamagotchiStateStore&&) = delete;
    TamagotchiStateStore& operator=(TamagotchiStateStore&&) = delete;

    bool begin();
    void end();
    bool isReady() const;
    TamagotchiLoadResult load();
    bool save(const TamagotchiState& state);

private:
    Preferences preferences;
    bool ready = false;
};
```

Private decode/encode helpers and private status enums may be added in the `.cpp` or private class section. Do not expose:

- a mutable `Preferences&`;
- raw JSON strings or buffers;
- arbitrary namespace/key access;
- delete, clear, factory-reset, or pet-reset methods;
- a reference or pointer into a JSON document;
- a shared mutable copy of the last-loaded state.

`load()` returns a typed value copy. `save()` accepts a const typed value and does not mutate model or card state.

## Load-result semantics and dirty-state handoff

`origin` explains where the returned state came from. `persisted` independently says whether NVS contains that exact state. A valid v1 load returns `StoredV1/true`. Missing, invalid, and unsupported records return `TamagotchiState{}` with the matching origin and `persisted` equal to the replacement-save result. A store/read failure returns fresh `StorageUnavailable/false` without claiming durability.

The card integration must initialize its persistence dirty flag as:

```cpp
TamagotchiLoadResult loaded = stateStore.load();
TamagotchiModel model(loaded.state);
bool persistenceDirty = !loaded.persisted;
```

Every later save follows one rule: clear the card's dirty flag only when `save(model.getState())` returns `true`. On `false`, retain the dirty flag and the future `SAVE!` indication. The store must never clear or own that flag.

## Preferences ownership and lifecycle

Use `TamagotchiConstants::NVS_NAMESPACE` (`tamagotchi`) and `TamagotchiConstants::NVS_STATE_KEY` (`state`) only.

Lifecycle requirements:

1. Construct exactly one `TamagotchiStateStore` as firmware dependency wiring, not inside a removable card.
2. In `setup()`, after core configuration initialization and before `CardController` creates dynamic cards, call `begin()` once.
3. `begin()` is idempotent: if already ready, return `true`; otherwise call `preferences.begin(namespace, false)` and retain its result.
4. Failure to open NVS is non-fatal to the rest of DeskHog. Log a safe category and let later store operations fail visibly.
5. Keep the Preferences handle open for the store's full useful lifetime. Do not end/reopen it after writes.
6. `end()` is idempotent, calls `preferences.end()` only when ready, and clears `ready`.
7. The destructor calls `end()` for tests/orderly teardown; embedded runtime should not depend on global destructor order.
8. Never call `ConfigManager::commit()` from this store and never let that method manage this handle.

In Phase 8, `main.cpp` may hold a pointer or statically owned dependency consistent with existing wiring. Check allocation failure before dereference. Phase 12 must pass the same instance by reference into `CardController`; it must not construct a second Preferences owner in the card factory.

## Exact JSON v1 mapping

Use `StaticJsonDocument<TamagotchiConstants::JSON_DOCUMENT_CAPACITY_BYTES>`, fixed at 512 bytes. Serialize in this deterministic order:

```text
schemaVersion, eggType, stage, location, ageSeconds,
hunger, happiness, health, energy, hygiene, sick, mess,
nextMessAtAgeSeconds, simulationRemainderSeconds, lastUpdatedEpoch
```

The types/ranges are exactly Phase 2's table: unsigned integers for schema/enums/needs/remainder, unsigned 64-bit integers for age/deadline/epoch, and real JSON Booleans for `sick`/`mess`.

The canonical fresh-state encoding remains:

```json
{"schemaVersion":1,"eggType":0,"stage":0,"location":0,"ageSeconds":0,"hunger":100,"happiness":100,"health":100,"energy":100,"hygiene":100,"sick":false,"mess":false,"nextMessAtAgeSeconds":0,"simulationRemainderSeconds":0,"lastUpdatedEpoch":0}
```

Unknown v1 fields may be ignored. All listed fields are required even if their value matches a default.

## Bounded read and parse algorithm

Implement `load()` in this order:

1. Start with local `TamagotchiState fresh{}`; never publish a partially decoded candidate.
2. If `ready` is false, return fresh with `StorageUnavailable` and `persisted = false`.
3. If `state` is absent, call `save(fresh)` once and return `Missing` with `persisted` equal to that result.
4. Require the NVS key type to be `PT_STR`; another type is an invalid record.
5. Read into a zero-initialized local `char record[MAX_SERIALIZED_STATE_BYTES + 1]` using the bounded buffer overload.
6. Treat a zero read result for a present string as unreadable/oversized and invalid. Do not retry through the unbounded `String` overload.
7. Convert the returned NVS length to JSON-byte length by excluding its terminating null.
8. Reject empty JSON and every record whose JSON-byte length is greater than or equal to 384.
9. Deserialize using the explicit byte length into a local 512-byte static document.
10. Reject any ArduinoJson error, `document.overflowed()`, or non-object root.
11. Read and strictly validate `schemaVersion` before decoding other fields.
12. Dispatch through an explicit version switch. `case 1` calls the v1 decoder; `default` returns unsupported.
13. Decode into a local `TamagotchiState candidate{}` using widened unsigned temporaries.
14. Validate every scalar and then every cross-field invariant.
15. Only after complete success assign candidate to the result and mark `StoredV1`, `persisted = true`.
16. For invalid or unsupported input, call `save(fresh)` once and report whether replacement succeeded.

ArduinoJson does not provide a supported duplicate-key rejection result in this repository's normal DOM path. Document and test its effective last-value behavior: the final value retained for a duplicate key is still subjected to full type/range/invariant validation. Do not merge nested objects or values. The 384-byte limit bounds this limitation.

## Validation before narrowing

Add focused private readers such as `readUnsignedField(...)` and `readBooleanField(...)`. Their contract matters more than their exact spelling.

For each unsigned field:

- require the key to exist;
- require an actual JSON integral number accepted losslessly as `uint64_t`;
- reject null, Boolean, negative, floating-point, exponent/fraction syntax, stringified number, array, and object;
- reject values outside the field-specific widened bounds;
- only then cast to `uint8_t` or the persisted enum type.

For `sick` and `mess`, require an actual JSON Boolean; reject `0`, `1`, and string aliases. Add validation vectors to prove the ArduinoJson predicates used distinguish these cases. If the selected predicate accepts coercion, inspect the variant's native type or add a bounded token-type check; do not rely on `.as<T>()` alone.

Validate the completed candidate exactly as Phase 2 requires:

- `schemaVersion == 1`;
- `eggType == Default` and `location == Room`;
- stage is only Egg, Child, or Adult;
- every need is 0..100;
- remainder is 0..59;
- epoch is 0 or `MIN_VALID_EPOCH..MAX_VALID_EPOCH`;
- Egg has age 0, remainder 0, no sickness, no mess, and next-mess age 0;
- Child has age below `CHILD_TO_ADULT_AGE_SECONDS` and a nonzero next-mess deadline;
- Adult has age at least the evolution threshold and a nonzero next-mess deadline;
- a hatched clean state has `nextMessAtAgeSeconds > ageSeconds`;
- a hatched messy state has `nextMessAtAgeSeconds <= ageSeconds`.

Do not clamp, promote, reschedule, clear sickness, infer a missing field, or repair one field from another. Any violation rejects the whole record.

## Save algorithm

Implement `save(const TamagotchiState&)` in this order:

1. Return `false` when the store is not ready.
2. Run the same complete typed-state invariant validator used after decode; reject invalid in-memory state rather than persisting it.
3. Create a local 512-byte static document and insert all 15 fields in canonical order.
4. Cast enum values only to their explicit unsigned underlying codes.
5. Preserve all 64-bit fields as integer JSON values; do not convert them through `double`, `long`, or Arduino `String`.
6. Check `document.overflowed()` after population.
7. Compute `measureJson(document)` and require it to be nonzero and strictly less than 384.
8. Serialize into `char record[MAX_SERIALIZED_STATE_BYTES]`, leaving room for the terminator.
9. Require the serialized byte count to equal the measured count and be below the limit.
10. Call `preferences.putString("state", record)` exactly once.
11. Return `true` only when the Preferences return value equals the serialized JSON-byte count.

Do not call `end()` as a commit operation. `putString()` already commits. Do not perform a read-after-write on every save; hardware validation can verify persistence across restart without doubling routine NVS traffic.

## Version switch and future migration seam

Keep decoding structured as:

```text
bounded NVS read
  -> JSON envelope parse
  -> strict schemaVersion read
  -> switch(schemaVersion)
       case 1: decodeV1 -> validateV1 -> current state
       default: unsupported
```

There is no v0 migration because Tamagotchi state has not shipped. Version 0 and newer versions are unsupported. A future v2 must retain `decodeV1()`, fully validate v1, migrate a local copy, set/validate v2, publish only on success, and save once with durability reported separately. Never reinterpret enum numbers. Renamed keys, changed units/meanings, or new required fields require a schema bump; harmless unknown metadata does not.

## Failure branches and safe logging

Use one short component prefix and category-only messages: open/missing/wrong-type/read-or-size/parse-or-overflow/field-type/range/invariant/unsupported-version/serialize/write/replacement-write. Only unsupported-version logging includes a value: the parsed numeric schema.

Never print the raw record, JSON fragments, arbitrary NVS keys, state dumps, credentials, or the complete pet state. Field names may be logged for a validation category, but field values should not be. For unsupported data, log only the category and parsed schema version.

Open/read failure returns fresh/unpersisted and does not stop DeskHog. Missing, corrupt, or unsupported data attempts one fresh replacement. Replacement failure leaves fresh authoritative in memory and dirty. Invalid live state causes no write; ordinary write failure preserves the prior NVS record and caller dirty.

This phase does not remove the key. Card removal, reorder, re-add, OTA, and restart must preserve it.

## Threading, UI, and lifetime rules

- `begin()` runs during startup wiring before the dynamic card consumer exists.
- After card integration, `load()` and `save()` run only from the LVGL/UI task.
- The store never calls LVGL and never owns UI labels or the `SAVE!` state.
- The store creates no task, timer, event subscription, callback, or queue message.
- Phase 9 may change `lastUpdatedEpoch` in a typed state, but it must request persistence on the UI task.
- Portal and OTA tasks must not call this store.
- If a future background or portal writer is added, introduce explicit synchronization and define ownership before enabling it; Phase 8's no-mutex design is not thread-safe.
- A removable `TamagotchiCard` borrows the store and must never delete or close it.

## Implementation sequence

1. Recheck `git status`; add the result types and non-copyable store declaration.
2. Implement lifecycle/readiness and one shared typed-state validator.
3. Implement strict scalar readers, `decodeV1()`, and version dispatch.
4. Implement bounded load plus fresh replacement.
5. Implement fixed serialization and exact write-result checking.
6. Add one long-lived startup instance; do not connect card behavior early.
7. Exercise validation vectors, run the production build, and review all diffs.

## Validation vectors

Exercise at minimum:

- canonical fresh byte-for-byte round-trip; valid hatch Child, threshold-minus-step Child, threshold Adult, clean/mess deadline relations, need/remainder/epoch boundaries, and maximum cross-field-valid 64-bit Adult;
- each missing key; wrong scalar types and aliases; negatives/fractions; enum/schema errors; need 101, remainder 60, and invalid epochs;
- every Egg/stage/deadline invariant violation;
- empty/whitespace/array/truncated/trailing-malformed JSON;
- lengths 383, 384, and above; document overflow; duplicate-key final-value behavior;
- absent/wrong-type/open/read/write/replacement failures and invalid typed-state save;
- unrelated `wifi_config`, `cards`, and `insights` data unchanged.

Where Preferences failure injection is impractical on target, isolate encode/decode/validate helpers enough for deterministic checks and validate actual NVS success/failure paths on hardware. Do not add a fake enabled native PlatformIO environment in this phase.

## Build and hardware checkpoints

Run:

```sh
pio run -e adafruit_feather_esp32s3_reversetft
```

Confirm fixed-buffer stack cost, exact 64-bit JSON, warnings, generator diffs, and the `0x1F0000` slot limit. On hardware verify fresh creation, non-default reset survival, invalid/unsupported replacement, dirty retention after forced write failure, unrelated NVS isolation, and no erase on later card removal/re-add.

Do not claim offline catch-up here; Phase 9 owns synchronized epoch meaning. Do not claim complete save cadence or sleep safety; Phase 13 owns those call sites.

## Deliverables

- `src/tamagotchi/TamagotchiStateStore.h` with the exact typed public boundary;
- `src/tamagotchi/TamagotchiStateStore.cpp` with bounded v1 encode/decode, validation, replacement, logging, and Preferences lifecycle;
- minimal `src/main.cpp` lifetime wiring for one initialized store instance, unless the already-approved dependency owner lands first;
- validation evidence for canonical, boundary, corrupt, unsupported, and failed-write cases;
- successful production build and reviewed diff.

## Exit criteria

Phase 8 is complete only when:

- the store exclusively owns one open `tamagotchi` Preferences handle;
- ConfigManager and all existing namespaces are unchanged;
- all 15 fields serialize in Phase 2 order and decode through `case 1`;
- reads and writes use fixed buffers/documents and enforce `< 384` bytes;
- raw types and ranges are checked before any narrowing conversion;
- every Phase 2 state invariant is enforced without repair or clamping;
- missing, corrupt, and unsupported records return fresh state and attempt one replacement save;
- the result reports whether the returned state is truly persisted;
- every save failure returns false so a caller can retain dirty state;
- unsupported schemas and failures are logged without raw-state disclosure;
- no delete/reset API, cross-task access, clock policy, LVGL behavior, or extra task was added;
- reset testing demonstrates isolation from Wi-Fi/API/insight/card NVS;
- the production firmware builds within the OTA slot.

## Handoff to clock and card phases

Phase 9 receives a typed `TamagotchiState`; it alone decides whether wall time is trustworthy, applies at most seven days of catch-up exactly once, handles backward time as zero elapsed, and updates the epoch baseline. It must not parse or write raw JSON.

Phase 12 injects the already-open store by reference into the singleton card factory. On card creation, load once, build the model from `result.state`, and set persistence dirty to `!result.persisted`. The card does not own or close the store.

Phase 13 calls `save()` after hatch, applied actions, important transitions, checkpoints, removal, and pre-sleep advancement. It clears dirty only on true, retains state across removal/re-add, and never invokes the store from OTA/background tasks.

Any future explicit pet reset, cloud sync, portal editor, schema migration, or background writer requires a separate contract. None is implied by this store API.
