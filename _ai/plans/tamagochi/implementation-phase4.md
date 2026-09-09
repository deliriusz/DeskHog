# Tamagotchi implementation phase 4: generate every animation strip

## Phase outcome

Produce and approve the complete source-strip set for the Tamagotchi sprite manifest: 17 horizontal strips containing 44 pet/egg frames intended for later 32x32 normalization plus one mess frame intended for 16x16 normalization. Preserve prompts, input lineage, strip geometry, and QA evidence so phase 5 can split and normalize the art without making creative decisions.

This phase creates source art and review metadata only. It does not create runtime frames, generated C descriptors, firmware code, or an image-processing script.

## Scope

Phase 4 includes:

- generating exactly one accepted horizontal source strip for each of the 17 manifest groups;
- using the matching phase 3 child or adult master as a reference-based input for every life-stage strip;
- using both masters for the child-to-adult evolution strip;
- defining ordered pose/action beats so frame order has gameplay meaning;
- preserving identity, scale, viewpoint, ground line, lighting, outline, palette, and transparency across all strips;
- checking every frame at source size, as an isolated thumbnail, and as an ordered animation/contact sheet;
- recording exact prompts, revisions, hashes, split geometry, alpha observations, palette observations, and required phase 5 cleanup;
- saving accepted source strips and QA artifacts under `_ai/art/tamagotchi/generated/`, outside the automatic firmware asset pipeline.

## Non-goals

Do not:

- redesign or replace either canonical phase 3 character;
- add another egg, child, adult, evolution branch, costume, personality, location, or character;
- split strips into individual runtime frames;
- crop, rescale, quantize, dither, force binary alpha, or normalize final 32x32/16x16 canvases;
- implement `tools/process_tamagotchi_sprites.py`; that belongs to phase 5;
- add any PNG under `raw-png/`;
- run `png2c.py`, edit `include/sprites/`, or build embedded descriptors; that belongs to phase 6;
- edit LVGL, card, model, storage, portal, or firmware code;
- add room scenery, UI, text, labels, status icons, audio cues, or NeoPixel behavior to a strip;
- use an API, CLI, third-party generator, or local image model instead of the built-in image-generation tool unless the user explicitly changes that constraint.

## Prerequisites and exact phase 3 inputs

Phase 4 must not begin generation until all four stable phase 3 deliverables exist:

```text
_ai/art/tamagotchi/references/tamagotchi_child_reference.png
_ai/art/tamagotchi/references/tamagotchi_adult_reference.png
_ai/art/tamagotchi/references/reference-manifest.json
_ai/art/tamagotchi/prompts.md
```

Before the first strip:

1. Parse `reference-manifest.json` and require `schemaVersion == 1`.
2. Recompute the child and adult SHA-256 values and require them to match the manifest.
3. Confirm both images are RGBA PNGs with transparent corners and the dimensions/modes recorded by phase 3.
4. Read the complete frozen identity bible, negative constraints, accepted palette, center, ground-line, facing, and lighting notes from `prompts.md` and the manifest.
5. Confirm phase 3 explicitly approved both masters and that the adult is derived from the accepted child.
6. Use the child reference as the sole identity source for child groups and the adult reference as the sole identity source for adult groups. Do not substitute the older walking sprites as character inputs.
7. Use both accepted masters for `tamagotchi_evolve`. Use the child master as a style/palette reference for the egg and mess only; those assets must not depict a second pet.

Expected phase 3 anchors are a screen-right front three-quarter view, visual center near x=16 after normalization, feet on final y=28, upper-left flat-step lighting, a strong consistent outline, and the exact accepted 8-12-color palette roles. The reference manifest is authoritative if its measured values differ from this summary.

If any reference is absent, a hash differs, the JSON does not parse, or the approved palette/anchors are missing, stop and return the work to phase 3. Do not generate from a guessed or stale master. A requested change to face geometry, forehead notch, belly patch, silhouette, palette, viewpoint, light direction, center, or ground line also reopens phase 3.

## Fixed source-output layout

Create this directory only for accepted outputs and their durable evidence:

```text
_ai/art/tamagotchi/generated/
```

Rejected candidates and disposable previews belong under:

```text
/tmp/deskhog-tamagotchi-phase4/
```

The committed Phase 4 outputs have fixed names:

