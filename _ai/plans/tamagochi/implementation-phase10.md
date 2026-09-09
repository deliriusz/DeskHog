# Tamagotchi implementation Phase 10: card UI and sprite animation

## Phase outcome

Build the LVGL presentation boundary for the Tamagotchi in:

```text
src/ui/TamagotchiCard.h
src/ui/TamagotchiCard.cpp
```

At the end of this phase the card can load one model session, advance it while
active, render the egg/child/adult state and five needs, display clock/save status,
and select every generated animation without recreating LVGL objects.

This phase deliberately does not register the card, finish its three-button action
policy, or add removal/deep-sleep/checkpoint persistence. Phase 11 owns input and
action execution, Phase 12 owns factory/singleton registration, and Phase 13 owns
those persistence/lifecycle save hooks.

## Required inputs

Do not begin until these earlier contracts are present and buildable:

- Phase 2: `src/tamagotchi/TamagotchiTypes.h` and its stable enums/state fields;
- Phase 6: all 17 generated sprite arrays and their `uint8_t` counts;
- Phase 7: `TamagotchiModel`, including `getState()`, `advanceBy()`, and change flags;
- Phase 8: one already-started, boot-lifetime `TamagotchiStateStore`;
- Phase 9: one boot-lifetime `ClockService` and the session-time rules;
- Phase 1/6 firmware-size results, including remaining `0x1F0000` OTA headroom.

If an earlier phase changed only an identifier, adapt the identifier here without
changing ownership, timing, model, persistence, or asset semantics.

## Repository facts and decisions

- The stack supplies a 233x135 card area: `240 - 7` indicator pixels by 135.
- Only the active handler is updated by `CardNavigationStack::updateActiveCard()`.
- That update executes from `lvglTask` on core 1; no Tamagotchi task is permitted.
- `CardNavigationStack::removeCard()` unregisters the handler and deletes the root;
  deleting the root also deletes every LVGL child.
- `FriendCard` proves the generated array shape and animimg usage, but its v8 alias
  and stop Boolean are not the pattern to copy.
- `PaddleCard` proves the model/view split and active-card update path, but its
  frame-rate work and Boolean-only removal ownership are not sufficient here.
- `Style::labelFont()` is an 18-pixel-line-height 15 px Inter font;
  `Style::valueFont()` is a 21-pixel-line-height 16 px Inter SemiBold font.
- Use those two shared getters. Do not reference generated font symbols directly.
- `Style::backgroundColor()`, `labelColor()`, `valueColor()`, and `accentColor()`
  provide the base palette; compact warning colors may be local constants.

### LVGL version decision

`platformio.ini` requests `lvgl/lvgl @ ^9.2.2`, while the dependency currently
resolved in `.pio/libdeps` identifies itself as LVGL 9.4.0. Phase 1 must resolve
that policy explicitly: either pin the documented 9.2.2 version or adopt the
resolved 9.4.x version and update project documentation. Code this card only with
the native APIs common to and verified against the selected LVGL 9.x version:

- `lv_image_create()` and `lv_image_set_src()`;
- `lv_image_set_scale(image, 512)` where 256 is 1x and 512 is 2x;
- `lv_animimg_create()`, `lv_animimg_set_src()`, and `lv_animimg_start()`;
- `lv_animimg_delete()` to remove the running animation;
- `lv_obj_delete()` for an unowned construction-failure root.

Do not use the v8 aliases `lv_img_set_zoom()` or `lv_obj_del()` in the new files.
If a source-level gate is retained, it must accept the documented 9.2.2 baseline
as well as the currently resolved 9.4.x build and reject an unreviewed future major:

```cpp
#if LV_VERSION_MAJOR != 9 || !LV_VERSION_CHECK(9, 2, 0)
#error "TamagotchiCard requires a reviewed LVGL 9.x version at or above 9.2"
#endif
```

