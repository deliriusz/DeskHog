# Tamagotchi implementation phase 3: canonical pet artwork

## Phase outcome

Produce and approve two original, canonical character masters—one child and one adult—that define the Tamagotchi pet identity for every later animation. Preserve the exact prompts and generation provenance so phase 4 can create the egg, idle, action, sickness, treatment, evolution, and mess assets without redesigning the character.

This is an art-direction and source-reference phase. Its committed deliverables are high-resolution reference PNGs and documentation under _ai/art/tamagotchi/. Nothing created here is a runtime sprite or belongs under raw-png/.

## Scope

This phase includes:

- treating all six existing walking frames as read-only visual references;
- defining one original desk-hog identity shared by child and adult;
- generating and selecting the canonical child first;
- deriving the adult from the accepted child through a reference-based edit;
- validating both masters at source size and through a temporary 32x32 nearest-neighbor preview;
- verifying real transparency, clean edges, recognizable silhouettes, and a limited palette;
- recording exact prompts, reference inputs, tool provenance, acceptance notes, and file hashes;
- handing stable identity and composition constraints to phase 4.

## Non-goals

Do not:

- generate the egg, action animations, sickness, treatment, evolution, or mess overlay;
- generate horizontal strips or individual runtime frames;
- write the phase 5 sprite-processing script;
- resize, quantize, or normalize final runtime assets;
- add files anywhere under raw-png/;
- run png2c.py or change anything under include/sprites/;
- change LVGL, model, build, or firmware code;
- copy, trace, recolor, or closely imitate an existing Tamagotchi character or another branded mascot;
- commit rejected candidates unless a reviewer explicitly needs them.

Phase 4 owns animation-strip production, phase 5 owns deterministic normalization and placement on 32x32 canvases, and phase 6 owns conversion to embedded LVGL descriptors.

## Dependencies and start gate

### From phase 1

Confirm phase 1 recorded:

- the configured firmware build result;
- baseline firmware.bin size and headroom in one 0x1F0000 OTA application slot;
- Pillow and NumPy availability in PlatformIO's Python environment;
- pre-existing worktree changes.

Phase 3 does not embed data, but the baseline must support the later target of 44 32x32 frames plus one 16x16 mess frame, near or below the 200 KiB Tamagotchi sprite budget. If that budget is already impossible, revise later frame counts before phase 4. Do not solve a budget problem by making the canonical face illegible.

### From phase 2

Treat these model contracts as fixed:

- stages are exactly Egg, Child, and Adult;
- TamagotchiEggType has one MVP value, Default;
- TamagotchiLocation has one MVP value, Room;
- child-to-adult evolution is deterministic and has no branch;
- care quality does not select a form;
- no death, senior stage, personality, rarity, or alternate adult exists.

If src/tamagotchi/TamagotchiTypes.h exists when this plan is executed, verify those names before approval. Resolve any mismatch as a contract issue; do not invent extra art variants.

## Exact inputs

Use these six files only as style and technical references. Never modify or overwrite them:

    raw-png/walking/Normal-Walking_01.png
    raw-png/walking/Normal-Walking_02.png
    raw-png/walking/Normal-Walking_03.png
    raw-png/walking/Normal-Walking_04.png
    raw-png/walking/Normal-Walking_05.png
    raw-png/walking/Normal-Walking_06.png

Keep these repository contracts available while reviewing candidates:

    _ai/plans/tamagochi-features.md
    _ai/plans/tamagochi-implementation-plan.md
    docs/assets.md
    png2c.py
    platformio.ini
    src/ui/FriendCard.cpp
    include/sprites/sprites.h
    include/sprites/sprites.c

## Exact outputs

Commit only these accepted-art deliverables:

    _ai/art/tamagotchi/references/tamagotchi_child_reference.png
    _ai/art/tamagotchi/references/tamagotchi_adult_reference.png
    _ai/art/tamagotchi/references/reference-manifest.json
    _ai/art/tamagotchi/prompts.md