```text
_ai/art/tamagotchi/generated/tamagotchi_egg_strip.png
_ai/art/tamagotchi/generated/tamagotchi_child_idle_strip.png
_ai/art/tamagotchi/generated/tamagotchi_adult_idle_strip.png
_ai/art/tamagotchi/generated/tamagotchi_child_feed_strip.png
_ai/art/tamagotchi/generated/tamagotchi_adult_feed_strip.png
_ai/art/tamagotchi/generated/tamagotchi_child_play_strip.png
_ai/art/tamagotchi/generated/tamagotchi_adult_play_strip.png
_ai/art/tamagotchi/generated/tamagotchi_child_clean_strip.png
_ai/art/tamagotchi/generated/tamagotchi_adult_clean_strip.png
_ai/art/tamagotchi/generated/tamagotchi_child_rest_strip.png
_ai/art/tamagotchi/generated/tamagotchi_adult_rest_strip.png
_ai/art/tamagotchi/generated/tamagotchi_child_sick_strip.png
_ai/art/tamagotchi/generated/tamagotchi_adult_sick_strip.png
_ai/art/tamagotchi/generated/tamagotchi_child_doctor_strip.png
_ai/art/tamagotchi/generated/tamagotchi_adult_doctor_strip.png
_ai/art/tamagotchi/generated/tamagotchi_evolve_strip.png
_ai/art/tamagotchi/generated/tamagotchi_mess_strip.png
_ai/art/tamagotchi/generated/animation-prompts.md
_ai/art/tamagotchi/generated/strip-manifest.json
_ai/art/tamagotchi/generated/strip-qa.md
_ai/art/tamagotchi/generated/tamagotchi-animation-contact-sheet.png
```

Do not append `final`, `approved`, dates, or revision suffixes to accepted strip filenames. Revisions are recorded in `animation-prompts.md` and `strip-manifest.json`; rejected image revisions remain in `/tmp`.

## Reconciled 17-group/45-frame manifest

The source set is fixed. Phase 4 may not silently add, omit, merge, or reorder a group.

| Order | Group | Source strip | Frames | Later target | Playback | Identity input |
| ---: | --- | --- | ---: | --- | --- | --- |
| 1 | `tamagotchi_egg` | `tamagotchi_egg_strip.png` | 2 | 32x32 each | loop | child, style/palette only |
| 2 | `tamagotchi_child_idle` | `tamagotchi_child_idle_strip.png` | 3 | 32x32 each | loop | child |
| 3 | `tamagotchi_adult_idle` | `tamagotchi_adult_idle_strip.png` | 3 | 32x32 each | loop | adult |
| 4 | `tamagotchi_child_feed` | `tamagotchi_child_feed_strip.png` | 3 | 32x32 each | one shot | child |
| 5 | `tamagotchi_adult_feed` | `tamagotchi_adult_feed_strip.png` | 3 | 32x32 each | one shot | adult |
| 6 | `tamagotchi_child_play` | `tamagotchi_child_play_strip.png` | 3 | 32x32 each | one shot | child |
| 7 | `tamagotchi_adult_play` | `tamagotchi_adult_play_strip.png` | 3 | 32x32 each | one shot | adult |
| 8 | `tamagotchi_child_clean` | `tamagotchi_child_clean_strip.png` | 3 | 32x32 each | one shot | child |
| 9 | `tamagotchi_adult_clean` | `tamagotchi_adult_clean_strip.png` | 3 | 32x32 each | one shot | adult |
| 10 | `tamagotchi_child_rest` | `tamagotchi_child_rest_strip.png` | 2 | 32x32 each | one shot | child |
| 11 | `tamagotchi_adult_rest` | `tamagotchi_adult_rest_strip.png` | 2 | 32x32 each | one shot | adult |
| 12 | `tamagotchi_child_sick` | `tamagotchi_child_sick_strip.png` | 2 | 32x32 each | loop | child |
| 13 | `tamagotchi_adult_sick` | `tamagotchi_adult_sick_strip.png` | 2 | 32x32 each | loop | adult |
| 14 | `tamagotchi_child_doctor` | `tamagotchi_child_doctor_strip.png` | 3 | 32x32 each | one shot | child |
| 15 | `tamagotchi_adult_doctor` | `tamagotchi_adult_doctor_strip.png` | 3 | 32x32 each | one shot | adult |
| 16 | `tamagotchi_evolve` | `tamagotchi_evolve_strip.png` | 4 | 32x32 each | one shot | child and adult |
| 17 | `tamagotchi_mess` | `tamagotchi_mess_strip.png` | 1 | 16x16 | static overlay | child, style/palette only |