Do not silently substitute `lv_anim_delete()` against animimg internals. Before
implementation exits, inspect the selected dependency headers and compile the
exact calls below; if 9.2.2 and 9.4.x differ, isolate the smallest version-checked
compatibility helper and validate both supported paths instead of requiring 9.4
contrary to the repository's documented baseline.

The relevant resolved declarations are
`lv_animimg_set_src(lv_obj_t*, const void*[], size_t)`,
`lv_animimg_set_duration(lv_obj_t*, uint32_t)`,
`lv_animimg_set_repeat_count(lv_obj_t*, uint32_t)`,
`lv_animimg_start(lv_obj_t*)`, and `bool lv_animimg_delete(lv_obj_t*)`.

The inspected implementation establishes an important semantic distinction:
`lv_animimg_delete(obj)` calls `lv_anim_delete()` for the animimg's internal
animation and returns a Boolean. It does not delete `obj`. Parent/root deletion is
still responsible for destroying the animimg widget.

## Scope and file changes

Create only:

- `src/ui/TamagotchiCard.h` — card API, UI pointers, render snapshot, and timing;
- `src/ui/TamagotchiCard.cpp` — object creation, rendering, time polling, animation.

Do not modify in this phase:

- `CardConfig.h`, `CardController`, portal files, or singleton validation;
- `InputHandler`, global button polling, or deep-sleep chord behavior;
- model rules, persisted fields, NVS schema, clock synchronization, or OTA;
- `raw-png/`, generated sprite `.c/.h` files, or `png2c.py`;
- `main.cpp`, task creation, `EventQueue`, or UI callback queues.

## Ownership and constructor contract

Use this exact public shape:

```cpp
class TamagotchiCard : public InputHandler {
public:
    TamagotchiCard(
        lv_obj_t* parent,
        TamagotchiStateStore& stateStore,
        ClockService& clockService
    );
    ~TamagotchiCard() override;

    lv_obj_t* getCard() const;
    bool handleButtonPress(uint8_t buttonIndex) override;
    bool update() override;
    void prepareForRemoval() override;

private:
    // Types, helpers, and members specified below.
};
```

The card owns `TamagotchiModel` by value. It borrows `TamagotchiStateStore` and
`ClockService` by reference; those dependencies must outlive every removable card.
It owns no event subscription and captures itself in no queued callback.

Load state exactly once. Avoid a default model followed by assignment and avoid
retaining a bulky `TamagotchiLoadResult` for the full card lifetime. Use a private
value helper and delegating constructor:

```cpp
struct InitialSession {
    TamagotchiState state{};
    bool persistenceDirty = false;
    bool saveFailed = false;
};

static InitialSession loadInitialSession(TamagotchiStateStore& stateStore);

TamagotchiCard(
    lv_obj_t* parent,
    TamagotchiStateStore& stateStore,
    ClockService& clockService,
    InitialSession initialSession
);
```

The public constructor delegates with `loadInitialSession(stateStore)`. Convert
the Phase 8 result to `state`, `persistenceDirty = !loaded.persisted`, and
`saveFailed = !loaded.persisted`. Initialize `_model(initialSession.state)` once.
Do not interpret load origin in the renderer and do not load NVS a second time.

## Header types and members

Define a private visual enum independent of persisted/model enums:

```cpp
enum class PetVisual : uint8_t {
    None,
    Egg,
    ChildIdle,
    AdultIdle,
    ChildSick,
    AdultSick,
    ChildFeed,
    AdultFeed,
    ChildPlay,
    AdultPlay,
    ChildClean,
    AdultClean,
    ChildRest,
    AdultRest,
    ChildDoctor,
    AdultDoctor,
    Evolve
};
```

Use a small render snapshot so each property is written to LVGL only when changed:

```cpp
struct RenderSnapshot {
    TamagotchiStage stage = TamagotchiStage::Egg;
    uint8_t needs[5] = {0, 0, 0, 0, 0};
    bool sick = false;
    bool mess = false;
    bool timeUnknown = true;
    bool saveFailed = false;
    bool initialized = false;
};
```

