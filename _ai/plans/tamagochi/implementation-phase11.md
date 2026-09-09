# Tamagotchi implementation Phase 11: three-button interaction

## Phase goal

Implement the complete edge-driven three-button policy inside `TamagotchiCard`.
The card must hatch the egg, expose a five-action selector, execute/refuse model
actions deterministically, hand accepted actions to Phase 10's visual helpers, and
request immediate persistence without taking ownership of Phase 13's save cadence.

## Preconditions

Consume the Phase 2 action/result contracts, Phase 7 model API, Phase 9 session
time/save boundary, and Phase 10 LVGL objects/render/visual helpers unchanged.
Phase 11 replaces Phase 10's temporary non-consuming `handleButtonPress()`.

The implementation must be checked against the repository behavior, not a generic
three-button abstraction: `Input::BUTTON_DOWN`, `BUTTON_CENTER`, and `BUTTON_UP`
are the values forwarded by `lvglHandlerTask`; `CardNavigationStack` calls the
active card first and applies down/next or up/previous only when the handler
returns false. Center has no stack default.

## Scope boundaries

Modify only `src/ui/TamagotchiCard.h` and `src/ui/TamagotchiCard.cpp`.

Do not modify `Input`, `InputHandler`, `CardNavigationStack`, or `main.cpp` for
this phase. Do not add card registration, card-type conversion, portal routes,
singleton validation, or factory wiring; those belong to Phase 12. Do not add the
pre-deep-sleep persistence hook, removal flush, periodic checkpoint policy, or
save retry policy; those belong to Phase 13.

Do not create a task, event, queue, mutex, timer, completion callback, mini-game,
cancel chord, long-press gesture, or held-input behavior.

## Existing dispatch behavior to preserve

`lvglHandlerTask` updates Bounce2 about every 50 ms and checks center+down before
forwarding `.pressed()` edges. The chord branch forwards nothing. For other edges,
the active handler receives first refusal: true suppresses defaults, while false
maps down/up to next/previous card. Handler and active-card `update()` calls are
serialized on the LVGL task, so pet input needs no event or lock.

## Exact transient state

Add the mode enum in `TamagotchiCard.h`:

```cpp
enum class TamagotchiInteractionMode : uint8_t {
    Egg,
    Normal,
    ActionSelector
};
```

Add these private members:

```cpp
TamagotchiInteractionMode _interactionMode;
TamagotchiAction _selectedAction;
bool _immediateSaveRequested;
```

Phase 10 already owns `_currentVisual`, one-shot/deadline state,
`_resultText[24]`, `_resultDeadlineMillis`, and cached rendered model values. Reuse
those result fields; do not add a second result enum/visibility/deadline. Interaction
mode, selection, result text/deadline, and save request are transient and must never
be added to `TamagotchiState` or the version-1 JSON record.

After model construction initialize Egg/Normal from the loaded stage, selection to
Feed, the Phase 10 result buffer/deadline to empty/zero, and the immediate save
request from the `InitialSession.persistenceDirty` value derived from
`!loadResult.persisted` in the delegating constructor.

Never persist the selected action. Every transition into ActionSelector resets it
to Feed, giving the user a stable starting position each time it opens.

## Selector order and conversion

The only legal circular order is:

```text
Feed -> Play -> Clean -> Rest -> Doctor -> Feed
```

Down advances in that direction. Up moves in the reverse direction, including
Feed -> Doctor. Implement the mapping with an explicit five-entry `constexpr`
array or exhaustive switch. Do not depend on arbitrary enum casts without first
checking the value against `TamagotchiAction::Count`.

Required helpers:

```cpp
void setInteractionMode(TamagotchiInteractionMode mode);
void moveSelection(int8_t direction);
void executeSelectedAction(uint32_t nowMillis);
void requestImmediateSave();
void setTransientResult(TamagotchiAction action,
                        TamagotchiActionResult result,
                        uint32_t nowMillis);
void expireTransientResult(uint32_t nowMillis);
void renderInteractionUi();
static bool deadlineReached(uint32_t nowMillis, uint32_t deadlineMillis);
```

