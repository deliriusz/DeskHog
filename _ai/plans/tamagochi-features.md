# Tamagotchi MVP feature scope for DeskHog

## Goal

Build a small, single-pet care simulation as a configurable DeskHog card. The player starts one egg, cares for the pet as it grows from child to adult, and manages its basic needs in one fixed location.

The MVP loop is:

```text
Add the Tamagotchi card through the DeskHog portal
      ↓
Confirm the single egg and hatch it
      ↓
Care for the child
      ↓
Respond to basic needs
      ↓
The pet grows into an adult
      ↓
Continue caring for the adult
```

The egg and location remain data fields so later versions can add choices without changing the save format. The MVP exposes only one egg and one location.

## DeskHog target and constraints

This feature runs on the Adafruit ESP32-S3 Reverse TFT Feather, not a general-purpose desktop UI:

- Dual-core ESP32-S3 running Arduino C++ and FreeRTOS through PlatformIO.
- One integrated 240x135 landscape ST7789 TFT driven over SPI.
- LVGL 9.2.2 owns the display. The display uses two buffers and partial-render mode; the navigation stack reserves 7 horizontal pixels for its indicators, leaving roughly 233x135 for a card.
- PSRAM is required at startup. Large display buffers, TLS, and JSON may use PSRAM, but the pet model should remain small and avoid per-frame allocation.
- The active card is updated from `lvglTask` on core 1. Do not create a pet task, render task, or input task.
- The three controls are `DOWN`/BOOT on GPIO 0 (active LOW), `CENTER` on GPIO 1 (active HIGH), and `UP` on GPIO 2 (active HIGH).
- Center + down held for two seconds is a global deep-sleep chord. No pet action may depend on that simultaneous hold.
- There is no touch input, audio system, or filesystem partition. Feedback must fit the TFT, the existing fonts/sprites, and NVS.
- The board has one NeoPixel, but the MVP should not depend on it for essential gameplay feedback.
- Automatic light sleep is enabled. Explicit deep sleep wakes through the hardware reset path, so state must be flushed before the device sleeps.

The implementation must fit the existing PlatformIO environment, `adafruit_feather_esp32s3_reversetft`, and each OTA application slot of `0x1F0000` bytes. Generated assets are part of the firmware; there is no separate asset filesystem.

## DeskHog integration architecture

The Tamagotchi is one normal dynamic card in the vertical `CardNavigationStack`:

1. Add a `TAMAGOTCHI` value to `src/config/CardConfig.h` and add matching `cardTypeToString()` and `stringToCardType()` cases. The persisted string is an API/storage contract.
2. Add a focused `TamagotchiCard` under `src/ui/` and a non-LVGL model/engine under a focused `src/tamagotchi/` directory, for example `TamagotchiModel` and `TamagotchiStateStore`.
3. Register a `CardDefinition` in `CardController::initializeCardTypes()` with `allowMultiple = false`, no configuration input, and a short portal description. The existing portal discovers it through `GET /api/cards/definitions`.
4. Track the card in `CardController::dynamicCards`, register it with `CardNavigationStack`, and make it removable, reorderable, and re-addable without losing the pet state.
5. Keep the pet state out of the card `config_list`. That JSON is for card instances, is limited to 2048 bytes, and causes a full card-stack rebuild when written. Store pet state in a separate versioned `tamagotchi` Preferences/NVS namespace.
6. Implement `InputHandler::handleButtonPress()`, `update()`, and `prepareForRemoval()`. The navigation stack owns deletion of the LVGL root; the card destructor must release its model/resources without deleting that root a second time.
7. Keep local actions and model updates on the LVGL task. Do not use `EventQueue` for ordinary button actions. If a background clock-sync event is introduced, copy the state and marshal any LVGL work through the UI callback queue; an event callback is not a UI callback.

Use the `PaddleCard`/`PaddleGame` split as the closest pattern, while avoiding the documented Flappy/Paddle removal leaks. The model should be testable without LVGL; the card should own only presentation, input policy, and lifecycle wiring.

## Feature summary