Count reconciliation:

```text
egg                                      2
child/adult idle                         6
child/adult feed                         6
child/adult play                         6
child/adult clean                        6
child/adult rest                         4
child/adult sick                         4
child/adult doctor                       6
evolve                                   4
                                        --
32x32-target frames                     44
mess, 16x16 target                       1
                                        --
total groups / frames              17 / 45
raw runtime cost expected in phase 6:
44 * 32 * 32 * 4 + 1 * 16 * 16 * 4 = 181,248 bytes
```

The 181,248-byte value is approximately 177 KiB and remains under the initial 200 KiB Tamagotchi sprite budget before descriptor/linker overhead. Source-strip pixel dimensions do not affect this estimate; phase 5 output dimensions do.

## Strip geometry contract

Every accepted strip must satisfy all of these rules:

- one horizontal row and exactly the manifest frame count;
- frames ordered left to right in playback order;
- one equally sized logical cell per frame;
- PNG width exactly divisible by `frameCount`;
- `frameWidth = sourceWidth / frameCount`, with a common `frameHeight = sourceHeight`;
- no header, caption, number, grid, separator, colored gutter, film border, or thumbnail label;
- no pose, prop, outline, glow, or alpha residue crossing a cell boundary;
- every cell has enough transparent clearance that its entire intended subject is unambiguous when split on the exact cell boundary;
- a square cell is preferred, but equal, explicitly measured rectangular cells are acceptable because phase 5 crops content before normalization;
- for the one-frame mess asset, the entire PNG is the single logical cell and must still be recorded as a horizontal layout with `frameCount = 1`;
- the strip must be RGBA with genuine transparency. A baked checkerboard or an opaque canvas is a hard rejection.

Do not manually cut and recombine good-looking frames from unrelated generations to manufacture a strip. That loses generation lineage and encourages identity/scale drift. If the generator cannot honor the exact cell count or clean boundaries, regenerate or use a reference-based edit of that same strip.

Phase 4 preserves accepted source strips as generated. It records partial-alpha edges, palette excess, and small isolated transparent-edge residue for phase 5, but does not normalize them. A full opaque rectangle, background scene, baked checkerboard, or subject crossing boundaries is too ambiguous for automated cleanup and must be rejected here.

## Frozen visual invariants

Repeat these constraints in every generation prompt; never rely on “same as before” or hidden conversation context.

### Identity

- Preserve the exact life-stage master’s face-patch geometry, cream forehead notch, visible ear, eye spacing, snout direction, cream belly patch, quill language, and limb proportions.
- Preserve the front three-quarter view facing screen-right. Do not mirror a frame.
- The child remains rounder and shorter; the adult remains modestly taller with a longer snout/limbs and more developed quill lobes.
- Never add clothing, accessories, hands/fingers, realistic spines, or a new identity mark.
- Props are generic action cues, not persistent character features, and must disappear outside their specified action.
- One pet at most per cell. The egg and mess cells contain no pet.

### Scale, center, and ground line

- Hold torso/head scale constant within every same-stage strip and across all same-stage strips.
- Keep the phase 3 visual center fixed within the logical cell; limb reach and small effects may vary around it.
- Keep standing feet on the phase 3 ground line. A one-pixel-equivalent upward/downward body shift is allowed only where the ordered pose calls for a bob, jump, slump, or lowered rest pose.
- Do not make the pet smaller merely to fit a prop. The prop must yield to the pet’s scale.
- Keep all quills, feet, outlines, and props inside their cell with economical transparent padding. Phase 5 must have room to place the normalized child around x=16/y=28 and the adult on the same anchors.
- Rest and sick poses may lower the head/body but must retain the same implied floor plane; no cast shadow or floor mark may define that plane.

### Palette, pixels, and lighting