`direction` is exactly `+1` for down and `-1` for up. `moveSelection()` repairs an
invalid current selection to Feed before moving only if corruption is observed;
normal operation never creates `Count`.

## Transition and consumption table

| Mode/guard | Button | State/model effect | Return |
| --- | --- | --- | ---: |
| Egg | Up | none; global previous-card navigation | false |
| Egg | Down | none; global next-card navigation | false |
| Egg | Center | call `hatch()`; on success enter Normal | true |
| Normal | Up | none; global previous-card navigation | false |
| Normal | Down | none; global next-card navigation | false |
| Normal | Center | reset selection to Feed; open selector | true |
| Normal + evolve visual active | Center | keep Normal and evolution; show `GROWING` | true |
| ActionSelector | Up | select previous action with wrap | true |
| ActionSelector | Down | select next action with wrap | true |
| ActionSelector | Center | execute selection; return to Normal | true |
| Any mode | unknown index | no state or UI change | false |

Apply lifecycle/removal guards before this table. If `prepareForRemoval()` has
invalidated the root, return false without sampling time, touching the model,
requesting a save, or dereferencing LVGL state. Otherwise sample `millis()` once,
expire due transient deadlines, advance same-boot time through that sample, and
reconcile the mode with the model stage before interpreting the edge. `Egg` is
legal only while the model stage is Egg. A stale Egg mode with a Child or Adult is
repaired to Normal and must never call `hatch()` as a reset.

Up/down return false in Egg and Normal even while an action/result animation,
`TIME?`, `SAVE!`, sickness, or mess is visible. This is the hard escapability
guarantee. The selector is the only condition that consumes up/down.

There is no selector cancel gesture in the MVP. Center executes and closes it;
the user can always leave the card before opening it or immediately after an
execution. Do not assign a simultaneous press or long hold to cancel.

## Center behavior in detail

### Egg center

Sample `millis()` once, advance session bookkeeping, and call `_model.hatch()` once.
On a Stage change, enter Normal, request a save, render returned changes, select
child idle (never Evolve), and refresh interaction/status UI. On a stale-mode no-op,
derive mode from actual stage, force a coherent render, and do not request a save.
Return true in either case.

### Normal center

First service expired visual/result deadlines with the sampled time. If the
unexpired visual is Evolve, remain in Normal, display `GROWING`, and consume the
press. Otherwise clear an old transient result, reset selection to Feed, enter
ActionSelector, call `renderInteractionUi()` immediately, and return true.

Opening the selector does not mutate the model and does not request a save.

### Selector center

Sample/advance current-boot time, validate the enum, and call `performAction()` once.
Copy its result, render only returned change bits, and return to Normal. Applied
requests a save and calls `startActionVisual()` even for `changes == None`;
refused/invalid starts no visual and requests no action save. Show the result and
return true. Never pass `Count` to the model.

The result and model change mask are independent. In particular, Rest at maximum
stats is Applied/None and still animates and requests persistence; low-energy Play
and healthy Doctor mutate nothing, do not animate, and do not request persistence.

If the initial time advance crosses Child-to-Adult while the selector is open,
evolution wins the dispatch: close the selector, start Evolve, consume the current
edge, and do not execute an action. This prevents the edge that was intended for
the old selector from paging the stack or starting a hidden action. The next edge
uses Normal-mode rules.

## Result text and timing contract

Use fixed string literals; no `String`, formatting allocation, emoji, or audio.

| Action/result | Text | Action visual | Result visible |
| --- | --- | ---: | ---: |
| Feed / Applied | `FED` | 750 ms | 1,600 ms |
| Play / Applied | `PLAYED` | 750 ms | 1,600 ms |
| Clean / Applied | `CLEAN` | 750 ms | 1,600 ms |
| Rest / Applied | `RESTED` | 900 ms | 1,600 ms |
| Doctor / Applied | `BETTER` | 900 ms | 1,800 ms |
| Play / RefusedLowEnergy | `TOO TIRED` | none | 1,800 ms |
| Doctor / RefusedNotSick | `NOT SICK` | none | 1,800 ms |
| any / RefusedWhileEgg | `HATCH FIRST` | none | 1,800 ms |
| any / InvalidAction | `ACTION?` | none | 1,800 ms |
| Normal center during Evolve | `GROWING` | keep evolve | 1,200 ms |

