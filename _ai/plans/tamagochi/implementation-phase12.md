# Tamagotchi implementation Phase 12: singleton card registration

## Outcome

Phase 12 makes the Phase 10 `TamagotchiCard` a normal configurable DeskHog card
while enforcing the singleton promise in both the HTTP write path and runtime
reconciliation.

At the end of this phase, `CardType::TAMAGOTCHI` has the stable external spelling
`"TAMAGOTCHI"`; unknown type strings are rejected; the catalog and factory expose
one no-config card using the boot-lifetime store and clock; POST validation is
atomic; and `allowMultiple == false` is enforced at the API and runtime boundaries.
Crafted duplicate singleton records instantiate only their first ordered occurrence
without rewriting NVS. Provisioning remains index 0 and reconciliation remains the
existing LVGL-task full rebuild.

This phase does not implement pet behavior, sprites, input policy, save cadence,
deep-sleep flushing, a pet reset route, or a new portal UI.

## Prerequisites and contracts consumed

Begin only after the following phase outputs exist, adapting spelling only when the
implemented API differs while preserving these ownership rules:

- Phase 8 provides one initialized `TamagotchiStateStore` whose `load()` and
  `save()` use namespace `tamagotchi`; a removable card borrows it and never calls
  `end()` or constructs another store.
- Phase 9 provides one initialized `ClockService`; the card borrows it, polls it on
  the LVGL task, and never subscribes itself to `EventQueue`.
- Phase 10 provides `TamagotchiCard(lv_obj_t*, TamagotchiStateStore&,
  ClockService&)`, `getCard()`, `handleButtonPress()`, `update()`, and
  `prepareForRemoval()`. The card owns its model/session, loads once, borrows its
  dependencies, and does not delete a stack-owned root after removal preparation.
- The production card stack still uses provisioning index 0 and dynamic indices
  beginning at 1.

Before editing, record `git status --short` and preserve unrelated changes. If a
prerequisite is absent, implement this phase after it rather than adding temporary
duplicate services or a placeholder Tamagotchi class.

## Scope and exact file list

Modify:

- `src/config/CardConfig.h`;
- `src/ui/CardController.h`;
- `src/ui/CardController.cpp`;
- `src/ui/CaptivePortal.h`;
- `src/ui/CaptivePortal.cpp`;
- `src/ConfigManager.cpp`;
- `src/main.cpp`;
- `docs/cards.md`;
- `docs/captive-portal.md`;
- `docs/configuration-and-state.md`.

Modify `src/ConfigManager.h` to publish explicitly named card-list limits used by
storage and the HTTP boundary, for example
`CARD_CONFIG_JSON_CAPACITY_BYTES = 2048`,
`MAX_CARD_CONFIG_BODY_BYTES = 2048`, `MAX_CONFIGURED_CARDS = 16`, and
`MAX_CARD_FIELD_BYTES = 64`, and retain the existing Boolean save-result contract.
Keep document capacity and serialized-body limits as separate names even where
their values match. Do not expose Preferences or card-definition knowledge from
`ConfigManager`.

Do not modify portal JS/generated HTML, model rules, state schema, animations,
button modes, `EventQueue`, card-stack ownership, partition/dependency settings, or
generated assets. Card configuration changes must not mutate the `tamagotchi` NVS
namespace. The current UI already consumes definitions and treats
`allowMultiple` as a client-side convenience.

## 1. Stable card type conversion

In `src/config/CardConfig.h`:

1. Append `TAMAGOTCHI` after `PADDLE` in `enum class CardType`. Appending avoids
   renumbering existing enum values even though persisted data should use strings.
2. Add `case CardType::TAMAGOTCHI: return "TAMAGOTCHI";` to
   `cardTypeToString()`.
3. Add a non-fallback API:

   ```cpp
   inline bool tryStringToCardType(const String& str, CardType& result);
   ```

4. Match only the exact, case-sensitive stable strings for every known type.
5. Assign `result` only on success; leave the caller's value unchanged on failure.
6. Reimplement the legacy `stringToCardType()` as a wrapper around the new helper,
   retaining its historical `INSIGHT` fallback for source compatibility only.
7. Mark that wrapper as legacy in its comment. New parsing boundaries must call
   `tryStringToCardType()` directly.

Implement the helper as one explicit comparison per stable spelling, including
`"TAMAGOTCHI"`; return immediately after assigning the matching enum and return
`false` without touching `result` at the end. The legacy wrapper should initialize
a local to `INSIGHT`, call the helper, and return the local. This preserves the old
source-level fallback without letting HTTP or NVS ingestion accidentally opt into
it.