- Use the exact semantic palette roles frozen by phase 3. Minor source-generation deviations must be recorded, not silently treated as new approved colors.
- Use crisp, hard-edged, limited-palette pixel art with the same dark outline weight in every cell.
- Keep one upper-left flat-step light direction. Do not change highlight side between poses.
- No gradient, glow, blur, soft painterly edge, high-frequency dithering, or photoreal texture.
- Important face and identity marks must remain readable in a nearest-neighbor 32x32 preview.
- Effects and generic props may use at most the existing palette plus the smallest necessary approved accent. Any new accent color requires an explicit note and cross-group review; it does not redefine the pet palette.

### Transparency and composition

- Require a true alpha channel and alpha-zero pixels at all four strip corners and around each cell subject.
- No room, floor, cast shadow, scenery, UI, words, letters, numerals, logo, signature, watermark, emoji, or baked status symbol.
- No antialiased colored halo. Narrow intermediate-alpha contours may be passed to phase 5 only if they composite cleanly on black, white, saturated magenta, and card green and are documented.
- Effects remain small and local. They must not fill the canvas, obscure identity landmarks, or become the dominant silhouette.

## Per-group pose and action direction

Each numbered beat below is one left-to-right frame. A candidate is wrong if its pictures are attractive but tell a different sequence.

| Group | Ordered frame direction | Specific acceptance emphasis |
| --- | --- | --- |
| `tamagotchi_egg` | 0: intact upright egg tilted slightly left; 1: same egg tilted slightly right, on the same base/center. | A compact original shell using project palette/shape language; no face, pet, crack, nest, shadow, or hatch event. The two-frame wrap must read as a gentle wobble rather than a teleport. |
| `tamagotchi_child_idle` | 0: canonical alert neutral; 1: very small lower-body bob or blink; 2: gentle return/rise close to frame 0. | Preserve the child master most strictly here; frames 2 -> 0 must loop without a scale or ground-line pop. |
| `tamagotchi_adult_idle` | 0: canonical adult alert neutral; 1: subtle breath/blink; 2: gentle return close to frame 0. | Adult maturity must remain visible, but motion amplitude stays as restrained as the child loop. |
| `tamagotchi_child_feed` | 0: child anticipates a tiny generic food morsel beside the snout; 1: lean/bite with morsel contacting the mouth; 2: upright satisfied chew/swallow with morsel reduced or gone. | Food must read at 32x32 without looking like inventory or a branded item; do not move the whole pet across the cell. |
| `tamagotchi_adult_feed` | Same three beats as child feed using the adult anatomy and identical generic food language. | Preserve adult snout, quills, body size, and common prop scale; do not collapse into the child form while leaning. |
| `tamagotchi_child_play` | 0: playful ready/lean with one tiny generic ball or simple toy; 1: contact, hop, or paw motion; 2: happy recovery with toy settled nearby. | One toy only, modest motion, readable joy without a mini-game scene; feet/body return toward the common anchor. |
| `tamagotchi_adult_play` | Same ready/contact/recovery arc with the adult and the same toy design. | Retain adult scale and maturity; do not resize the adult to match the child or generate a different toy system. |
| `tamagotchi_child_clean` | 0: normal child with one small bubble/sparkle appearing; 1: two or three localized bubbles/sparkles passing over the body; 2: clean, pleased child with a final small sparkle. | No bath, room, soap bottle, text, shower, second helper, or full-canvas effect. Do not use dirt as a permanent body recolor. |
| `tamagotchi_adult_clean` | Same appear/sweep/clean arc around the adult. | Effects must not clip longer quills or hide the adult identity; match the child effect vocabulary. |
| `tamagotchi_child_rest` | 0: child lowers onto the same floor plane, eyes closing; 1: settled compact breathing pose with closed eyes and a minimal body shift. | No bed, night scene, moon, text, or “Z”; both frames remain a simple explicit rest action. |
| `tamagotchi_adult_rest` | Same lower/settle sequence using the adult body and quills. | Lowered pose must not shrink the adult or alter anatomy; all quills remain inside the cell. |
| `tamagotchi_child_sick` | 0: child visibly slumped with uncomfortable eyes/mouth; 1: tiny weak sway or breath while remaining slumped. | Loop must show discomfort without emoji, thermometer text, gore, palette shift, or a second status icon; identity remains recognizable. |
| `tamagotchi_adult_sick` | Same slumped/weak-loop logic using the adult. | Preserve mature silhouette and same sickness visual language used by the child. |
| `tamagotchi_child_doctor` | 0: sick child with a tiny generic needle/medical cue adjacent; 1: cue approaches or treatment is applied without gore; 2: cue recedes and child rises into a visibly recovering pose. | The cue must be compact, non-graphic, and readable; recovery is the narrative endpoint. No clinic, doctor character, medicine inventory, cross text, or UI. |
| `tamagotchi_adult_doctor` | Same cue/treatment/recovery arc using the adult. | Keep adult scale and quills consistent; match the child medical cue exactly enough to read as the same action. |
| `tamagotchi_evolve` | 0: canonical child at the common anchor; 1: child-to-adult intermediate with slightly lengthening torso/snout/limbs and developing quills; 2: later intermediate clearly nearer adult; 3: canonical adult at the same anchor/ground line. | Both master identities must visibly bookend one deterministic transformation. No alternate form, cocoon, full-screen flash, scenery, text, or accessory. Intermediates may not resemble a third species. |
| `tamagotchi_mess` | 0: one compact, non-graphic, readable waste/mess pile or spill symbol, isolated on transparency. | No pet, face, emoji, flies, text, floor, container, or status badge. It must remain distinct at a 16x16 nearest-neighbor preview and use a restrained subset of the approved palette. |