Declare members in safe initialization order:

```text
TamagotchiStateStore& _stateStore
ClockService& _clockService
TamagotchiModel _model
bool _persistenceDirty
bool _saveFailed

uint32_t _lastAdvanceMillis
uint32_t _millisRemainder
uint64_t _sameBootAppliedSeconds
uint64_t _pendingBaselineEpoch
OfflineCatchUpState _catchUpState
bool _timeUnknown

uint32_t _lastModelUpdateMillis
uint32_t _lastAnimationCheckMillis
TamagotchiModelChange _pendingRenderChanges
TamagotchiStage _stageBeforeLastAdvance

PetVisual _currentVisual
PetVisual _oneShotVisual
uint32_t _oneShotDeadlineMillis
uint32_t _resultDeadlineMillis
char _resultText[24]
RenderSnapshot _rendered
bool _preparedForRemoval
bool _visualFaultLogged
```

Use the `OfflineCatchUpState` and six session fields exactly as Phase 9 defines.
The phase-9 `advanceFromMillis()` and `tryApplyOfflineCatchUp()` helpers belong in
this card, not the pure model. OR their returned model changes into
`_pendingRenderChanges` so catch-up and normal updates use one render path.

Declare and initialize these LVGL pointers to `nullptr`:

```text
_card
_header
_stageLabel
_resultLabel
_statusLabel
_roomViewport
_floorLine
_petImage
_messImage
_needsPanel
_needLabels[5]
_needBars[5]
_footer
_actionLabel
_hintLabel
```

No member is an owning pointer to the model, store, clock, generated frames, or
fonts. Generated arrays and font descriptors remain flash/static data.

## Exact 233x135 layout

Use explicit pixels, zero root padding, and no percentage/flex layout. The card's
visible range is x `0..232`, y `0..134`. Each table coordinate is relative to the
object's direct parent shown in the object tree; `_card`, `_header`,
`_roomViewport`, `_needsPanel`, and `_footer` therefore use card coordinates,
while their children use container-local coordinates.

| Object | x | y | width | height | Notes |
| --- | ---: | ---: | ---: | ---: | --- |
| `_card` | 0 | 0 | 233 | 135 | black, border 0, radius 0, non-scrollable |
| `_header` | 0 | 0 | 233 | 18 | transparent, no border/padding |
| `_stageLabel` | 3 | 0 | 45 | 18 | left aligned, label font |
| `_resultLabel` | 48 | 0 | 104 | 18 | centered, label font, clipped/dot mode |
| `_statusLabel` | 152 | 0 | 78 | 18 | right aligned, label font |
| `_roomViewport` | 3 | 19 | 137 | 89 | dark room, radius 4, clipped children |
| `_floorLine` | 3 | 81 | 131 | 2 | muted floor/accent primitive |
| `_petImage` | 53 | 33 | 32 | 32 | native layout box; 2x draw around pivot |
| `_messImage` | 105 | 65 | 16 | 16 | fixed overlay beside pet/floor |
| `_needsPanel` | 143 | 19 | 87 | 90 | transparent, no padding |
| `_footer` | 0 | 110 | 233 | 25 | dark separator/background |
| `_actionLabel` | 4 | 2 | 122 | 21 | value font, selected action/normal prompt |
| `_hintLabel` | 127 | 3 | 102 | 18 | label font, right aligned |

Need rows are relative to `_needsPanel`. Use row y positions `0, 18, 36, 54, 72`.
At each row create an 18x18 label at x 0 and a 64x7 bar at x 21, y `row + 6`.
The fixed label texts are `HU`, `HA`, `HP`, `EN`, and `HY` for hunger, happiness,
health, energy, and hygiene. Set every bar range to `0..100` once.

Use this object tree and create every node exactly once:

```text
_card
├── _header
│   ├── _stageLabel
│   ├── _resultLabel
│   └── _statusLabel
├── _roomViewport
│   ├── _floorLine
│   ├── _petImage                 (one lv_animimg)
│   └── _messImage                (one lv_image)
├── _needsPanel
│   ├── _needLabels[5]
│   └── _needBars[5]
└── _footer
    ├── _actionLabel
    └── _hintLabel
```

The room viewport itself supplies the wall/background. Do not embed a room bitmap.
Clear `LV_OBJ_FLAG_SCROLLABLE` on every container and ensure the viewport does not
have `LV_OBJ_FLAG_OVERFLOW_VISIBLE`; transformed pixels must clip inside the room.

Set the pet source before positioning/scaling, explicitly set pivot `(16, 16)`,
disable transform antialiasing with `lv_image_set_antialias(_petImage, false)`, then
call `lv_image_set_scale(_petImage, 512)` once. With object position `(53,33)`
inside the 137x89 viewport, the native box is local x `53..84`, y `33..64`; its
64x64 transformed draw bounds are local x `37..100`, y `17..80` (card/global x
`40..103`, y `36..99`). Those bounds remain inside the room and end one pixel above
the local floor at y `81..82`. Scaling does not change the object's 32x32 layout
size, so do not center it by assuming LVGL reserves 64x64. Inspect every frame's
transparent extent on hardware because descriptor size alone cannot prove that a
visible quill or effect remains away from the transformed edge.

Set the mess source once to `tamagotchi_mess_sprites[0]`. It remains native 16x16.
Only add/clear `LV_OBJ_FLAG_HIDDEN` when model `mess` changes.

## Styles and colors

- Card: `Style::backgroundColor()` and `LV_OPA_COVER`.
- Room: `0x17212B`; floor: `0x40505C`; no decorative gradients.
- Header/footer: black or `0x090D10`, with no border and one 1 px separator.
- Stage/result: `Style::valueColor()` / `Style::labelColor()`.
- Status healthy/empty: label color; sick/save: `0xFF5A5F`; time: `0xF2C14E`;
  mess: `0xD98C3F`.
- Bar track: `0x26313A`; bar indicator >=60: `Style::accentColor()`;
  30..59: `0xF2C14E`; 0..29: `0xFF5A5F`.
- Bars have radius 2, zero border, and indicator opacity cover.

Apply font/style properties once at construction except a bar's threshold color and
the current status color. Do not initialize an `lv_style_t` on the stack and attach
it beyond that stack frame; direct object styles are sufficient for this small tree.

## Generated sprite symbols

Include only `sprites/sprites.h` from the `.cpp`. Use these exact Phase 6 arrays and
their matching `uint8_t` count symbols without copying the pointer arrays:

| Visual | Array/count symbols | Duration | Repeat |
| --- | --- | ---: | --- |
| Egg | `tamagotchi_egg_sprites`, `_count` | 900 ms | infinite |
| Child idle | `tamagotchi_child_idle_sprites`, `_count` | 900 ms | infinite |
| Adult idle | `tamagotchi_adult_idle_sprites`, `_count` | 900 ms | infinite |
| Child feed | `tamagotchi_child_feed_sprites`, `_count` | 750 ms | one shot |
| Adult feed | `tamagotchi_adult_feed_sprites`, `_count` | 750 ms | one shot |
| Child play | `tamagotchi_child_play_sprites`, `_count` | 750 ms | one shot |
| Adult play | `tamagotchi_adult_play_sprites`, `_count` | 750 ms | one shot |
| Child clean | `tamagotchi_child_clean_sprites`, `_count` | 750 ms | one shot |
| Adult clean | `tamagotchi_adult_clean_sprites`, `_count` | 750 ms | one shot |
| Child rest | `tamagotchi_child_rest_sprites`, `_count` | 900 ms | one shot |
| Adult rest | `tamagotchi_adult_rest_sprites`, `_count` | 900 ms | one shot |
| Child sick | `tamagotchi_child_sick_sprites`, `_count` | 700 ms | infinite |
| Adult sick | `tamagotchi_adult_sick_sprites`, `_count` | 700 ms | infinite |
| Child doctor | `tamagotchi_child_doctor_sprites`, `_count` | 900 ms | one shot |
| Adult doctor | `tamagotchi_adult_doctor_sprites`, `_count` | 900 ms | one shot |
| Evolve | `tamagotchi_evolve_sprites`, `_count` | 1200 ms | one shot |
| Mess | `tamagotchi_mess_sprites[0]`, `_count == 1` | static | none |