Successful hatch uses `HATCHED` for 1,600 ms, switches directly from the egg loop
to child idle, and requests persistence. It does not use the Child-to-Adult Evolve
strip. A defensive second/stale hatch is a no-op: repair the mode from model state,
do not show `HATCHED`, and do not request a new save.

`RefusedWhileEgg` defensively covers stale state. `SAVE!` and `TIME?` remain
independent status indicators, not replacements for transient result text.

## Animation and deadline race policy

`startActionVisual()` selects the child/adult sprite from the post-action stage,
uses the duration table above, sets a wrap-safe deadline, and performs Phase 10's
single `setPetAnimation()` handoff. It is called only for Applied.

Action execution returns to Normal immediately; it is not a fourth Busy mode.
While an action one-shot remains active:

- up/down still navigate away because Normal never consumes them;
- center may reopen the selector;
- a later Applied action replaces the earlier action animation and deadline;
- a later refusal changes only result text and leaves the earlier visual to expire;
- no actions are queued and no completion callback captures `this`.

Evolution has higher visual priority than actions. Center cannot open/execute the
selector during an active evolve visual; it is consumed with `GROWING`. This avoids
applying an action whose required sprite cannot be shown.

At the start of both `update()` and `handleButtonPress()`, expire deadlines using:

```cpp
static_cast<int32_t>(nowMillis - deadlineMillis) >= 0
```

All deadlines are far below `INT32_MAX`, so this is wrap-safe. If a button arrives
on the same sample that an action deadline expires, expiration runs first and the
new edge then operates on the settled state. The new accepted action is therefore
the sole active one-shot. Do not compare with `now >= deadline`.

When an action visual expires, Phase 10 selects healthy/sick idle from current
state at that moment. When result text expires, hide only the transient result and
rerender status priority; never alter model state, mode, or selector selection.

Input and `update()` are serialized on `lvglTask`; these are ordering rules, not
cross-task locking rules. When passive advancement changes sickness, mess, or
stage immediately before an action, the action observes the updated model. Doctor
may therefore treat newly entered sickness. A newly triggered evolution preempts
any action one-shot and closes an open selector. No action/result is queued, and
no callback may retain the removable card.

## Immediate-save request boundary

`requestImmediateSave()` only sets `_immediateSaveRequested = true`. It is
idempotent and performs no NVS I/O, clock wait, queue send, or callback. Set it
after a successful hatch and every Applied action, including Applied/None.

Phase 13 will service this request through `saveWithClockBaseline(true)`, clear it
only after a successful write, retain it on failure, and combine it with model
dirty flags and `SAVE!`. Phase 11 must not call `markPersisted()` or clear a prior
request. This boundary makes the action durable as soon as the later persistence
policy is connected without duplicating epoch logic here.

The handoff is deliberately split:

- Phase 11 sets the request only after a successful hatch or an `Applied` action;
- Phase 9's session adapter remains the only code allowed to stamp or invalidate
  `lastUpdatedEpoch`;
- Phase 13 calls `saveWithClockBaseline(true)`, clears the request only after the
  exact snapshot is stored, and leaves both the request and `SAVE!` active on
  failure;
- refusal, invalid selection, selector movement, selector open, and result expiry
  never create an action save request;
- an already-pending request is never cleared or replaced by a later refusal.

## Render/update call discipline

All methods in this phase execute on `lvglTask`. Do not dispatch another UI
callback. Create no LVGL objects in input or update paths.

- `setInteractionMode()` toggles the already-created selector and normal hint,
  then calls `renderInteractionUi()` once.