## Prompt and reference-based generation workflow

Generate one group at a time. Do not batch several groups into one large sheet.

1. Create `animation-prompts.md` before generation. Copy the complete frozen identity and negative constraints from phase 3, record stable prompt IDs such as `CHILD-IDLE-v1`, and spell out every requested frame beat.
2. Immediately before each call, inspect the applicable accepted reference with `view_image`. For an adult group inspect the adult; for a child group inspect the child; for evolve inspect both.
3. Use the built-in image-generation edit/reference flow:
   - child stage group: pass `tamagotchi_child_reference.png`;
   - adult stage group: pass `tamagotchi_adult_reference.png`;
   - evolve: pass both child and adult references and label their left/right endpoint roles explicitly;
   - egg/mess: pass the child reference only as style/palette context, explicitly instructing the tool not to draw the character or edit its anatomy.
4. State that the reference is the identity target for stage strips, not merely a mood board. Ask to change only the listed pose, expression, and permitted generic prop/effect.
5. Request exactly N equally sized frames in a single one-row horizontal strip, left-to-right in the listed order, on a genuinely transparent background with no gutters or separators.
6. Repeat exact identity landmarks, facing, lighting, outline, palette, scale, center, ground plane, transparency, no-background, and originality constraints in that prompt. Never write “same character/style as before” without expanding the contract.
7. Generate one candidate at a time. Save a rejected candidate as `/tmp/deskhog-tamagotchi-phase4/<group>-candidate-NN.png` and record its rejection reason and prompt ID.
8. Inspect the complete strip and every logical cell. If it fails, make one targeted correction while preserving the same reference input. Do not combine unrelated corrections into one revision.
9. When it passes, copy the unmodified accepted PNG to its fixed `_strip.png` path, calculate its metadata/hash, and mark the prompt revision accepted.
10. Re-open the accepted file from its stable repository path and run the complete QA gates. Acceptance based only on the tool preview is invalid.

An executable prompt template is:

```text
Use case: stylized-concept.
Asset type: horizontal pixel-art animation source strip for later 32x32 embedded-display sprites.
Create exactly {N} equally sized frames in one horizontal row, ordered left to right: {complete ordered beats}.
Use the attached accepted {child|adult} desk-hog as the exact identity reference. Preserve {all frozen identity landmarks}, front three-quarter screen-right view, body scale, visual center, shared ground plane, upper-left lighting, strong dark outline, and the accepted limited palette. Change only {pose/expression/permitted prop or effect}.
Each frame must remain readable after nearest-neighbor reduction to 32x32. Keep all pixels and effects within their cell and leave transparent clearance between cells.
Use a genuinely transparent RGBA background. One pet per cell. No text, UI, room, floor, shadow, scenery, logo, watermark, separator, gutter, second character, gradient, glow, soft edge, antialiased halo, branded character trait, or unrelated prop.
```

For the mess prompt replace the final-size preview with 16x16 and require one object/no pet. For evolve name the child reference as frame 0 and adult reference as frame 3. The complete executed prompt, not just this template, must be saved.

## Iteration and rejection policy