Here `_count` means the complete matching name, for example
`tamagotchi_child_feed_sprites_count`; do not literally create a shared `_count`.

## Visual priority and transition rules

Rendering the model and choosing the pet sprite are related but separate. Bars,
labels, and mess can update while a one-shot animation remains on screen.

| Priority | Condition | Pet visual | End behavior |
| ---: | --- | --- | --- |
| 1 | Child changed to Adult in this advance | `Evolve` | current Adult sick/idle |
| 2 | Applied action one-shot is active | stage/action match | current sick/idle |
| 3 | Stage is Egg | `Egg` | loop |
| 4 | Child and sick | `ChildSick` | loop |
| 5 | Adult and sick | `AdultSick` | loop |
| 6 | Child and healthy | `ChildIdle` | loop |
| 7 | Adult and healthy | `AdultIdle` | loop |

Evolution preempts and cancels an action one-shot. A loaded Adult must not replay
evolution; start it only when one model call actually changes Child to Adult.
An action one-shot is otherwise not interrupted by sickness/mess/need changes.
At its deadline, derive the return visual from the live stage and sick flag, so a
Doctor returns to healthy idle and a newly sick pet returns to sick idle.

The mess image is orthogonal to this table. Show it whenever `state.mess` is true,
including during any action/evolution; hide it only when the model clears the flag.

Top status uses one constrained label and this priority:

```text
SAVE! > TIME? > SICK > MESS > empty
```

`SAVE!` means an actual load/save durability failure, not merely normal dirty model
state. Sickness and mess remain apparent through sprite/overlay when a higher alert
temporarily occupies the status label.

## `setPetAnimation()` procedure

Keep selection and application separate:

```cpp
void setVisual(PetVisual requested, uint32_t nowMillis);
void setPetAnimation(
    const lv_img_dsc_t* frames[],
    uint8_t frameCount,
    uint32_t durationMs,
    bool loop
);
```

`setVisual()` first returns when `requested == _currentVisual`. This equality gate
is mandatory: reapplying a desired loop on every render would continuously restart
frame zero. It maps the enum to the exact group/count/timing table, validates the
group, invokes `setPetAnimation()`, and updates `_currentVisual` only on success.

`setPetAnimation()` must perform these operations in order:

1. Require `_petImage`, `lv_obj_is_valid(_petImage)`, non-null frames, and count > 0.
2. Call `lv_animimg_delete(_petImage)` unconditionally; false only means no matching
   running animation existed and is not an object-deletion failure.
3. Call `lv_animimg_set_src(_petImage, (const void**)frames, frameCount)`.
4. Call `lv_image_set_src(_petImage, frames[0])` so stale content is never shown.
5. Set total forward duration with `lv_animimg_set_duration()`.
6. Set repeat to `LV_ANIM_REPEAT_INFINITE` for a loop, otherwise `0`.
7. Start once with `lv_animimg_start()`.

Scale and pivot are construction-time properties and are not repeated here.
Do not set a completed callback and especially do not capture `this` in one.

For a one-shot, set `_oneShotVisual` and
`_oneShotDeadlineMillis = nowMillis + durationMs`. All durations are below `2^31`
milliseconds. Detect expiry with wrap-safe signed subtraction:

```cpp
static bool deadlineReached(uint32_t now, uint32_t deadline) {
    return static_cast<int32_t>(now - deadline) >= 0;
}
```

Checking the deadline, not an LVGL callback, ensures dynamic removal cannot leave a
callback targeting a destroyed C++ wrapper.

## Model, clock, and render cadence