| Area | MVP scope | DeskHog-specific implementation note |
| --- | --- | --- |
| Card | One Tamagotchi card | Added through the existing captive portal; provisioning card remains stack index 0 |
| Egg | One selectable egg option | Store `eggType` even though only one value is available |
| Lifecycle | Egg -> Child -> Adult | Deterministic transition, no branches |
| Location | One fixed location | Keep a location field, but render one compact room view |
| Needs | Hunger, happiness, health, energy, hygiene | Clamp all values to a documented range such as 0-100 |
| Care actions | Feed, play, clean/toilet, rest, doctor | Three-button action selector; no touch or custom mini-game |
| Sickness | Simple sick state cured by doctor/needle | Use a text/shape/sprite indicator rather than relying on emoji glyph coverage |
| Time | Same-boot elapsed time and persisted offline catch-up | Use `millis()` only for uptime; use a valid wall clock for reboot/power-loss catch-up |
| Feedback | Compact text, bars/icons, and simple animations | Fit the 233x135 card area and avoid per-frame allocations |
| Persistence | Pet state plus schema and last-update timestamp | Separate NVS namespace, debounced writes, explicit sleep/removal flush |
| Terminal state | None | No death, memorial, or forced restart flow |

## 1. Card setup and egg selection

The user adds the Tamagotchi card through the existing DeskHog web portal. The card has no user-entered configuration value; its persisted `CardConfig.config` can remain empty or hold a stable schema marker.

When the pet state does not exist, the card shows exactly one egg:

```text
Egg
 ↓ center
Hatch
 ↓
Child
```

The setup screen may describe the egg as selectable for future compatibility, but there is only one valid `eggType` in the MVP. Center confirms the egg/hatch action. There are no rarity tiers, random starting traits, or meaningful differences between pet types.

The portal/card catalog must not permit multiple Tamagotchi instances. `allowMultiple = false` is currently mostly portal metadata, so firmware-side validation or reconciliation must also prevent duplicate singleton cards.

## 2. Pet lifecycle

The only stages are:

1. **Egg** — waits for the player’s hatch confirmation or the defined hatch condition.
2. **Child** — the main early-care stage.
3. **Adult** — reached through one deterministic evolution.

The child-to-adult threshold must be one named model constant, not a UI literal. Use a deterministic age/stage-progress value and keep the duration easy to accelerate in hardware validation. Care quality does not choose between forms.

There are no baby, teen, senior, healthy, troublemaker, angel, or other alternate stages/forms. The adult remains available for continued care and never evolves again.

## 3. Core needs

The model tracks only these public needs:

### Hunger

Hunger decreases with elapsed time. Feeding restores it.

### Happiness

Happiness increases through feeding and playing and decreases when needs are unmet or the pet is sick.

### Health

Health is affected by sickness and prolonged neglect. Good care and the doctor action restore it.

### Energy

Energy is consumed by play and restored by rest. A tired pet may refuse or weaken play rather than creating a new system.

### Hygiene

Hygiene decreases as time passes and when the pet creates a mess. Cleaning restores it.

All values must be clamped after every update/action. Do not add hidden evolution scores or systems for stress, affection, weight, fitness, discipline, behavior, or personality in the MVP.

## 4. Care actions and physical controls

Because DeskHog has only three buttons and up/down normally page through cards, use a two-mode control policy:

- **Card navigation mode:** the default mode. `UP` and `DOWN` return `false` from `handleButtonPress()` so `CardNavigationStack` moves between cards. `CENTER` opens the pet action selector or confirms the current setup screen.
- **Action selector mode:** `UP` and `DOWN` consume the press and move through Feed, Play, Clean, Rest, and Doctor. `CENTER` consumes the press and executes the selected action, then returns to the normal view.

The model must not require a simultaneous button hold because center + down is reserved for deep sleep. The selector should always show the selected action and a short result message so the player does not need sound.

### Feed

- Restores hunger.
- Provides a small happiness increase.
- Uses a fixed food action rather than an owned food inventory.

### Play

- Increases happiness.
- Consumes energy.
- Plays one short built-in interaction/animation.
- Does not open a custom mini-game or create a separate game card.

### Toilet and cleaning

- The pet can create a mess.
- Cleaning the mess restores hygiene and clears the mess state.
- Ignored messes can make the pet dirty and contribute to sickness.

### Rest

- Resting restores energy over the action’s defined recovery period or through repeated explicit rest actions.
- Sleep is an explicit action, not a day/night schedule.

### Doctor

When the pet is sick, the player selects Doctor, represented by a compact needle/medical visual:

```text
Sick
  ↓ center
Doctor / needle
  ↓
Recovering
```

The doctor action is the only sickness cure. There is no medicine inventory, clinic location, or treatment item.

## 5. Sickness

The pet can enter a simple sick state when needs are neglected, especially health or hygiene. While sick:

- Show a persistent sick indicator in the compact card header/status area.
- Reduce happiness and/or energy according to model rules.
- Keep Doctor available in the action selector.

Treatment clears the sick state and applies a defined recovery effect. Illness never escalates into a terminal condition.

## 6. Time and simulation

The model must separate simulation time from rendering time:

- `TamagotchiCard::update()` runs from the LVGL task while the card is visible. It should call a model `advance(now)` method using elapsed time and render only changed state.
- Because DeskHog updates only the active card, the model must catch up from its last timestamp when the user returns to the card. It does not need a background render task.
- Use `millis()` deltas for continuous same-boot simulation, with wrap-safe unsigned arithmetic. Do not persist `millis()` as an offline timestamp; it resets on reboot.
- Persist a wall-clock epoch such as `lastUpdatedEpoch` for power-cycle catch-up. The existing NTP logic lives in the OTA path and is not a general pet clock, so add or reuse a small clock/time-sync service before claiming exact offline progression.
- On boot or Wi-Fi time synchronization, apply the elapsed interval once, clamp it to a documented maximum if necessary, save the result, and mark the state caught up. If no valid wall clock is available, preserve the state and surface that exact offline catch-up is pending rather than inventing elapsed days.

Persisting the last-update time is therefore a hardware/software prerequisite for the offline requirement, not just a field added to the UI model. There is no documented battery-backed RTC in the target board, and deep sleep/restart behavior must not be treated as equivalent to a powered uptime counter.

The MVP has no day/night cycle, real-world sleep schedule, weather, seasons, or time-dependent activities.

## 7. Single location

The pet stays in one fixed room/location. All MVP actions are available in that card.

Keep `location` in the model for future compatibility, but use one fixed value. There is no travel, exploration, location selection/unlocking, room customization, shop, school, hospital, or other separate area.

## 8. Visual feedback for a 240x135 TFT

The card should be designed for the real 233x135 content area, not a desktop-sized mockup. A practical layout is:

- a small top status row for lifecycle stage, sickness/mess, and the current action/result;
- a central low-resolution pet/room sprite or LVGL primitive animation;
- compact bars or numeric abbreviations for the five needs;
- a bottom action selector shown only while action mode is active.

Use the shared `Style` font getters and colors. The generated fonts currently target 15 px labels, 16 px values, 36 px prominent values, and 20 px decorative/game text. Avoid five full-width rows of 15 px text, emoji, or layouts that assume portrait orientation.

Create LVGL objects once and update their text, bar values, positions, colors, and visibility. Do not allocate `String`/LVGL objects every 5 ms. Throttle simulation/render work to a human-visible cadence while keeping action transitions immediate.

Useful simple animations are:

```text
idle
eat
play
rest
dirty
sick
doctor
evolve
```

If new sprites are needed, edit `raw-png/` sources and let `png2c.py` regenerate `include/sprites/`. Crop transparent space and keep frame count/dimensions small because sprites are embedded as ARGB8888 data. Do not hand-edit generated files.

The MVP uses visual feedback only. The single NeoPixel may later mirror status, but the card must remain understandable with the TFT alone.

## 9. Persistence and state schema

Use a separate, versioned Tamagotchi record in NVS, exposed through a focused state-store component or explicit `ConfigManager` accessors. Do not put pet state in `cards/config_list`.

The minimum state is:

```text
schemaVersion
eggType
stage
age / stage progress
location
hunger
happiness
health
energy
hygiene
sickState
messState
lastUpdatedEpoch
```

Define defaults, valid ranges, absent-state behavior, and migration rules before writing code. On malformed or unsupported state, log the schema/version and fall back to a fresh egg without logging secrets or dumping arbitrary NVS contents.

Avoid flash writes on every render tick. Keep a dirty flag, save after player actions and important transitions, periodically checkpoint while the card is active, and flush from `prepareForRemoval()` and the deep-sleep path. The storage layer is not currently mutex-protected; keep pet writes on the UI task and do not add portal writes to the same record without explicit synchronization.

The state must survive:

- normal restart and power loss, subject to valid wall-clock availability for exact elapsed-time catch-up;
- card removal and re-addition;
- card reorder and navigation away;
- OTA update, since the OTA process replaces the application slot but preserves NVS.

There is no persistence for inventory, currency, relationships, achievements, generations, or other deferred systems.

## 10. Power and lifecycle safety

The global input code currently enters deep sleep after the center+down chord and does not explicitly flush transient card state. Integrate the feature with a small persistence/sleep hook so the pet is advanced and saved before `esp_deep_sleep_start()` without putting LVGL calls in the power path.

The Tamagotchi card must also:

- stop animations and held-input effects in `prepareForRemoval()`;
- release all non-LVGL resources in its destructor;
- never leave a queued UI callback capturing a destroyed card;
- tolerate card-stack reconciliation while Wi-Fi, portal, insight, and OTA tasks are active;
- remain stable when the active card changes during a save or clock-sync event.

