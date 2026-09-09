# Tamagotchi card: step-by-step implementation plan

## Goal

Implement the MVP from [`tamagochi-features.md`](tamagochi-features.md) as a singleton DeskHog card with a fully animated, original pixel-art pet. The card must support the egg, child, and adult stages; five needs; five care actions; sickness and mess; persistence; offline catch-up; safe card removal; and the global deep-sleep chord.

This document is an implementation handoff. Complete the steps in order. Keep every step buildable, and do not move to the next step while the current step has compile errors or unexplained generated-file changes.

## Non-negotiable architecture

- The model contains no Arduino, LVGL, NVS, networking, or task code.
- `TamagotchiCard` is the only component that maps model state to LVGL and button behavior.
- Model updates, actions, rendering, and pet-state writes happen on the LVGL task after startup.
- Do not add a Tamagotchi task or a removable-card event subscription.
- Pet state lives in a dedicated `tamagotchi` NVS namespace, not `cards/config_list`.
- The navigation stack owns deletion of the LVGL card root.
- Sprites are mandatory MVP assets. LVGL primitives may provide the room, bars, and small effects, but not replace the animated pet.
- Use an original desk-hog character. Do not imitate copyrighted Tamagotchi characters or reuse branded character art.

## Sprite set to deliver

Use transparent, tightly framed 32x32 PNG frames. Display them at 2x scale for an approximately 64x64 pet on the TFT. Keep the same ground line, center point, light direction, outline weight, and palette in every frame.

| Sprite group | Frames | Use |
| --- | ---: | --- |
| `tamagotchi_egg` | 2 | Looping egg wobble before hatching |
| `tamagotchi_child_idle` | 3 | Normal child idle loop |
| `tamagotchi_adult_idle` | 3 | Normal adult idle loop |
| `tamagotchi_child_feed` | 3 | Child feed action |
| `tamagotchi_adult_feed` | 3 | Adult feed action |
| `tamagotchi_child_play` | 3 | Child play action |
| `tamagotchi_adult_play` | 3 | Adult play action |
| `tamagotchi_child_clean` | 3 | Child clean action |
| `tamagotchi_adult_clean` | 3 | Adult clean action |
| `tamagotchi_child_rest` | 2 | Child rest action |
| `tamagotchi_adult_rest` | 2 | Adult rest action |
| `tamagotchi_child_sick` | 2 | Persistent sick child loop |
| `tamagotchi_adult_sick` | 2 | Persistent sick adult loop |
| `tamagotchi_child_doctor` | 3 | Child treatment action |
| `tamagotchi_adult_doctor` | 3 | Adult treatment action |
| `tamagotchi_evolve` | 4 | One-shot child-to-adult transition |
| `tamagotchi_mess` | 1 at 16x16 | Mess overlay shown beside the pet |

This is 44 32x32 frames plus one 16x16 frame, approximately 177 KiB of raw ARGB8888 pixel data before compiler/linker overhead. Treat 200 KiB as the initial Tamagotchi sprite budget. If the firmware lacks sufficient OTA-slot headroom, reduce action frame counts before reducing legibility.

## Existing sprite pipeline verified in this repository

The implementing model must use the current walking animation as the visual and technical reference, not assume a generic asset pipeline:

- `raw-png/walking/` contains six visually consistent pixel-art hedgehog walking frames. They use a brown/tan/black palette, a strong pixel outline, a transparent background, and small body/leg shifts between frames.
- Every current source frame is an 80x80 sRGB `TrueColorAlpha` PNG with 1-bit alpha.
- `png2c.py` recursively sorts every PNG under `raw-png/`, groups files by their first directory, and flattens all generated `.c`/`.h` files into `include/sprites/`.
- The generator converts each pixel to BGRA byte order and declares an `lv_img_dsc_t` using `LV_COLOR_FORMAT_ARGB8888`.
- Each current 80x80 descriptor has `data_size = 80 * 80 * 4 = 25,600` bytes. The six walking frames therefore embed 153,600 bytes before overhead.
- `include/sprites/sprites.h` includes every generated descriptor and declares `walking_sprites[]` plus `walking_sprites_count`; `sprites.c` defines that ordered pointer array.
- `platformio.ini` compiles `include/sprites/*.c`, so adding a PNG under `raw-png/` adds its uncompressed pixel data to the firmware on the next successful regeneration/build.
- `FriendCard` creates one `lv_animimg`, assigns `walking_sprites`, sets a 1000 ms duration and infinite repeat, scales it with the LVGL 8 compatibility name `lv_img_set_zoom(..., 512)`, and starts it.
- `FriendCard::stopAnimation()` only changes a Boolean and does not stop LVGL's running animation. Do not copy that behavior; the Tamagotchi switching procedure below uses `lv_animimg_delete()` correctly.
- `png2c.py` does not clean stale per-frame `.c`/`.h` outputs. When a source PNG is renamed or removed, remove only its matching generated files and confirm that `sprites.h`, `sprites.c`, and the build no longer reference it.

The Tamagotchi assets intentionally use 32x32 canvases instead of copying the current 80x80 dimensions. Forty-four 80x80 frames would consume about 1.1 MiB and put the two-slot OTA budget at serious risk. Match the existing art direction and binary-alpha transparency, but use the smaller canvas and LVGL's native v9 `lv_image_set_scale(..., 512)` to display at 2x.

Never hand-edit anything in `include/sprites/`; make changes in the source/reference assets or final `raw-png/` frames, rerun `png2c.py`, and review the generated result.

## Step 1: establish the baseline

1. Read `docs/cards.md`, `docs/games.md`, `docs/input-and-navigation.md`, `docs/display-and-lvgl.md`, `docs/configuration-and-state.md`, `docs/assets.md`, `docs/ota.md`, and `docs/build-test-release.md`.
2. Record `git status` and preserve all unrelated user changes.
3. Run the configured firmware build:

   ```sh
   pio run -e adafruit_feather_esp32s3_reversetft
   ```

4. Record the baseline flash/RAM usage and `firmware.bin` size.
5. Confirm that Pillow and NumPy are available to the PlatformIO Python environment. The current sprite generator exits successfully without regenerating assets when they are missing, so a green build alone is not proof that sprites were converted.

Deliverable: a known-good baseline and a recorded size against the `0x1F0000` OTA application slot.

## Step 2: define the model and persistence contracts

Create `src/tamagotchi/TamagotchiTypes.h` with:

- `TamagotchiEggType { Default }`;
- `TamagotchiStage { Egg, Child, Adult }`;
- `TamagotchiLocation { Room }`;
- `TamagotchiAction { Feed, Play, Clean, Rest, Doctor }`;
- action-result and model-change enums;
- a `TamagotchiState` containing schema version, egg, stage, location, age, five 0-100 needs, sickness, mess, next-mess time, simulation remainder, and last valid epoch;
- all tuning constants in one place.

Use these initial rules:

- child becomes adult after 24 hours of post-hatch age;
- offline catch-up is capped at seven days;
- a mess appears every six hours after hatch or the previous clean;
- hunger drops 1 every 20 minutes;
- happiness drops 1 every 45 minutes and faster while sick/neglected;
- energy drops 1 every 30 minutes;
- hygiene drops 1 every 45 minutes and faster while a mess exists;
- health drops only during sickness or severe hunger/hygiene neglect;
- Feed: hunger +35, happiness +5, health +3;
- Play: happiness +20, energy -15, refused below 15 energy;
- Clean: hygiene to 100, health +3, clear mess;
- Rest: energy +35, health +2;
- Doctor: clear sickness and raise health to at least 60;
- no need value, sickness, or age can kill/reset the pet.

Define schema version 1 before implementing behavior. Use a single compact JSON record under NVS namespace `tamagotchi`, key `state`, with a target serialized size below 384 bytes.

Deliverable: stable types, constants, and version-1 field names that all later steps consume.

## Step 3: generate the canonical pet artwork

Use the built-in image-generation tool. Do not use an API/CLI fallback unless the user explicitly requests it.