Initialize the Phase 9 session fields immediately after loading: current `millis()`,
zero millisecond remainder and same-boot applied seconds, loaded epoch copied to the
pending baseline, catch-up pending, and `_timeUnknown = true`.

Do not advance or block on the clock in the constructor. The first active `update()`
performs nonblocking `tryApplyOfflineCatchUp()`. Follow Phase 9 exactly: advance
same-boot time first, poll `tryGetEpoch()`, subtract already-applied same-boot
seconds, cap offline time at seven days, apply it at most once, and handle missing or
backward baselines without guessed time.

Phase 9's baseline/catch-up save is part of clock correctness and may call its
narrow `saveWithClockBaseline(true)` path. Phase 10 must not add the remaining
action, periodic checkpoint, removal, or deep-sleep save triggers; Phase 13 does so.

`update()` is called about every 5 ms but should use two gates:

- every 50 ms: check one-shot and result-message deadlines;
- every 1000 ms: advance uptime, poll pending clock catch-up, and render changes.

Use unsigned elapsed subtraction for both gates. When returning to the card after
it was inactive, the first update consumes the entire elapsed uptime interval once.
Return `true` while the root is valid; the current caller ignores the value, so it
must not be treated as cancellation.

Track model changes rather than repainting everything. On each simulation/catch-up
call, retain old stage, OR returned flags into `_pendingRenderChanges`, and then:

- `Stage`: update stage label and test exact Child-to-Adult evolution transition;
- `Needs`: update only need bars whose values differ from `_rendered.needs`;
- `SickState`: update status and steady visual unless a one-shot owns the pet;
- `MessState`: toggle the existing overlay only;
- time/save Boolean changes: update status only;
- first render: force every property regardless of flags.

Status changes and one-shot completion render immediately at their cadence rather
than waiting for another model minute. Phase 11 action handlers will call the same
focused render helpers immediately after an action.

Use fixed literals and buffers only. `lv_label_set_text()` copies the text, so a
stack/local fixed buffer is safe during the call. Use `snprintf` into bounded
`char[24]`/`char[32]` buffers where composition is required. Do not construct an
Arduino `String`, `std::string`, vector, JSON document, or LVGL object in `update()`.

## UI creation and failure behavior

Implement `bool createUi(lv_obj_t* parent)` and fail closed:

1. Reject null/invalid parent before creating the root.
2. Create/style root, then every object in tree order.
3. Check each returned pointer before its first use.
4. Set fixed label texts, bar ranges, mess source, and initial hidden flags once.
5. Configure an initial sprite and force an initial snapshot render.
6. If any essential creation fails, stop animimg if valid, synchronously delete the
   partially built root, null every UI pointer, and leave `getCard() == nullptr`.

Do not silently register a partial card. Log one short category such as
`Tamagotchi UI creation failed`; do not log persisted content.

If a sprite group is null/empty, keep the last valid frame/visual, set a one-time
visual fault for diagnostics, and do not retry/restart it at 20 Hz. A missing symbol
normally produces a compile failure and must return to Phase 6. A wrong runtime
count/dimension returns to Phase 6 rather than adding UI workarounds.

## Lifecycle and pointer safety

Centralize pointer nulling in `clearUiPointers()`. Delayed/conditional render paths
must validate the individual target with `ptr && lv_obj_is_valid(ptr)` before use.

`prepareForRemoval()` runs while the stack-owned root is still valid:

1. Return if `_preparedForRemoval` is already true.
2. If `_petImage` is valid, call `lv_animimg_delete(_petImage)`.
3. Clear one-shot/result deadlines and set visual to `None`.
4. Set `_preparedForRemoval = true`.
5. Null root and every child pointer through `clearUiPointers()`.
6. Do not delete any LVGL object; the navigation stack deletes the root next.
7. Do not save in this phase; Phase 13 prepends advance/force-save before step 2.

The destructor has two paths:

- after `prepareForRemoval()`: destroy normal C++ members only; references are not
  deleted and all LVGL pointers are already null;