Do not accept aliases, lowercase spellings, whitespace trimming, numeric enum
codes, or `"UNKNOWN"`. This is an API/storage contract, not a fuzzy UI parser.

Update `ConfigManager::getCardConfigs()` to treat NVS as an untrusted ingestion
boundary and use the new try path. Require the root to be an array and each retained
item to be an object with an exact string `type` and an actual integral `order`
representable as `int` (not Boolean, string, fraction, array, or object). Accept
missing `config` and `name` as empty for compatibility with the POST contract, but
if present require them to be strings no longer than 64 bytes. Known legacy order
values may be negative, duplicated, or gapped here: retain them for deterministic
runtime sorting/filtering rather than silently normalizing or rewriting NVS. For a
missing/wrong-type/unknown field, log only the stored array index and category,
skip that entry, and continue. Never reinterpret it as an insight and never call
`saveCardConfigs()` from a read. A malformed root/document still yields no dynamic
configs without altering the stored bytes.

## 2. Dependency ownership and startup wiring

Keep both Tamagotchi dependencies alive for the entire firmware boot.

In `src/main.cpp`:

1. Include `tamagotchi/TamagotchiStateStore.h` and `time/ClockService.h` if their
   phase implementations do not already add the includes.
2. Declare one `TamagotchiStateStore* tamagotchiStateStore` and one
   `ClockService* clockService` with the other boot-lifetime component pointers.
3. Construct/start `ClockService` after `eventQueue->begin()` and before
   `wifiInterface->begin()`, as required by Phase 9.
4. Construct `TamagotchiStateStore` after core `ConfigManager::begin()` and call
   `begin()` before `CardController::initialize()` can create dynamic cards.
5. A store `begin()` failure is non-fatal: the card can load fresh/unpersisted and
   expose Phase 10's `SAVE!` state. Never create a replacement store in a factory.
6. Check pointer allocation before dereferencing. A missing required object must
   take the repository's explicit startup-failure path, not continue with a null
   reference.
7. Pass `*tamagotchiStateStore` and `*clockService` to `CardController`.
8. Continue passing `*clockService` to the Phase 9 `OtaManager` constructor.

Do not add a Tamagotchi task, clock task, store mutex, global pet model, or direct
pet method call in `main.cpp`.

## 3. `CardController` constructor and members

In `src/ui/CardController.h`:

- include `ui/TamagotchiCard.h`, or forward-declare it if the `.cpp` can own the
  complete-type include;
- include/forward-declare `TamagotchiStateStore` and `ClockService` consistently;
- append `TamagotchiStateStore& tamagotchiStateStore` and
  `ClockService& clockService` to the constructor;
- add references with those names beside the other system dependencies;
- do not add a special `TamagotchiCard*` legacy member or getter;
- keep `CardInstance { InputHandler* handler; lv_obj_t* lvglCard; }` and
  `dynamicCards` as the sole instance tracking mechanism.

In `src/ui/CardController.cpp`, update the constructor definition and initializer
list in declaration order. The controller borrows both dependencies and never ends,
deletes, or reinitializes them.

Update every `CardController` construction site. Repository search should show only
the startup site unless tests add a fixture.

## 4. Register the Tamagotchi definition and factory

In `CardController::initializeCardTypes()`, register exactly one definition:

```text
type              CardType::TAMAGOTCHI
name              Tamagotchi
allowMultiple     false
needsConfigInput  false
configInputLabel  empty string
uiDescription     Hatch and care for a tiny desk companion
```

The factory captures `this` like the existing definitions and ignores the empty
configuration value. Its exact flow is:

1. Construct `TamagotchiCard(screen, tamagotchiStateStore, clockService)`.
2. Check the wrapper pointer, `getCard()`, and `lv_obj_is_valid(getCard())`.
3. Only after all checks pass, append
   `CardInstance{newCard, newCard->getCard()}` to
   `dynamicCards[CardType::TAMAGOTCHI]`.
4. Register that root/handler pair through
   `cardStack->registerInputHandler(newCard->getCard(), newCard)`.
5. Return the LVGL root for the reconciliation loop to add to the stack.
6. On failure, delete the wrapper and return `nullptr`; do not track/register a
   partial instance.

Do not call `stateStore.load()` in the factory; Phase 10's constructor already loads
once. Do not request time sync, save NVS, publish an event, or add the root directly
to the stack from the factory.