Use a strict accept/reject gate; do not select the least-bad strip for phase 5 to redesign.

### Hard rejection

Reject and regenerate/edit when any of these occurs:

- wrong group, frame count, ordering, or action story;
- width not divisible by frame count, unequal cells, visible separator/gutter, or artwork crossing a split boundary;
- identity drift in face patch, forehead notch, ear, eyes, snout, belly patch, quills, stage anatomy, or species;
- mirroring, camera/viewpoint change, scale jump, horizontal wander, or unexplained ground-line jump;
- clipped quill, ear, foot, outline, prop, bubble, sparkle, or medical cue;
- opaque rectangle, baked checkerboard, room, floor, cast shadow, scene, text, logo, watermark, or second character;
- animation prop too small to read or so large that it changes pet scale/framing;
- feed/play/clean/rest/sick/doctor/evolve intent not readable at the final target thumbnail size;
- action introduces a persistent accessory or a new palette/identity feature;
- sick or doctor uses emoji, gore, a human/animal doctor, or a clinic scene;
- evolution skips to an unrelated adult, contains an alternate form, or uses a flash that fills most of a cell;
- mess is graphic, ambiguous at 16x16, contains a face/emoji, or includes the pet;
- recognizable imitation of a commercial virtual-pet or hedgehog character.

### Correctable through one targeted revision

Examples include: retain the entire strip but restore the forehead notch in frame 1; retain poses but align all feet to the original floor plane; retain identity and scale but reduce the bubbles; retain frame order but remove the opaque background. Reinspect all frames after an edit because correcting one frame may drift another.

### Phase 5 cleanup, not Phase 4 redesign

The following may be accepted only when clearly documented and demonstrated not to affect interpretation: narrow clean partial-alpha contours, a few palette deviations that map unambiguously to an approved semantic color, excess transparent outer margin, and subpixel source anchors requiring deterministic normalization. Phase 5 may remove/crop/map/place those. It may not repair missing anatomy, redraw an expression, move a prop from the wrong frame, infer a cell boundary, or reconstruct clipped content.

If three targeted revisions fail for the same defect, abandon that candidate lineage and start a new generation from the canonical reference. If the same defect recurs across independent lineages, stop and revise the full prompt/strip layout request rather than accepting the defect. Do not change the phase 3 master to make one action easier.

## QA and contact-sheet procedure

For each serious candidate and every accepted strip:

1. Record pixel width/height, mode, alpha min/max/value count, visible-color count excluding fully transparent pixels, nontransparent bounding box per cell, and whether the width divides evenly by the frame count.
2. Calculate exact split boundaries as half-open intervals `[x0, x1)` for each cell and verify no visible or nonzero-alpha pixel crosses a boundary unexpectedly.
3. Inspect the strip at original detail.
4. Split only into temporary `/tmp` previews using the recorded boundaries. Do not commit these as runtime frames.
5. Crop/fit each temporary preview with nearest-neighbor only to a non-authoritative 32x32 preview, or 16x16 for mess; also enlarge it 8x with nearest-neighbor for pixel-grid inspection.
6. Composite each temporary cell over checkerboard, black, white, saturated magenta, and the card green used by the current UI reference. Confirm no matte, opaque background, or colored fringe appears.
7. Compare every child cell with the canonical child at target size and every adult cell with the canonical adult. Check face marks, body ratio, scale, center, and palette.
8. Play or inspect frames in order at both target size and intended 2x display size. Check loop seams for egg/idle/sick and narrative order for one-shots.
9. Compare paired child/adult actions side by side so Feed, Play, Clean, Rest, Sick, and Doctor share one visual vocabulary without erasing stage differences.
10. Check evolve frame 0 against the child master and frame 3 against the adult master, then verify both intermediates change monotonically toward adulthood.
11. Check the mess alone at actual 16x16 and beside both life stages at an approximate 2x display preview. It must not overlap the pet or become confused with food.
12. Record pass/fail and precise notes in `strip-qa.md`; do not rely on visual memory.

After all 17 strips pass, produce `tamagotchi-animation-contact-sheet.png` outside `raw-png/`. It should label groups in the surrounding QA layout, not inside source strips, and show:

- all 17 groups in manifest order;
- every source cell at consistent preview scale;
- child/adult action pairs adjacent where practical;
- target-size nearest-neighbor previews and an enlarged pixel view;
- transparent regions against a checkerboard plus at least one dark and one light background;
- no resampling other than nearest-neighbor for target-size previews.