- construction/factory failure before stack ownership: if root is still valid,
  stop a valid animimg and synchronously `lv_obj_delete(_card)` on the UI context,
  then null pointers.

Never delete children individually, use `lv_obj_delete_async()`, or let both wrapper
and stack delete the same root. The model is a value member and needs no `delete`.

## Phase 11 and 13 seams

Phase 10's `handleButtonPress()` returns `false` for all buttons and does not read
Bounce2 flags. This makes the passive card escapable until Phase 11 replaces it.

Implement private mapping helpers now for Phase 11 to call:

```cpp
void startActionVisual(TamagotchiAction action, uint32_t nowMillis);
void setTransientResult(TamagotchiAction action,
                        TamagotchiActionResult result,
                        uint32_t nowMillis);
void renderFooter(bool selectorOpen, TamagotchiAction selectedAction);
```

They must not execute model actions or save. Phase 11 calls the first only for an
`Applied` outcome, updates the result immediately for applied/refused outcomes, and
adds the interaction-mode/selected-action members.

Retain `_persistenceDirty` and `_saveFailed` as the Phase 13 handoff. Do not add a
second dirty flag in the store. Phase 13 clears them only after a successful store
save and adds action/evolution/mess/sickness saves, five-minute checkpoints,
removal flush, and sleep flush through Phase 9's baseline-aware helper.

## Implementation sequence

1. Record `git status --short`; preserve unrelated edits and generated files.
2. Verify the Phase 2/6/7/8/9 APIs and exact generated symbols compile.
3. Add `TamagotchiCard.h` with the public contract and initialized members.
4. Implement one-load constructor delegation and Phase 9 session initialization.
5. Implement `createUi()` in object-tree order with exact dimensions/styles.
6. Set bar ranges, fixed abbreviations, mess source, pet pivot, and 2x scale once.
7. Implement visual-to-array mapping and the guarded animimg switch procedure.
8. Implement one-shot priority, wrap-safe deadlines, and live steady-state return.
9. Implement changed-property rendering and fixed-buffer status/footer text.
10. Implement throttled active-card advancement and pending clock catch-up.
11. Implement the passive input stub and precise removal/destructor ownership split.
12. Build, inspect size/diffs, then perform the hardware scenarios below.

## Validation

### Static and build checks

- Run `pio run -e adafruit_feather_esp32s3_reversetft` and record result/size.
- Do not claim native tests; this repository has no enabled native environment.
- Confirm new files contain no `xTaskCreate`, event subscription, queued callback,
  `String`, `std::string`, per-update object creation, or direct generated font use.
- Confirm new files use `lv_image_set_scale`, not `lv_img_set_zoom`.
- Confirm every `tamagotchi_*_sprites` array/count appears exactly in the mapping.
- Confirm `setVisual()` exits for an unchanged visual before animimg deletion/start.
- Confirm every deadline comparison is wrap-safe and every duration is below `2^31`.
- Confirm transformed 64x64 bounds and native mess bounds fit the room coordinates.
- Compare firmware bytes with Phase 6 and keep the image below 2,031,616 bytes.

### Deterministic review scenarios

1. Fresh Egg selects Egg once; recurring renders do not restart frame zero.
2. Loaded healthy Child/Adult selects only the matching idle group.
3. Loaded sick Child/Adult selects only the matching sick group.
4. A loaded Adult does not play Evolve.
5. A Child-to-Adult change preempts another visual, plays Evolve once, then Adult.
6. Each stage/action combination maps to the matching generated group.
7. Doctor one-shot returns to healthy idle; a sick Feed returns to sick idle.
8. Sickness appearing during another action does not interrupt it, but controls return.
9. Mess toggles its one static object during idle, action, sick, and evolution.
10. Simultaneous alerts obey `SAVE! > TIME? > SICK > MESS`.
11. Unchanged needs/status/visuals issue no repeated LVGL setter calls.
12. A `millis()` deadline crossing `UINT32_MAX` completes normally.
13. First update after card inactivity advances the interval once.
14. Invalid parent/partial allocation produces null `getCard()` and no leaked root.
15. Repeated prepare is harmless; destructor after prepare does not touch deleted UI.