1. Inspect all six `raw-png/walking/Normal-Walking_*.png` files with `view_image`. Use them as style references for pixel density, outline, palette, transparency, pose readability, and frame-to-frame consistency; do not modify or overwrite them.
2. Generate one canonical child character on a transparent background, passing a representative walking frame as a style reference and labeling it as a reference rather than an edit target.
3. Inspect the result both at generated size and after a temporary nearest-neighbor reduction to 32x32. The silhouette, face, and action must remain readable at final size.
4. Generate the adult from the selected child as an edit/reference-based variant. Keep identity, palette, face, and proportions recognizable; make adulthood visible through a slightly larger body and more developed quills.
5. Save the selected high-resolution references under `_ai/art/tamagotchi/references/`, outside `raw-png/`. Anything under `raw-png/` is automatically embedded in firmware.
6. Save the final prompts in `_ai/art/tamagotchi/prompts.md` so later frames can repeat the same invariants.

Use a prompt shaped like this:

```text
Use case: stylized-concept
Asset type: pixel-art game character sprite reference for a 240x135 embedded display
Primary request: an original tiny desk-hog virtual pet, cute and expressive, designed to remain readable at 32x32 pixels
Subject: one full-body character, compact silhouette, short legs, expressive face, simple quills
Style/medium: crisp limited-palette pixel art, strong dark outline, no gradients
Composition/framing: centered, front three-quarter view, consistent ground line, generous but economical transparent padding
Color palette: 8-12 high-contrast colors suitable for a small TFT
Constraints: genuinely transparent background; one character only; no text; no logo; no watermark; no cast shadow; no scenery; no anti-aliased halo; original design
Avoid: existing Tamagotchi characters; photorealism; soft painterly edges; excessive detail
```

Deliverable: approved child and adult reference art plus the reproducible prompt specification.

## Step 4: generate every animation strip

Generate one animation group at a time, using the selected child/adult reference image to preserve identity. Prefer reference-based edits over unrelated fresh generations.

For each group:

1. Load the selected local child/adult reference with `view_image`, then pass that visible image to the built-in image-generation edit flow.
2. Ask for the required number of equally spaced frames in one horizontal strip on a genuinely transparent background.
3. Repeat the invariants: same character, dimensions, viewpoint, palette, ground line, lighting, outline, and framing; change only the requested pose/action.
4. Include no text, UI, room background, unrelated props, watermark, or second character.
5. Inspect the strip before accepting it. Reject identity drift, clipped quills/feet, inconsistent scale, opaque backgrounds, and action frames that do not read at thumbnail size.
6. Iterate with one targeted correction at a time.
7. Save accepted source strips under `_ai/art/tamagotchi/generated/`, never directly under `raw-png/`.

Action requirements:

- feed clearly shows eating a small generic food item;
- play uses one simple toy/motion and retains the same pet scale;
- clean uses a small sparkle/bubble effect without text;
- rest closes the eyes and lowers the body;
- sick uses a slumped pose and clear discomfort without emoji;
- doctor includes a compact medical/needle cue and recovery pose;
- evolve visibly bridges child and adult without flashes filling the whole canvas;
- mess is a separate, readable 16x16 overlay.

Deliverable: accepted source strips for all groups in the sprite manifest.

## Step 5: normalize and place final PNG frames

Add a deterministic helper such as `tools/process_tamagotchi_sprites.py` using Pillow. It should:

1. split each accepted strip into frames;
2. remove accidental opaque backgrounds;
3. convert alpha to the same binary 0/255 behavior used by the current walking sprites, removing near-transparent edge pixels that would create halos;
4. quantize to the approved limited palette if necessary;
5. resize only with nearest-neighbor sampling and place every pet on the same tightly framed 32x32 canvas, center point, and ground line;
6. emit the mess icon at 16x16;
7. fail if a frame has the wrong size, missing alpha, empty content, or pixels outside the canvas;
8. print total frame count and estimated ARGB8888 byte cost.