The definition automatically appears in `GET /api/cards/definitions`; no
Tamagotchi-specific portal markup is needed.

## 5. Shared definition lookup and singleton policy

Use registered definitions as the single source of multiplicity/config-input
metadata. Add an exact read-only public lookup so the portal does not duplicate the
catalog or hold a pointer into the controller's vector:

```cpp
bool tryGetCardDefinition(CardType type, CardDefinition& result) const;
```

Assign `result` only on a match. Returning a value copy is deliberate: registration
finishes during controller initialization, but the HTTP callback runs later and
must not depend on vector-address stability. Reconciliation may use the same API or
a private pointer lookup while the immutable vector is in scope. `INSIGHT` and
`HELLO_WORLD` remain repeatable; `FRIEND`, `FLAPPY_HOG`, `QUESTION`, `PADDLE`, and
`TAMAGOTCHI` allow one each. Future definitions inherit the same behavior from
`allowMultiple`.

Do not implement only a Tamagotchi string comparison. The portal and reconciliation
must make the same decision from `CardDefinition`.

## 6. Atomic portal POST validation

Replace the permissive parsing in
`CaptivePortal::handleSaveConfiguredCards()` with validate-then-save behavior.

Add focused private helpers/types in `CaptivePortal.h` or file-local equivalents:

```cpp
void handleConfiguredCardsBody(
    AsyncWebServerRequest* request,
    uint8_t* data,
    size_t len,
    size_t index,
    size_t total
);
void sendCardConfigResponse(...);
```

Store expected size, received size, a buffer pointer, and a body-status enum in a
small request-body object in `request->_tempObject`. The status must distinguish at
least `Receiving`, `Complete`, `Empty`, `TooLarge`, `AllocationFailed`, and
`InvalidChunk`, so the request handler can choose the correct response without
examining uninitialized memory. The callback must:

1. Allocate state only for the first chunk (`index == 0`).
2. Reject `total == 0` and `total > 2048` without allocating a `total`-sized buffer.
3. Guard `total + 1`, `index + len`, and every copy boundary against overflow.
4. Require every callback's `total` to match the initial total.
5. Require chunks to be contiguous (`index == received`) and within `total`.
6. Allocate exactly `total + 1`, copy each chunk at `buffer + index`, and append a
   null terminator only when all `total` bytes have arrived.
7. Preserve an error sentinel after oversize, allocation, ordering, or bounds
   failure so later chunks cannot retry or overwrite it.
8. Mark `Complete` only when `received == expected == total`; do not deserialize in
   the body callback.
9. Let the request handler first detach the state by setting `_tempObject` to null,
   then own and free the state/buffer exactly once on every response path.

The terminal request handler must reject a null/incomplete body state rather than
assuming the final body callback ran. If allocation of the state object itself
fails and therefore no sentinel can be stored, distinguish an empty request from
allocation/callback failure using the request's declared content length: zero is
`empty_body`; nonzero with no state is `body_allocation_failed`. Parse with the
stored byte length, not by
constructing an unbounded `String` or calling `strlen()`. Keep the JSON document
alive for the complete validation loop; copy accepted strings into owning
`String` fields on `CardConfig`, and let no `JsonVariant`, `JsonObject`, or pointer into
the request buffer escape into the candidate vector, `ConfigManager`, a callback,
or a later task. Free the detached request buffer only after parsing and candidate
construction are complete.

Never allocate from `len`, overwrite state per chunk, assume one chunk, parse an
incomplete buffer, or log submitted JSON. Remove the raw-body serial log. Register
the member body handler for the POST and its existing-style CORS OPTIONS route. Keep
the bounded write synchronous; do not add it to the portal action queue.

## 7. Exact payload validation rules

After a complete bounded body is available, deserialize exactly `expected` bytes
into a 2048-byte ArduinoJson document and reject parse error or
`document.overflowed()`. Build a local candidate vector only; reserve no more than
the validated 16-entry cap and never mutate stored/current configuration during
parsing.

Validate the entire array in source order:

1. Root must be a JSON array with no trailing malformed content.
2. Limit the array to 16 cards so vector/JSON work remains bounded.
3. Every item must be an object.
4. `type` and `order` are required.
5. `type` must be a JSON string accepted by `tryStringToCardType()` and must have a
   matching registered `CardDefinition`.
6. `order` must be a JSON integer, not Boolean/string/float; require `0 <= order <
   array.size()`.