- `moveSelection()` updates only the existing selector label/hints immediately.
- Hatch renders returned change bits, the stage/visual, interaction row, and status.
- Action execution renders returned change bits, then result/status and visual.
- Refusal renders result/status and mode only; bars and model sprites stay cached.
- `update()` continues Phase 9 time advancement and Phase 10 throttled model render,
  but checks 50-100 ms visual/result deadlines before the one-second render gate.
- A passive stage transition to Adult forces mode Normal, hides the selector, and
  starts Evolve before an input edge can be processed in that loop.

Guard every LVGL mutation with Phase 10's existing pointer-validity policy. If the
card was prepared for removal, input returns false and performs no model, visual,
save-request, or LVGL work.

`handleButtonPress()` is the sole gameplay-input path. Neither it nor `update()`
may call `buttons[]`, `Input::update()`, Bounce2 `.pressed()`, `.released()`,
`.isPressed()`, or `.read()`. `update()` may service elapsed time and the
50-100 ms one-shot/result deadlines, but it must never synthesize or repeat an
action from a held level.

## Global deep-sleep chord priority

Do not add chord recognition, chord state, or cancellation to `TamagotchiCard`.
The existing global polling order is authoritative: after all Bounce2 objects are
updated, `lvglHandlerTask` samples center and down levels; if both are held it
arms/continues the two-second sleep timer and forwards no individual press edge.
Therefore a chord observed in one polling sample has priority over hatch, selector
open, selector movement, action execution, and card paging in that sample.

The guarantee is bounded by the existing 50 ms polling granularity. If center was
already observed and forwarded in an earlier poll before down became held, that
earlier press is a legitimate center action and Phase 11 cannot retroactively undo
it. Once both levels are observed together, all later individual edges are
suppressed until the chord is released or deep sleep begins. Validate both the
same-sample chord and this staggered-press boundary; do not describe the latter as
an input race inside the card. Phase 13 owns the controller-wide pre-sleep flush.

## Implementation sequence

1. Confirm the Phase 2/7/9/10 identifiers and preserve unrelated work.
2. Add the enum, members, fixed action/result tables, and initialization.
3. Add selector movement, result, deadline, and save-request helpers.
4. Implement hatch, open, move, execute/refuse, and close transitions.
5. Integrate deadline expiry into `update()` and input dispatch.
6. Replace Phase 10's placeholder handler with the consumption table.
7. Build, inspect the focused diff, and perform hardware button validation.

## Validation matrix

| Start | Input | Expected state/effect | Consumed/navigation |
| --- | --- | --- | --- |
| Egg | Up | Egg unchanged | false; previous card |
| Egg | Down | Egg unchanged | false; next card |
| Egg | Center | Child, idle, save requested | true; no navigation |
| Normal | Up | no pet mutation | false; previous card |
| Normal | Down | no pet mutation | false; next card |
| Normal | Center | selector at Feed | true |
| Selector/Feed | Down x5 | Feed after full wrap | true each |
| Selector/Feed | Up | Doctor | true |
| Selector/Doctor | Down | Feed | true |
| Selector/each action | Center | exact Phase 7 result, then Normal | true |
| Selector/Play, energy 14 | Center | `TOO TIRED`, no mutation/save/visual | true |
| Selector/Play, energy 15 | Center | energy 0, Applied visual/save | true |
| Selector/Doctor, healthy | Center | `NOT SICK`, no mutation/save/visual | true |
| Selector/Doctor, sick | Center | not sick, health >=60, visual/save | true |
| Normal/action active | Up or Down | animation may continue offscreen | false; navigate |
| Normal/evolve active | Center | `GROWING`, no action | true |
| Selector/advance reaches Adult | any button | close selector, start Evolve, no action/navigation | true |
| Any | unknown index | no change | false |
| Egg/Normal/Selector | Center+Down already held in same poll | no hatch/open/select/execute edge | global sleep path; handler not called |