Place only final runtime frames in `raw-png/`. Use underscore-only directory names and globally unique basenames because `png2c.py` writes all generated C files into one flat directory:

```text
raw-png/tamagotchi_child_idle/tamagotchi_child_idle_00.png
raw-png/tamagotchi_child_idle/tamagotchi_child_idle_01.png
raw-png/tamagotchi_child_idle/tamagotchi_child_idle_02.png
raw-png/tamagotchi_adult_feed/tamagotchi_adult_feed_00.png
...
```

Do not use repeated names such as `frame_00.png` in different directories; they would overwrite each other's generated C/H files.

Deliverable: exactly the final runtime PNG frames under `raw-png/`, plus the repeatable processing script.

## Step 6: convert and validate embedded sprites

1. Run `png2c.py` through the normal PlatformIO build or directly with the required environment.
2. Verify that `include/sprites/sprites.h` exposes one array per top-level sprite directory, for example `tamagotchi_child_idle_sprites` and `tamagotchi_child_idle_sprites_count`.
3. For each final PNG, verify a matching `include/sprites/sprite_<basename>.c/.h` exists and that its descriptor reports `LV_COLOR_FORMAT_ARGB8888`, width 32, height 32, and `data_size = 4096` (the mess frame reports 16, 16, and 1024).
4. Verify every generated group array contains frames in lexicographic filename order and its count matches the manifest.
5. Review generated diffs. No source strip/reference sheet should appear in the generated output, and no pre-existing walking descriptor should change.
6. Rebuild and compare firmware size with the baseline and 200 KiB sprite budget.
7. Render at least one generated frame on hardware before implementing gameplay. Confirm transparency, BGRA color order, native dimensions, 2x scale, stable center/ground line, and absence of clipping on the TFT.

Deliverable: compiled LVGL image descriptors with validated color, transparency, ordering, and firmware-size cost.

## Step 7: implement the pure model

Create `src/tamagotchi/TamagotchiModel.h/.cpp`.

Implement, in this order:

1. fresh egg defaults;
2. hatch transition and starting stats;
3. clamped action effects and action results;
4. fixed 60-second elapsed-time steps with a persisted remainder;
5. deterministic mess scheduling and clean reset;
6. deterministic sickness entry and doctor recovery;
7. deterministic child-to-adult transition;
8. dirty/change flags used by rendering and persistence.

`advanceBy(seconds)` must produce the same state for one bulk interval as for equivalent minute-by-minute calls. It must allocate nothing and must not contain `millis()` or epoch logic.

Deliverable: a host-compatible model whose outputs depend only on the previous state, action, and elapsed seconds.

## Step 8: implement the dedicated state store

Create `src/tamagotchi/TamagotchiStateStore.h/.cpp`.

1. Open namespace `tamagotchi` once during startup.
2. Serialize the complete version-1 state into one fixed-capacity ArduinoJson document and `state` string.
3. Validate required fields, enum values, need ranges, remainder, stage/age consistency, schema version, and maximum record length.
4. Return a fresh egg for a missing record.
5. For malformed or unsupported data, log only the error category/version, replace it with a fresh state, and never print the raw record.
6. Keep future migrations behind an explicit schema-version switch.
7. Return success/failure from every save so the card can retain its dirty flag after a failure.

Deliverable: state survives reset without affecting Wi-Fi, API, insight, or card configuration NVS data.

## Step 9: add trustworthy wall-clock support

Create a long-lived `ClockService` and share it with Tamagotchi and `OtaManager`.

1. Start SNTP asynchronously after `WIFI_CONNECTED`.
2. Expose current synchronized epoch and a sanity/validity check.
3. Preserve OTA's bounded wait behavior by making its existing sync helper use the service.
4. In the card session, use wrap-safe `millis()` deltas for current-boot progression.
5. Apply persisted epoch catch-up exactly once when synchronized time is available.
6. Cap catch-up at seven days and immediately save the new epoch baseline.
7. If time is unavailable, preserve state, continue same-boot progression, and show `TIME?`; never persist or compare `millis()` across reboot.
8. If the clock moves backward, apply no negative time and rebaseline safely.