7. Orders must be unique and together form the exact contiguous permutation
   `0..array.size()-1`. Reject gaps, duplicates, negatives, and huge values.
8. `config`, when present, must be a JSON string of at most 64 bytes. Missing stays
   backward-compatible and becomes empty.
9. For `needsConfigInput == true`, require 1..64 bytes and at least one
   non-whitespace character. Preserve the supplied bytes; do not silently trim or
   normalize persisted identifiers.
10. For `needsConfigInput == false`, require the config value to be empty. This
    includes Tamagotchi; it cannot smuggle pet state into `cards/config_list`.
11. `name`, when present, must be a JSON string of at most 64 bytes; missing remains
    backward-compatible and becomes empty. It need not equal the catalog name.
12. Ignore unknown object keys for forward-compatible metadata, but never use them
    to repair a required field.
13. Track counts by parsed `CardType`. On the second occurrence of any definition
    with `allowMultiple == false`, reject the whole request.

Only after all entries pass may the handler call
`_configManager.saveCardConfigs(candidateConfigs)` once. A validation failure makes
zero NVS calls and publishes zero `CARD_CONFIG_CHANGED` events. A storage failure
must not be reported as success.

Validation order is part of the contract: validate the current entry's object,
required fields, scalar types, type lookup, order, config/name, and then singleton
count; after all entries pass, validate that the collected order bitmap is the
exact permutation; only then save. Thus a second Tamagotchi returns HTTP 400
`duplicate_singleton` with that second entry's `index` and `field: "type"` before
any Preferences call, even when the first entry was otherwise valid. Do not invoke
the factory or reconcile cards from the HTTP callback.

Harden `ConfigManager::saveCardConfigs()` sufficiently for that promise:

- reject an enum whose `cardTypeToString()` is `"UNKNOWN"`;
- detect JSON document overflow/failed nested-object creation;
- require nonzero serialization within the configured capacity;
- require `putString()` to report the full serialized byte count;
- publish `CARD_CONFIG_CHANGED` only after the successful write;
- return `false` on every failure.

Do not make `ConfigManager` depend on `CardController` or `CardDefinition`; catalog
validation belongs at the HTTP boundary and runtime reconciliation.

## 8. HTTP response contract

Keep the top-level `success` and `message` fields so existing portal JavaScript
continues to work. Use this failure shape:

```json
{
  "success": false,
  "message": "Card type may only appear once",
  "error": {
    "code": "duplicate_singleton",
    "index": 3,
    "field": "type"
  }
}
```

`index` and `field` are optional when an error is request-wide. Never include the
raw body, config value, or arbitrary JSON fragment.

Use these status classes and stable codes:

| HTTP | Codes |
| ---: | --- |
| 200 | successful save (`success: true`, message, and `count`) |
| 400 | `empty_body`, `invalid_json`, `invalid_root`, `too_many_cards`, `invalid_entry`, `missing_field`, `invalid_field_type`, `unknown_card_type`, `unregistered_card_type`, `invalid_order`, `invalid_config`, `invalid_name`, `duplicate_singleton` |
| 413 | `payload_too_large` |
| 415 | `unsupported_media_type` when a non-JSON content type is explicitly supplied |
| 500 | `body_allocation_failed`, `storage_write_failed`, or response-allocation failure where a response is still possible |