## 11. Implementation map

Expected focused changes:

| Area | Planned change |
| --- | --- |
| `src/config/CardConfig.h` | Add `TAMAGOTCHI` and both stable string conversions |
| `src/tamagotchi/` | Add the pure pet state/action/time model and versioned persistence boundary |
| `src/ui/TamagotchiCard.h/.cpp` | Add the LVGL card, action selector, rendering, input policy, and removal lifecycle |
| `src/ui/CardController.h/.cpp` | Include the card, register its singleton factory, track it in `dynamicCards`, and keep `main.cpp` wiring-focused |
| `src/ConfigManager.*` or state store | Add a dedicated NVS namespace/key schema with validation and migration |
| `src/main.cpp`/power ownership | Add only the minimal pre-deep-sleep persistence hook if the existing global chord cannot call a safe coordinator |
| `raw-png/` and possibly `typography/` | Add only compact local assets; regenerate tracked `include/` outputs during build |
| Docs/tests | Document time-source behavior, state schema, and hardware validation; isolate pure model code for a future native test target |

Do not add Tamagotchi-specific logic to `src/main.cpp`, `EventQueueTask`, the portal HTTP callbacks, or a new continuously running FreeRTOS task.

## 12. MVP acceptance criteria

The feature is within scope when:

1. The existing portal can add exactly one Tamagotchi card, and the card can be removed, reordered, revisited, and re-added without losing its saved pet.
2. A fresh state starts at the single egg and the player can hatch it into a child with the center button.
3. The child eventually becomes an adult through one deterministic, testable evolution threshold.
4. There are no alternate evolution forms or care-based personality categories.
5. The player can open the action selector and feed, play with, clean, rest, and treat the pet using only the three physical buttons.
6. Up/down still pages the DeskHog card stack when the pet is not in action-selector mode.
7. The center+down deep-sleep chord remains reserved for power management and never triggers a pet action.
8. Sickness can be entered from neglected needs and cured with the Doctor/needle action.
9. Stats progress during powered uptime and catch up after restart/power loss when a valid synchronized wall clock is available; the unsynchronized-clock behavior is explicit and safe.
10. State is persisted in its own versioned NVS record without corrupting card configuration or Wi-Fi/API settings.
11. The pet remains available after reaching adulthood and never enters a death, memorial, or forced restart state.
12. All gameplay fits the 240x135 landscape display, uses no touch/audio/network asset dependency, and produces no per-frame allocation or LVGL calls from a background task.
13. The firmware builds in `adafruit_feather_esp32s3_reversetft`, generated assets are reviewed, and the binary remains within the `0x1F0000` OTA application slot.

## 13. Validation plan

There is currently no enabled native PlatformIO test environment, so do not report native tests as passing unless one is explicitly configured. Isolate the model so deterministic time/action tests can be added later, then validate on the actual Feather:

- Build with `pio run -e adafruit_feather_esp32s3_reversetft`; review generator output and firmware size.
- Start with clean NVS, add the card through the portal, hatch the pet, reboot, and verify state restoration.
- Repeat with existing Wi-Fi/API/card configuration and verify no unrelated settings change.
- Exercise every button in navigation mode and action mode, including the center+down two-second sleep chord.
- Verify timing with a shortened development threshold, same-boot elapsed time, Wi-Fi/NTP synchronization, no-network boot, and a power-cycle/restart.
- Remove/re-add/reorder the card repeatedly and watch for invalid LVGL callbacks, double deletion, heap loss, or model reset.
- Run while Wi-Fi reconnects, portal actions execute, insight data arrives, and OTA status is active; ensure the UI remains on the LVGL task.
- Inspect free heap/PSRAM and serial logs for allocation failures without logging credentials or pet-sensitive data.

## Deferred features

The following are intentionally out of scope for this version:

- additional egg options;
- healthy, troublemaker, angel, or other pet/evolution distinctions;
- multiple evolution branches or generations;
- death from age, sickness, starvation, or neglect;
- custom mini-games;
- discipline and behavior;
- personality traits;
- day/night cycle and scheduled sleep;
- genetics and breeding;
- currency and shop;
- inventory and item ownership;
- clothing and cosmetic customization;
- multiple locations, exploration, and location unlocking;
- NPCs and friendships;
- quests and achievements;
- school and jobs;
- notifications;
- sound;
- cloud synchronization or PostHog analytics for pet state;
- senior stages, memorials, or any other death-related flow.

When these systems are revisited, extend the versioned pet state and action model rather than changing MVP behavior implicitly. Any new background producer must document its task context, ownership, queue-full behavior, and UI handoff before it is added.