Deliverable: no duplicate NTP owner, no double-counted interval, and explicit no-clock behavior.

## Step 10: build the card and connect sprite animation

Create `src/ui/TamagotchiCard.h/.cpp` with one root, one `lv_animimg` pet object, one mess `lv_image`, status labels, five compact bars, and one bottom action-selector row. Create all LVGL objects once.

Use this layout:

- top 18 px: stage, transient result, `SICK`, `MESS`, `TIME?`, or `SAVE!` status;
- left/main area: 2x sprite animation in a simple LVGL-primitive room;
- right area: five abbreviated 0-100 bars;
- bottom 25 px: normal hint or selected action and button hints.

Add one method that maps state to the correct generated array:

```text
Egg                         -> tamagotchi_egg loop
Child + healthy + idle      -> tamagotchi_child_idle loop
Adult + healthy + idle      -> tamagotchi_adult_idle loop
Child/adult + sick + idle   -> matching sick loop
Feed/Play/Clean/Rest/Doctor -> matching stage action, one shot, then current idle/sick loop
Child reaches adult         -> tamagotchi_evolve, one shot, then adult idle
mess == true                -> show tamagotchi_mess overlay
```

Implement sprite switching against the actual LVGL 9.2 API in this repository. Use one helper with the same generated pointer-array shape already used by `FriendCard`:

```cpp
void TamagotchiCard::setPetAnimation(
    const lv_img_dsc_t* frames[],
    uint8_t frameCount,
    uint32_t durationMs,
    bool loop
) {
    if (!_petImage || !lv_obj_is_valid(_petImage) || !frames || frameCount == 0) {
        return;
    }

    // Stops the currently running animimg animation; it does not delete the LVGL object.
    lv_animimg_delete(_petImage);

    lv_animimg_set_src(_petImage, (const void**)frames, frameCount);
    lv_image_set_src(_petImage, frames[0]);
    lv_animimg_set_duration(_petImage, durationMs);
    lv_animimg_set_repeat_count(
        _petImage,
        loop ? LV_ANIM_REPEAT_INFINITE : 0
    );
    lv_animimg_start(_petImage);
}
```

Apply `lv_image_set_scale(_petImage, 512)` once after creating the object; LVGL uses 256 for 1x and 512 for 2x. Align the image inside a fixed pet viewport and validate the scaled bounds because scaling does not make LVGL reserve a larger layout box automatically.

Important switching rules:

1. Track the current visual enum and do nothing when the requested visual is already active; otherwise a recurring render would restart frame zero continuously.
2. Call `lv_animimg_delete()` before changing source, duration, or repeat count. In this LVGL version it removes the running animation, not `_petImage` itself.
3. Set frame zero explicitly with `lv_image_set_src()` so the previous animation does not remain visible until the first animation tick.
4. Use `LV_ANIM_REPEAT_INFINITE` for egg/idle/sick loops and repeat count `0` for one-shot action/evolution sequences.
5. Store a wrap-safe action end deadline when starting a one-shot. In `update()`, switch back to the stage-appropriate healthy/sick idle sequence after that deadline.
6. Do not capture `this` in `lv_animimg_set_completed_cb()`. A card-owned deadline avoids a completion callback outliving a dynamically removed card.
7. In `prepareForRemoval()`, call `lv_animimg_delete(_petImage)` while the object is valid, then null all LVGL pointers. Let `CardNavigationStack` delete the root and children.

Create the mess overlay once with `lv_image_create()`. Assign `tamagotchi_mess_sprites[0]` with `lv_image_set_src()` and only toggle `LV_OBJ_FLAG_HIDDEN` when `mess` changes. Do not animate or recreate this object.

Throttle model/render checks to about one second and animation-state deadlines to about 50-100 ms. Do not allocate `String`, create LVGL objects, or deserialize data in the recurring update path.

Deliverable: the egg, both life stages, every action, sickness, evolution, and mess visibly use the generated assets on hardware.

## Step 11: implement the three-button interaction