Use /tmp/deskhog-tamagotchi-phase3/ for rejected candidates and temporary 32x32, checkerboard, light-background, and dark-background previews. Only accepted masters move into the repository.

Never put masters, candidates, previews, contact sheets, or prompt images under raw-png/. png2c.py recursively converts every PNG there and platformio.ini compiles the resulting C, so a high-resolution reference in that tree would be embedded in firmware. Never hand-edit include/sprites/.

## Walking-reference inventory and observations

All six files were inspected individually with view_image and together as a nearest-neighbor sequence. Every file is an 80x80 RGBA PNG. Every alpha channel contains only 0 and 255. Each frame uses the same 11 visible colors and a stable brown, tan, peach, black, gray, and white palette.

| Frame | Opaque bounding box | Visual observation | Production lesson |
| --- | --- | --- | --- |
| Normal-Walking_01.png | x 13–64, y 18–77; 52x60 | Open eyes, upright pose, widest silhouette, extended front-side limb and broad gray ground mark. | Best representative style input because face, belly, outline, and overall mass are all visible. The ground mark is reference context, not part of the new master. |
| Normal-Walking_02.png | x 13–63, y 18–77; 51x60 | Open eyes, compact early-stride pose, limb moved inward while face and belly remain unchanged. | Pose changes should be localized; facial proportions and belly mass must not drift. |
| Normal-Walking_03.png | x 13–62, y 18–76; 50x59 | Closed-eye expression, bent limb, slightly raised lower silhouette. | A tiny expression change remains readable when the large identity shapes stay fixed. |
| Normal-Walking_04.png | x 13–62, y 17–75; 50x59 | Open eyes, highest point in the bob, alternating foot placement. | One-pixel-scale vertical motion is enough; future animations should not redraw or rescale the body. |
| Normal-Walking_05.png | x 13–61, y 18–77; 49x60 | Open eyes, straight hanging limb, lower recovery pose. | Limb poses can read as simple dark shapes without extra detail. |
| Normal-Walking_06.png | x 13–60, y 18–77; 48x60 | Open eyes, narrowest recovery silhouette, stable face and belly. | Preserve a fixed visual center and ground anchor even as the extremities change. |

Shared reference characteristics:

- a strong, mostly black pixel outline separates the pet from any background;
- large contiguous face and belly shapes carry the character at small size;
- quills use a dark-to-mid brown pattern with restrained texture;
- eyes, nose, and mouth use very few pixels;
- short legs and arms read through silhouette rather than interior detail;
- the character faces screen-right in a front three-quarter view;
- animation comes from small limb, expression, and one-pixel body shifts;
- substantial transparent padding surrounds the 48–52 by 59–60 occupied region;
- the gray/black floor mark is baked into the references, but the Tamagotchi master must omit it because the card will supply the room with LVGL primitives.

The new pet should inherit the reference vocabulary—limited palette, hard pixel edges, strong outline, readable masses, restrained motion—not its exact outline or anatomy. Do not trace a walking frame.

## Canonical identity bible

The following invariants become frozen once the child is accepted.

### Shared identity

- Species and concept: an original, friendly desk-hog virtual pet; clearly hedgehog-like, but not a recreation of a known virtual-pet mascot.
- View: full-body front three-quarter view, looking toward screen-right. Do not mirror the canonical masters.
- Silhouette: compact pear-shaped torso, distinct rounded face patch, short blunt snout, small rounded visible ear, short limbs, and a rear quill mass made of a few large readable lobes.
- Face: two dark simple eyes with at most one tiny highlight each, a small dark nose at the snout tip, and a one- or two-pixel-equivalent neutral-friendly mouth.
- Identity marks: a small cream forehead notch above the visible eye and a simple oval cream belly patch. These must survive every action and both life stages; do not add clothing or handheld identity props.
- Quills: chunky layered forms, not realistic individual needles. Texture may use sparse deliberate clusters, never noisy high-frequency dithering that collapses at 32x32.
- Outline: one consistent dark pixel-equivalent contour, thickened only where necessary to separate overlapping limbs.
- Lighting: one fixed upper-left light direction represented by flat color steps; no gradients, glow, rim light, or cast shadow.
- Palette: 8–12 visible colors, including one dark outline, two or three quill browns, two warm face/belly tones, one highlight, one blush/accent, and no more than two neutral support colors. Child and adult share exactly the same palette roles.
- Expression: alert, gentle, and slightly mischievous; not distressed in the canonical neutral masters.
- Grounding: both feet touch one shared ground line. No floor, ellipse, dust, or shadow is part of the artwork.