The contact sheet is review evidence, never a phase 5 split input and never a runtime asset.

## `animation-prompts.md` requirements

Record:

- the copied phase 3 identity bible and palette values;
- shared negative constraints in full;
- one base prompt plus the complete expanded executed prompt for each group;
- stable prompt IDs/revision numbers;
- exact reference path(s), hashes, and whether each was an identity edit target or style/palette-only reference;
- generation date, built-in tool/model/version if exposed, generation ID/output hint if exposed;
- every targeted revision in execution order;
- rejection reason for each revision and acceptance reason for the winner.

Do not record shorthand dependent on conversation history.

## `strip-manifest.json` contract

Use `schemaVersion: 1`, `expectedGroupCount: 17`, `expectedFrameCount: 45`, `target32FrameCount: 44`, `target16FrameCount: 1`, and `estimatedArgb8888Bytes: 181248` at the top level. Include generation tool information, creation date, phase 3 manifest path/hash, and a `groups` array in the fixed manifest order.

Each group entry must include:

- `group`, `source`, `sha256`, `byteSize`, `width`, `height`, and `colorMode`;
- `frameCount`, `layout: "horizontal"`, `frameWidth`, `frameHeight`, and ordered half-open `splitBoundaries` with `index`, `x0`, and `x1`;
- `targetRuntimeWidth` and `targetRuntimeHeight` (32/32, except mess 16/16);
- `playback` (`loop`, `oneShot`, or `static`) and an ordered `frameIntent` array;
- `identityReferences`, each with relative path, SHA-256, and role (`identity` or `stylePaletteOnly`);
- accepted `promptId`, revision, generation metadata, approval date, reviewer, and acceptance notes;
- observed `alphaMin`, `alphaMax`, `alphaValueCount`, `visibleColorCount`, and per-cell nontransparent bounding boxes;
- observed facing, light direction, visual-center and ground-line measurements per cell where a pet exists; use explicit `null` plus a reason for egg/mess properties that do not apply;
- palette comparison against phase 3 semantic hex roles;
- `requiredPhase5Corrections`, an array that may name alpha thresholding, palette mapping, transparent-margin crop, and anchor placement, but never creative redraw work.

Use repository-relative paths only. Validate the JSON parses, contains 17 unique group names and source paths, has split-boundary counts equal to frame counts, and reconciles to 45 frames before exit.

## Failures and decision branches

- **Phase 3 input absent/hash mismatch:** stop and return to phase 3; do not repair or replace a master here.
- **Canonical pet is unreadable in action poses at 32x32:** first simplify motion/prop/effect while preserving identity. If neutral identity itself is unreadable, reopen phase 3.
- **Generator returns separate images instead of one strip:** retry with the one-row/equal-cell constraint. Do not stitch unrelated candidates.
- **Wrong number of frames or ambiguous cell boundaries:** reject. Phase 5 must never guess a split.
- **Opaque/checkerboard background or cell-spanning scene:** request a targeted transparent-background correction, then reject the lineage if it persists.
- **Minor partial-alpha halo or palette excess:** accept only if composites remain clean and phase 5 mapping is objective; record exact cleanup. Otherwise reject.
- **Child/adult action pair uses inconsistent props/effects:** keep the already approved stronger group and regenerate the paired group from its correct master with the shared prop/effect direction documented in the prompt.
- **Adult repeatedly collapses into child proportions:** restart from the canonical adult reference and emphasize adult-only anatomy; never upscale afterward to fake maturity.
- **Evolution endpoints drift:** reject; both references are mandatory. If the tool cannot preserve both endpoints in one strip, generate a new reference-based evolution lineage, not hand-built unrelated frames.
- **A prop cannot fit without clipping:** reduce/simplify the prop or pose. Do not shrink the pet, move the ground line, or widen only one cell.
- **Originality concern:** reject and regenerate. Cosmetic edits are insufficient for recognizable derivative art.
- **Source strip is unusually large:** source size alone is not firmware cost because it stays outside `raw-png/`. Still prefer manageable files; runtime size is fixed by phase 5.
- **Phase 1 OTA budget no longer supports 181,248 raw bytes plus overhead:** report the capacity blocker to the overall plan owner. Do not reduce counts in Phase 4 without an explicit manifest revision; if approved, reduce action frames before sacrificing idle/sick legibility as the master plan directs.