For the chord case, press both within one 50 ms polling sample, hold beyond two
seconds, and release/reset as the hardware permits. State/action counters must not
change in any mode. Separately press center, wait for it to be forwarded, then hold
down: the already-dispatched center effect may remain, while no later pet edge may
be produced once the chord is active. Phase 11 validates input priority only;
Phase 13 adds the pre-sleep flush.

Also verify a center edge on the exact action deadline, repeated rapid center
actions, a refusal during a prior action visual, navigation away during result
text, return after the deadline, `millis()` wrap simulation, and passive evolution
while the selector is open.

Run `pio run -e adafruit_feather_esp32s3_reversetft` and report the production
build and firmware size. Do not claim native tests passed;
the repository has no enabled native PlatformIO test environment.

Static review must also confirm that only `ActionSelector` branches return true for
up/down, no new input task or event was added, no source reads shared Bounce2 state
from Tamagotchi code, all deadline comparisons use signed-delta wrap-safe logic,
and no selector/result/deadline field was added to the versioned state record.

## Failure branches

- Missing/invalid LVGL root after removal: return false without dereferencing it.
- Stale Egg mode with a hatched model: resynchronize to Normal; never rehatch/reset.
- Invalid selected enum: show `ACTION?`, close to Normal, no model call/save/visual.
- Model refusal: show exact refusal, close to Normal, no mutation/save/action visual.
- Applied with no change bits: animate and request save; do not relabel as refusal.
- Save already pending: leave the Boolean true; never lose the earlier request.
- Result deadline expires off-card: next active update clears it safely.
- Action deadline expires off-card: next active update selects current idle/sick.
- Evolve starts with selector open: close selector, enter Normal, preserve evolution.
- Rapid new Applied action: replace prior action visual/deadline; do not enqueue it.
- Unknown button: pass through false; do not reinterpret it as a selector direction.
- Center+down chord: card receives nothing and cannot mutate; global code retains priority.

## Deliverables

- `TamagotchiCard` has the explicit Egg, Normal, and ActionSelector transient modes
  and initializes/reconciles them from the authoritative model stage.
- Its handler implements the complete transition/consumption table and the fixed
  Feed, Play, Clean, Rest, Doctor wrap order in both directions.
- Hatch, selector open, selector execution, all Phase 7 outcomes, fixed messages,
  and wrap-safe result deadlines are connected to the Phase 10 UI objects.
- Applied actions hand off exactly one replaceable action visual and an idempotent
  immediate-save request; refusals hand off text only.
- Evolution, rapid input, expiry-at-edge, stale mode, removal, and invalid-selection
  behavior follow the deterministic policies above.
- Source review and the production PlatformIO build are recorded; the physical
  button/chord cases are explicitly marked hardware validation until run.
- The focused diff contains only `src/ui/TamagotchiCard.h/.cpp` when this phase is
  implemented; it contains no Phase 12 registration or Phase 13 sleep/save-policy
  work.

## Exit criteria

Phase 11 is complete only when all three modes and transitions are explicit and:

- Feed/Play/Clean/Rest/Doctor wrap in the specified order in both directions;
- the per-mode return values exactly match the table;
- center hatches, opens, and executes exactly once per forwarded edge;
- every action result maps to the fixed message/duration contract;
- only Applied starts an action visual and requests immediate persistence;
- refusal and Applied/None semantics match Phase 7;
- one-shot/result deadlines are wrap-safe and deterministic against input races;
- Egg/Normal always allow up/down card paging, including during one-shots;
- no Bounce2 edge/held/released state is read from `update()`;
- the card defines no chord and center+down cannot cause a pet action;
- model/LVGL work stays on the UI task with no recurring allocation;
- the production build and hardware-only checks are reported honestly;
- no Phase 12 registration/portal or Phase 13 sleep-flush implementation is included.

## Handoff

Phase 12 registers and constructs the singleton card and its input handler; it must
not change this consumption policy. Phase 13 services `_immediateSaveRequested`,
adds retry/checkpoint/removal behavior, and connects the global pre-sleep flush.
Phase 14 validates durability across reset/deep sleep and repeats this full button
matrix on the target Feather.