### Child form

- Rounder head and torso with a head-to-body ratio that reads young.
- Shortest limbs and three or four blunt quill lobes.
- Smaller snout and larger apparent eye area than the adult.
- Target normalized 32x32 occupancy: approximately 20–23 pixels wide and 22–25 pixels high.
- Target ground line: lowest opaque foot pixel at y=28, leaving at least three transparent rows below.
- Target horizontal visual center: x=16 within one pixel.
- Leave at least two transparent pixels around all other extrema so later action props have room.

### Adult form

- Must remain unmistakably the same individual: same face-patch shape, forehead notch, ear, eye spacing, snout direction, belly patch, outline language, and palette.
- Make adulthood visible anatomically, not through an accessory: torso two or three final pixels taller, slightly longer snout, modestly longer limbs, and four or five more developed quill lobes.
- Reduce the apparent head-to-body ratio slightly, but retain the cute compact silhouette.
- Target normalized 32x32 occupancy: approximately 23–26 pixels wide and 25–28 pixels high.
- Use the same x=16 visual center and y=28 ground line as the child.
- Do not turn the adult into a different species, a muscular form, or merely a scaled child.

### Forbidden identity drift

Reject a candidate if it changes the viewpoint, adds clothing, relies on a prop for recognition, changes the face-mask geometry, changes palette roles, adds detailed fingers/toes, uses realistic spines, becomes bipedal in one form and quadrupedal in the other, or resembles a branded Tamagotchi, Sonic, or another recognizable character.

## Prompt design and reproducibility contract

Use the built-in image-generation tool. Do not use an API, local model, third-party image service, or CLI fallback unless the user explicitly requests that change.

Generation is not assumed to be bit-for-bit deterministic. Reproducibility means preserving:

- the complete prompt with no omitted conversational context;
- a stable prompt identifier and revision number;
- the exact local reference path and its SHA-256 hash;
- whether the input was style-only or an edit/reference target;
- generation date and tool/model identifier when exposed;
- returned generation identifier or output hint when exposed;
- source dimensions, color mode, and alpha observations;
- every targeted correction applied after the first attempt;
- the reason the accepted candidate won.

Write _ai/art/tamagotchi/prompts.md with these sections:

1. frozen identity bible;
2. shared negative constraints;
3. CHILD-CANONICAL-v1 exact prompt;
4. each child revision prompt in execution order;
5. ADULT-CANONICAL-v1 exact edit prompt;
6. each adult revision prompt in execution order;
7. accepted palette with exact hexadecimal values sampled from the child;
8. notes for phase 4 on pose, center, ground line, and allowable expression changes.

Never write shorthand such as “same as before” in the saved prompts. Expand all identity invariants so a later agent can reproduce the request without hidden chat context.

## Child generation workflow