Use explicit modes: `Egg`, `Normal`, and `ActionSelector`.

1. Egg: center hatches; up/down return `false` so normal card navigation works.
2. Normal: center opens the selector; up/down return `false`.
3. Selector: up/down wrap through Feed, Play, Clean, Rest, and Doctor and return `true`.
4. Selector center performs the action, saves it, starts its sprite animation, shows a short result, returns to Normal, and returns `true`.
5. Do not re-read Bounce2 edge flags from `update()`; there is no held-input gameplay.
6. Do not implement any chord in the card. The global center+down deep-sleep chord keeps priority.

Deliverable: the card is always escapable outside selector mode, while selector presses cannot page the stack.

## Step 12: register the singleton card

1. Add `CardType::TAMAGOTCHI` and stable string conversion `"TAMAGOTCHI"` in `src/config/CardConfig.h`.
2. Add an exact `tryStringToCardType()` path so malformed portal values are not silently converted to `INSIGHT`.
3. Register a `CardDefinition` in `CardController::initializeCardTypes()`:
   - name: `Tamagotchi`;
   - `allowMultiple = false`;
   - `needsConfigInput = false`;
   - description: `Hatch and care for a tiny desk companion`.
4. Inject the state store and clock service into the factory and card.
5. Track the instance in `dynamicCards` and register its input handler.
6. Validate portal POST payloads against card definitions and reject duplicate singleton entries with HTTP 400 before writing NVS.
7. During reconciliation, instantiate only the first ordered singleton entry even if legacy/crafted NVS contains duplicates. Do not rewrite configuration from reconciliation.

Deliverable: the portal can add only one card, and crafted configuration cannot create a second in firmware.

## Step 13: finish persistence and lifecycle safety

1. Save immediately after hatch, every applied action, mess/sickness transition, and evolution.
2. Checkpoint passive dirty changes no more than once every five minutes while active.
3. In `prepareForRemoval()`, advance time, force-save, stop animation, and null LVGL pointers without deleting the stack-owned root.
4. Add default `InputHandler::prepareForSleep()` and `CardController::prepareForSleep()` hooks.
5. Call the controller hook immediately before `esp_deep_sleep_start()`; Tamagotchi's implementation advances/saves without touching LVGL.
6. Keep the NVS record when the card is removed so re-adding resumes the same pet.
7. Do not call the removable card directly from OTA/background tasks. Immediate saves plus epoch catch-up handle OTA restart and unexpected power loss.

Deliverable: actions and lifecycle transitions survive removal, re-add, reorder, reset, deep sleep, and OTA.

## Step 14: validate the completed implementation

Run the production firmware build and review source plus generated diffs. Then validate on the Feather:

1. clean NVS -> add -> egg animation -> hatch -> child;
2. all five actions and their child sprites;
3. mess, hygiene penalty, clean, sickness, doctor, and their sprites;
4. accelerated child-to-adult transition and adult sprite/action set;
5. navigation versus selector button consumption;
6. center+down held for two seconds without a pet action;
7. remove/re-add/reorder with state preserved;
8. connected catch-up, delayed NTP, no-network boot, later sync, backward clock, and seven-day cap;
9. immediate reset after each action and evolution;
10. at least 20 card rebuild cycles while monitoring heap/PSRAM for leaks;
11. operation during Wi-Fi reconnect, portal save, insight update, and OTA activity;
12. transparency, color, frame order, animation timing, and clipping for every sprite group;
13. final binary size within `0x1F0000` and Tamagotchi sprite data near/below the 200 KiB budget.

There is no enabled native PlatformIO test environment. Do not claim native tests passed unless one is explicitly added and run. Record which validation items require physical hardware and any that remain unverified.

## Definition of done

The card is complete when one animated pet can be added through the portal, hatched, cared for, evolved, removed, and resumed; every pet state and action uses the intended embedded sprite animation; time and NVS behavior are safe; button navigation and deep sleep still work; repeated lifecycle operations do not leak or double-delete; and the production firmware builds within the OTA slot.