### Feather display checks

- Confirm the root exactly fills 233x135 without covering navigation indicators.
- Confirm header, five rows, and footer have no clipping at actual font metrics.
- Confirm all five bars remain distinguishable at 0, 29, 30, 59, 60, and 100.
- Observe every loop for stable ordering and at least two full cycles.
- Observe every one-shot once and confirm it holds/returns without an idle flash.
- Verify the 32x32 pet displays near 64x64, uses correct colors/transparency, keeps
  its shared ground line, and does not clip at any frame extremity.
- Verify the 16x16 mess appears beside rather than over the pet.
- Leave/return after several minutes and confirm responsive catch-up/rendering.
- Rebuild/remove the passive card at least 20 times while watching heap/PSRAM.

Hardware-only visual checks must be marked unverified when no board is available.

## Failure branches

- Missing group/count or malformed descriptor: stop and return to Phase 6; never
  hand-edit generated output or replace the mandatory sprite with primitives.
- Wrong scale/clipping: first verify pivot and calculated bounds; adjust only the
  pet object's fixed position/viewport if source dimensions are correct.
- Wrong color/alpha: compare a known walking frame, then fix Phase 5/6 source or
  conversion; do not swap channels in card rendering.
- Repeated frame zero: prove `_currentVisual` gating happens before switch calls.
- One-shot never returns: inspect wrap-safe deadline cadence, not a new callback.
- Heap growth across rebuilds: audit partial-construction and prepare/destructor
  paths, especially whether animimg stopped and only one owner deleted the root.
- UI allocation failure: return a null card so the Phase 12 factory rejects it.
- Clock unavailable: preserve state, continue same-boot progression, show `TIME?`;
  never fabricate epoch elapsed time.
- Save failure: retain dirty/failure state and show `SAVE!`; never hide the problem
  merely to clear the header.
- Build exceeds OTA slot: do not upload; compare Phase 6, card code, and generated
  diffs before changing the approved sprite budget.

## Deliverables and exit criteria

Deliver:

- `TamagotchiCard.h/.cpp` with the exact dependency/lifecycle contract;
- one fixed 233x135 object tree with five bars and an action/footer row;
- all 16 pet animation mappings plus the static mess overlay;
- no-restart visual switching, deterministic one-shot deadlines, and proper return;
- throttled, change-driven UI/model/time work with no recurring allocation;
- build/size record and hardware observations or explicit hardware blockers.

Phase 10 is complete only when the production build succeeds within the OTA slot,
all objects are created once, all sprite symbols resolve, native LVGL 9 APIs are
used, unchanged loops do not restart, one-shots respect evolution/action precedence,
the transformed pet and overlay fit, rendering remains on the UI task, and repeated
construction/removal shows no double deletion or growth.

## Handoff to Phase 11 and Phase 12

Phase 11 keeps the public constructor/lifecycle API, adds explicit Egg/Normal/
ActionSelector input state, executes model actions, and calls the prebuilt action,
result, footer, and immediate-render helpers. It must preserve the one-shot priority
table and must not add held/chord input.

Phase 12's factory captures the existing boot-lifetime store and clock references,
constructs `TamagotchiCard(screen, stateStore, clockService)`, and accepts it only
when `card`, `getCard()`, and `lv_obj_is_valid(getCard())` are all true. Only then
does it track the wrapper and register the handler. It must delete a failed wrapper,
must not create another store/clock, and must preserve stack ownership of the root.

Phase 13 prepends baseline-aware advance/force-save to `prepareForRemoval()`, adds
checkpoint/action/transition and deep-sleep persistence hooks, and retains pet NVS
when the card configuration is removed. It must not alter animation ownership or
introduce a callback that outlives the card.