1. Confirm raw-png/walking/Normal-Walking_01.png is unchanged and inspect it with view_image immediately before generation.
2. Pass that local file as a style reference. State explicitly that it is not an edit target and must not be traced.
3. Generate one candidate at a time. Do not request a sheet of variations because selection and alpha inspection must be unambiguous.
4. Use this base request, expanded with the identity bible:

       Use case: stylized-concept.
       Asset type: pixel-art game character master for a 240x135 embedded TFT.
       Create an original tiny desk-hog child virtual pet that remains readable after nearest-neighbor reduction to 32x32.
       The attached walking sprite is style reference only for pixel density, limited palette, hard outline, and small-scale readability. Do not edit, trace, copy its silhouette, or reproduce its ground shadow.
       Show one full-body character, centered, front three-quarter view facing screen-right, both feet on a shared ground line.
       Use a compact pear body, rounded face patch, short blunt snout, small visible ear, simple short limbs, chunky quill lobes, cream forehead notch, and oval cream belly patch.
       Use 8-12 high-contrast flat colors, a strong dark pixel outline, sparse deliberate texture, and upper-left lighting.
       Provide a genuinely transparent background with economical transparent padding.
       One character only. No text, logo, watermark, scenery, floor, cast shadow, glow, gradient, soft painterly edge, antialiased halo, or extra props.
       Avoid every existing Tamagotchi character, Sonic, branded mascot traits, photorealism, and excessive detail.

5. Save each unaccepted result only under /tmp/deskhog-tamagotchi-phase3/child-candidate-NN.png.
6. Inspect and reject or request one targeted correction. Examples: “retain everything but simplify the quill texture,” “retain the face and palette but remove the opaque floor,” or “retain identity but enlarge the eyes for 32x32 readability.”
7. Do not stack unrelated corrections in one revision. A one-defect edit makes drift diagnosable.
8. When a child passes every check, preserve the unmodified accepted PNG as _ai/art/tamagotchi/references/tamagotchi_child_reference.png.

## Adult generation workflow

1. Do not start until the child reference is accepted, named, hashed, and recorded.
2. Inspect the accepted child with view_image immediately before generating the adult.
3. Pass tamagotchi_child_reference.png as the edit/reference image. The adult must be a derived variant, not an unrelated fresh generation.
4. Use this base request, expanded with the full shared identity:

       Create the single adult life-stage variant of this exact original desk-hog.
       Preserve face-patch geometry, forehead notch, ear, eye spacing, snout direction, belly patch, palette, outline, viewpoint, upper-left lighting, visual center, and ground line.
       Show adulthood only through a modestly taller torso, slightly longer snout and limbs, a slightly smaller head-to-body ratio, and four or five more developed chunky quill lobes.
       Keep the pet cute, compact, full-body, neutral-friendly, and readable after nearest-neighbor reduction to 32x32.
       Use the same genuinely transparent background and economical padding.
       Do not add clothing, accessories, text, logo, watermark, floor, shadow, scenery, gradient, glow, soft edges, extra character, alternate form, or branded character traits.

5. Save rejected results only as /tmp/deskhog-tamagotchi-phase3/adult-candidate-NN.png.
6. Compare each adult directly against the accepted child at source size and 32x32. Reject identity changes even if the result looks polished.
7. Make one targeted edit per iteration.
8. Preserve the unmodified accepted PNG as _ai/art/tamagotchi/references/tamagotchi_adult_reference.png.

## Transparency and originality checks

Both masters must:

- be RGBA PNGs with real alpha; a checkerboard drawn into RGB pixels is a hard rejection;
- contain alpha-zero pixels at every canvas corner;
- have no opaque rectangle, floor strip, shadow, glow, or scenery;
- show no colored matte when composited over black, white, saturated magenta, and the green used by FriendCard;
- avoid a soft semitransparent halo. Binary alpha is preferred; if intermediate alpha remains in the high-resolution master, it may only form a narrow clean contour with no colored fringe and must be recorded for phase 5 normalization;
- contain one character and no words, logos, signatures, or watermarks;
- remain visibly distinct from the six references in exact silhouette and from known commercial virtual-pet and hedgehog mascots;
- use generic hedgehog anatomy and project-owned identity marks, not protected character-specific shell shapes, eyes, accessories, costumes, or color blocking.

If there is doubt about originality, reject and regenerate. Do not try to repair a recognizably derivative candidate by making small cosmetic changes.

## Source-size and 32x32 inspection procedure

For every serious candidate:

1. Inspect the original PNG with view_image at original detail.
2. Record width, height, color mode, alpha extrema, alpha-value count, visible color count, and non-transparent bounding box.
3. Composite temporary previews over checkerboard, black, white, magenta, and card green. These files stay in /tmp.
4. Create a temporary 32x32 preview using nearest-neighbor only. Do not use bilinear, bicubic, Lanczos, smoothing, or AI up/downscaling.
5. Also create a temporary 256x256 display preview by enlarging the 32x32 result 8x with nearest-neighbor. Inspect both the actual 32x32 thumbnail and enlarged pixel grid.
6. Preview child and adult side by side at 32x32 and at the intended LVGL 2x display size, approximately 64x64.
7. Simulate binary-alpha normalization in a temporary preview if the source contains partial alpha. Do not overwrite the master.
8. Confirm the face, nose direction, forehead notch, belly patch, feet, and adult quill difference all survive.
9. Delete or leave all previews under /tmp; never promote them to raw-png/.

The 32x32 check is an approval gate, not the final normalization step. Phase 5 will perform deterministic crop, palette, binary-alpha, center, and ground-line processing.

## Selection and rejection criteria

### Accept a child only when

- its silhouette reads as a cute desk-hog at actual 32x32 size;
- both eyes/face, nose direction, belly patch, feet, and quill mass remain distinguishable;
- it matches the repository's hard-edged, limited-palette visual language without tracing the walking pet;
- it leaves sufficient negative space for food, toy, bubbles, medical cue, and motion in later strips;
- its neutral pose is suitable as the identity anchor for idle and all actions;
- the alpha and edge checks pass.

### Accept an adult only when

- a blind side-by-side viewer recognizes it as the same individual;
- adulthood is visible at 32x32 without relying on a caption or accessory;
- its larger body and quills still fit the planned common center and ground line;
- its palette and feature placement agree with the child;
- it remains compact enough to leave action-prop space;
- the alpha and edge checks pass.

### Hard rejection conditions

Reject any candidate with:

- opaque or baked checkerboard background;
- cast shadow, floor, scenery, text, logo, or watermark;
- clipped quills, ears, feet, or outline;
- fuzzy antialiasing or a bright/dark alpha fringe;
- more than 12 meaningful colors after ignoring transparent RGB;
- important one-pixel features that disappear at 32x32;
- weak silhouette, ambiguous face direction, or merged feet;
- a child/adult viewpoint, lighting, ground line, or palette mismatch;
- adult identity drift or an adult that is only a uniform scale-up;
- close resemblance to a recognizable commercial character;
- tiny noisy texture, gradients, or detail that phase 5 would have to invent rather than normalize.

Do not select the “least bad” candidate. Iterate until both masters pass.

## Naming and metadata

Accepted filenames are fixed and lowercase with underscores:

- tamagotchi_child_reference.png
- tamagotchi_adult_reference.png

Do not append final, latest, v2, or approved to accepted filenames. Version prompts and provenance, not the stable downstream paths.

reference-manifest.json must contain:

- schemaVersion set to 1;
- tool name and exposed model/version;
- generation date in ISO 8601;
- child and adult prompt IDs;
- each accepted file's relative path, SHA-256, byte size, pixel dimensions, PNG mode, alpha minimum and maximum, alpha-value count, visible-color count, and opaque bounding box;
- each generation's input reference relative path and SHA-256;
- child/adult intended facing direction, visual center, ground line, and target 32x32 occupied size;
- exact selected palette hex values and semantic roles;
- explicit acceptance reviewer and notes;
- a note that these are non-runtime masters and must not be consumed by png2c.py.

Use relative repository paths in committed metadata. Do not record machine-specific absolute paths. Validate that the JSON parses before completing the phase.

## Execution sequence