A valid empty array returns 200 and removes all configurable cards, leaving the
provisioning card. HTTP 202 is not used because this route writes synchronously.
Every response, including early body errors, uses `application/json`, adds the
existing permissive CORS header, and is sent exactly once. The success shape is
`{"success":true,"message":"Card configuration saved successfully","count":N}`.
The duplicate response is exactly the failure envelope above (with the second
occurrence's array index), not a 200 carrying `success:false`.

## 9. Defensive runtime reconciliation

Portal validation cannot be the only guard because NVS may predate this firmware or
be crafted outside the route.

In `CardController::reconcileCards()`:

1. Copy `newConfigs` and apply `std::stable_sort` by `order`. Equal legacy orders
   retain their stored array order, defining an explicit tie-breaker.
2. Resolve each type against `registeredCardTypes`.
3. Maintain a local set/list of singleton types already accepted.
4. For `allowMultiple == false`, accept the first item in stable sorted order and
   skip every later item of that type.
5. Log only the type and skipped source/runtime position; do not log config data.
6. Skip unknown/unregistered typed values and missing factories defensively.
7. Use the resulting effective list/count for `hasNewCard`, creation, and navigation
   decisions so skipped duplicates do not look like successfully added cards.
8. Do not call `saveCardConfigs()` from reconciliation. Runtime filtering is not a
   migration and must never rewrite user NVS behind their back.

Keep the existing lifecycle:

- dispatch the full operation to the UI callback queue;
- take the display mutex in LVGL context;
- call `prepareForRemoval()` on every dynamic handler;
- let `CardNavigationStack::removeCard()` delete each LVGL root;
- delete wrappers and clear `dynamicCards`;
- retain the separately owned provisioning card/root;
- recreate accepted dynamic cards through factories;
- refresh indicators and restore/select a valid index;
- provisioning remains index 0, so dynamic targets retain the existing `+1` offset;
- release the mutex and clear `reconcileInProgress` on all failure exits.

Build the effective, stable-sorted list before deleting any live dynamic card, but
still perform the existing all-or-nothing full teardown/recreate rather than an
in-place diff. Because `dispatchToLVGLTask()` currently returns `void`, change it to
return `bool` (existing callers may ignore the result): return false when the UI
queue/callback allocation/send fails. `reconcileCards()` must clear
`reconcileInProgress` immediately when dispatch fails; the queued lambda must use a
single cleanup path that releases a taken display mutex and clears the flag. This
prevents one queue-full event from permanently suppressing later configuration
reconciliation.

When calculating navigation, count only roots that factories actually returned.
Provisioning remains the separately owned root at index 0; each accepted dynamic
root is appended in effective order and its target index is `createdCount + 1`.
Clamp restoration to `0..createdCount`. A skipped duplicate, missing definition,
missing factory, or failed factory must neither increment the created count nor be
selected as a newly added card.

Do not convert reconciliation to an in-place diff. Do not delete the Tamagotchi NVS
record when its card config is removed or skipped.

## 10. Compatibility and failure branches

Known strings and the GET schemas keep their meanings; `config`/`name` remain
optional; repeatable cards remain repeatable; and the existing portal UI needs no
asset change. Duplicate singleton NVS is preserved but rendered once. Removing or
re-adding Tamagotchi changes only `cards/config_list`.

| Failure or edge | Required result |
| --- | --- |
| Unknown POST type | 400; no fallback insight; no write/event |
| Known enum without registered definition | 400 `unregistered_card_type`; no write |
| Duplicate Tamagotchi | 400 `duplicate_singleton`; old NVS remains |
| Duplicate other singleton | Same catalog-wide rejection |
| Duplicate repeatable type | Allowed if every entry/config/order is valid |
| Missing/invalid/gapped order | 400; no normalization and no partial write |
| Nonempty Tamagotchi config | 400 `invalid_config`; pet state stays separate |
| JSON/doc overflow | 400 or 413 according to body size; no write |
| Multi-chunk body | Reassembled once by offsets; same result as one chunk |
| Repeated/out-of-order chunk | 400; buffer freed once; no write |
| Body allocation failure | 500; no parse/write |
| NVS write short/fails | 500; no success claim and no change event |
| Event queue full after a successful NVS save | Return success because persistence succeeded; log the dropped change event and rebuild on the next load/event opportunity |
| UI callback queue full during reconciliation | Drop no live cards, clear `reconcileInProgress`, log safely, and allow a later event/reboot to retry |
| Factory allocation/root failure | Skip that runtime card; no partial tracking/handler registration |
| Crafted duplicate NVS | First stable ordered singleton created; later duplicates skipped; NVS unchanged |
| Equal legacy orders | Original stored array order is the deterministic tie-breaker |
| Store unavailable | Card may exist with unpersisted state; no second store created |
| Clock unavailable | Card exists and shows Phase 10 `TIME?`; no second clock owner |
| Empty valid array | All dynamic cards removed; provisioning remains at index 0 |

## 11. Verification

### Static review

- Confirm all type switches cover Tamagotchi and ingestion uses the try API.
- Confirm exactly one store/clock and no factory-side load or partial registration.
- Confirm singleton checks derive from `allowMultiple`, not type conditionals.
- Confirm chunk arithmetic, the 2048-byte cap, cleanup, and no raw-body logs.
- Confirm reconciliation never saves and provisioning never enters `dynamicCards`.
- Confirm `dispatchToLVGLTask()` reports enqueue failure and every reconcile path
  clears its in-progress flag without leaking/taking the display mutex twice.

### HTTP matrix on a running Feather

Use `curl` or an equivalent client and inspect both HTTP code and JSON:

1. `[]` -> 200, count 0, provisioning still visible.
2. One `TAMAGOTCHI` with empty/missing config and order 0 -> 200.
3. Two Tamagotchi entries -> 400 `duplicate_singleton`, prior GET unchanged.
4. Two `FRIEND` entries -> the same 400 policy.
5. Two valid `INSIGHT` entries with distinct orders -> 200.
6. Unknown/misspelled/lowercase/numeric type -> 400, never an insight.
7. Missing type/order and wrong scalar types -> one 400 per vector.
8. Negative, duplicate, gapped, floating, string, Boolean, and huge orders -> 400.
9. Missing/empty/whitespace/over-64 insight config -> 400.
10. Nonempty Tamagotchi config and non-string optional fields -> 400.
11. Malformed JSON, object root, null root, and trailing garbage -> 400.
12. Sixteen valid entries within singleton limits -> accepted; seventeenth -> 400.
13. Exactly-at-limit and over-limit bodies -> expected validation/413 behavior.
14. Send the same valid JSON in multiple transport chunks -> 200 and exact round trip.
15. Force a Preferences write failure -> 500; no event/rebuild success claim.

After every rejected request, compare `GET /api/cards/configured` byte-for-byte or
semantically with the prior value and confirm there was no full card rebuild.

### Crafted persistence and lifecycle cases

1. Seed duplicate Tamagotchi records with orders 4 and 1; only order 1 is created.
2. Seed equal-order duplicate singletons; the earlier stored array entry wins.
3. Seed duplicates for each existing singleton; one of each is created.
4. Seed unknown type text; it is skipped, not converted to `INSIGHT`, and NVS is not
   rewritten.
5. Reboot twice and confirm the same deterministic winners.
6. Add, reorder, remove, and re-add Tamagotchi; pet state resumes from its separate
   store and provisioning stays at index 0.
7. Run at least 20 valid saves/full rebuilds while watching heap/PSRAM and input
   handler behavior for leaks, double deletion, or duplicate registration.

### Build

Run only the configured production environment:

```sh
pio run -e adafruit_feather_esp32s3_reversetft
```

Report warnings, final RAM/flash usage, and the result against the `0x1F0000` OTA
application slot. There is no enabled native test environment; do not claim native
tests passed unless one is deliberately added and run. Record HTTP, NVS corruption,
and lifecycle checks as hardware-required where applicable.

## Deliverables

- Stable `TAMAGOTCHI` type/string mapping and non-fallback parse API.
- Boot-lifetime store/clock dependencies injected through `CardController`.
- Catalog definition and correctly owned factory/tracking/input registration.
- Bounded chunk-safe body assembly and atomic full-array validation.
- Honest HTTP status/error responses and checked storage result.
- Catalog-wide singleton enforcement in POST and defensive reconciliation.
- Updated card, portal, and configuration documentation.
- Successful production build plus recorded hardware-only validation gaps.

## Exit criteria

Phase 12 is complete only when:

1. `TAMAGOTCHI` round-trips exactly and unknown text cannot become an insight at an
   ingestion boundary.
2. The catalog exposes the exact Tamagotchi metadata and no config input.
3. The factory uses the one state store and one clock service, loads only through
   the card constructor, and registers one valid handler/root pair.
4. The portal rejects every malformed entry, invalid order/config, unregistered
   type, and duplicate singleton before any write.
5. HTTP codes distinguish success, client validation, oversize/media, and storage
   failure while preserving `success`/`message` compatibility.
6. Multi-chunk and allocation failure paths are bounded and free memory exactly
   once.
7. A crafted NVS list deterministically creates only the first ordered instance of
   every singleton definition and is not rewritten by reconciliation.
8. Repeatable definitions continue to create multiple instances.
9. Full rebuild, UI-task/mutex ownership, input registration, and provisioning
   index 0 remain intact.
10. Removing/re-adding the card never deletes its separate pet state.
11. Production firmware builds within the OTA slot and hardware-only cases are
    explicitly recorded.

## Handoff to Phase 13

Phase 13 may now assume at most one live Tamagotchi card. It must route hatch,
actions, transitions, checkpoints, removal, and pre-sleep saves through Phase 10's
baseline-aware save helper; add `prepareForSleep()` ownership without changing this
portal policy; and preserve the pet record when card configuration is removed.

Phase 13 must not add another state store, clock path, singleton check, HTTP parser,
or background card callback. Any future pet reset or portal state editor requires a
separate authenticated/validated contract and explicit concurrency design.