## Execution sequence

1. Validate and hash the four phase 3 inputs.
2. Create the accepted-output and temporary candidate directories; do not touch `raw-png/`.
3. Initialize `animation-prompts.md`, `strip-manifest.json`, and `strip-qa.md` with the fixed 17-group order.
4. Generate and approve child idle, then adult idle; use them as motion-amplitude checks against the masters.
5. Generate and approve child/adult sick loops and rest actions; these establish controlled body lowering without scale drift.
6. Generate paired Feed, Play, Clean, and Doctor groups, reviewing each child/adult pair before moving on.
7. Generate and approve the egg using child style/palette context only.
8. Generate and approve the one-frame mess overlay at the 16x16 readability target.
9. Generate evolve last, using both accepted masters and the now-stable cross-stage visual vocabulary.
10. Re-open all 17 stable paths, recompute hashes and metadata, and run the complete per-cell QA.
11. Create the contact sheet and conduct cross-group identity, scale, palette, ground-line, and action-consistency review.
12. Parse and validate the manifest; reconcile 17 groups, 45 total frames, 44 32x32 targets, one 16x16 target, and 181,248 raw ARGB8888 bytes.
13. Review `git status`. Exactly the intended `_ai/art/tamagotchi/generated/` artifacts may be new/changed; `raw-png/`, `include/sprites/`, and source code must remain unchanged.

## Deliverables

Phase 4 delivers:

1. the 17 accepted, fixed-name source strips under `_ai/art/tamagotchi/generated/`;
2. `animation-prompts.md` with complete reproducible prompts, revisions, input lineage, and acceptance decisions;
3. valid `strip-manifest.json` with exact group/frame geometry, hashes, image facts, references, and phase 5 cleanup requirements;
4. `strip-qa.md` with per-strip and cross-strip inspection results;
5. `tamagotchi-animation-contact-sheet.png` as human review evidence.

No individual frame under `raw-png/` and no generated C/H file is a Phase 4 deliverable.

## Exit criteria

Phase 4 is complete only when:

1. all 17 fixed source-strip paths exist and their hashes match the parsed manifest;
2. exact cell counts reconcile to 45 frames: 44 intended for 32x32 and one intended for 16x16;
3. every width divides by its frame count and every recorded split boundary is equal, ordered, non-overlapping, and covers the full strip width;
4. every loop is seam-readable and every one-shot tells the required action in left-to-right order at target thumbnail size;
5. every child/adult frame preserves the approved identity, life-stage anatomy, scale, screen-right viewpoint, center, ground plane, lighting, outline, and palette roles;
6. feed, play, clean, rest, sick, doctor, evolve, egg, and mess each pass their specific direction and rejection gates;
7. all strips use real transparency with no baked background, scenery, UI, text, logo, watermark, separator, or clipped content;
8. any remaining cleanup is objective and fully recorded for phase 5; no accepted strip requires creative reconstruction;
9. prompts, manifest, QA report, and contact sheet permit review without conversation history;
10. no file under `raw-png/`, `include/sprites/`, or firmware source changed in this phase.

If a strip is merely “fixable later,” the phase is not complete. Phase 5 normalizes pixels and placement; it does not art-direct or repair missing frames.

## Downstream handoff to phase 5

Phase 5 must consume `strip-manifest.json` as the authoritative source list and split geometry. It receives exactly the 17 `_strip.png` files, not the contact sheet or temporary previews. It must verify every source hash before processing.

For each group, phase 5 may perform only the documented deterministic operations: split at recorded boundaries, remove documented minor background/edge residue, convert alpha to binary 0/255, map to the approved limited palette, crop transparent margin, resize with nearest-neighbor only, and place the pet on the shared 32x32 center/ground-line canvas or the mess on 16x16. It must emit globally unique, lexicographically ordered runtime basenames under matching underscore-only `raw-png/<group>/` directories.

Phase 5 must stop and return a strip to Phase 4 if it finds a hash mismatch, wrong frame count, ambiguous boundary, clipped subject, missing alpha, identity drift, unreadable action, or any correction that would require drawing or subjective reconstruction. Any requested identity change instead returns to Phase 3.