1. Verify phase 1 and phase 2 gates.
2. Confirm the six walking sources and repository outputs are unchanged.
3. Create only the _ai/art/tamagotchi/references/ directory needed for accepted deliverables; use /tmp for candidates.
4. Write the first complete child prompt to prompts.md before invoking generation.
5. Generate one child using Normal-Walking_01.png as style-only reference.
6. Inspect source, alpha composites, 32x32 preview, and 2x display preview.
7. Iterate one defect at a time and preserve every exact revision prompt.
8. Promote one passing child to the stable child reference path.
9. Freeze its exact palette, proportions, landmarks, SHA-256, and prompt ID.
10. Write the complete adult edit prompt.
11. Generate the adult from the accepted child.
12. Inspect the adult alone and in direct side-by-side comparison with the child at both sizes.
13. Iterate one defect at a time, then promote one passing adult.
14. Write and parse reference-manifest.json.
15. Re-read prompts.md without chat context and confirm it is sufficient for phase 4 to reproduce identity.
16. Review git status and confirm there are no raw-png, include/sprites, source-code, or generated-file changes.

## Validation checklist

- [ ] Phase 1 baseline and OTA headroom are recorded.
- [ ] Phase 2 confirms one child and one adult with no form branches.
- [ ] All six walking frames remain byte-for-byte unchanged.
- [ ] All six walking frames informed the style analysis; Normal-Walking_01.png was the generation style reference.
- [ ] The built-in image-generation tool was used; no unapproved fallback was used.
- [ ] Child was accepted before adult generation began.
- [ ] Adult was generated as a reference-based edit/variant of the accepted child.
- [ ] Both accepted files are RGBA PNGs with real transparent backgrounds.
- [ ] Both pass black, white, magenta, checkerboard, and card-green composite checks.
- [ ] Neither contains a shadow, floor, scenery, text, logo, watermark, or second character.
- [ ] Both pass actual-size 32x32 and 2x display-size inspection.
- [ ] The child is readable and leaves action-prop space.
- [ ] The adult is recognizably the same pet and visibly mature at 32x32.
- [ ] Shared face, forehead notch, belly patch, palette, viewpoint, light direction, center, and ground line are stable.
- [ ] The design is original and not recognizably derived from a commercial virtual-pet or hedgehog mascot.
- [ ] prompts.md contains complete prompts and every targeted correction.
- [ ] The manifest parses and includes hashes, image facts, anchors, palette, and acceptance notes.
- [ ] Exactly two accepted reference PNGs exist under the references directory.
- [ ] No phase 3 PNG exists under raw-png/.
- [ ] No file under include/sprites/ changed.
- [ ] No firmware build is claimed as phase 3 asset validation; embedding begins only after phases 5 and 6.

## Exit criteria

Phase 3 is complete only when:

1. both stable reference paths exist and pass the source-size, transparency, originality, and 32x32 readability gates;
2. the adult is a clearly mature version of the same child rather than a second design;
3. prompts.md fully reproduces the identity constraints without relying on conversation history;
4. reference-manifest.json parses and accurately describes both accepted files and their input lineage;
5. only _ai/art/tamagotchi/ contains new art artifacts, with no runtime or generated sprite changes;
6. a reviewer can approve the two masters as the sole identity source for phase 4.

If either character is merely “usable with cleanup,” the phase is not complete. Phase 5 may normalize pixels and placement, but it must not redesign the face, silhouette, palette, or life-stage relationship.

## Downstream handoff to phase 4

Phase 4 receives:

- tamagotchi_child_reference.png as the sole child identity source;
- tamagotchi_adult_reference.png as the sole adult identity source;
- prompts.md as the source of frozen positive and negative prompt invariants;
- reference-manifest.json as the machine-readable provenance, palette, scale, center, and ground-line contract.

For each later animation group, phase 4 must use the matching accepted master as a reference-based input, repeat the complete identity constraints, and change only the pose, expression, or explicitly required generic prop. It must not regenerate either life stage from scratch. The evolution strip must bridge these exact child and adult masters. Generated strips belong under _ai/art/tamagotchi/generated/, outside raw-png/, until phase 5 has split and normalized them.

Any requested downstream change to the face, palette, silhouette, viewpoint, identity marks, center, or ground line reopens phase 3 and requires updated prompts, manifest hashes, and reapproval of both masters.
