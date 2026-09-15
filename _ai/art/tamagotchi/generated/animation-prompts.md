# Tamagotchi Phase 4 animation prompts

Generated with the OpenAI built-in image-generation tool (exposed model metadata: `gpt-image 2.0`) on 2026-09-15. All accepted source strips are unmodified generator outputs.

## Frozen Phase 3 inputs

- Child identity master: `_ai/art/tamagotchi/references/tamagotchi_child_reference.png` — SHA-256 `dc23b6f8a95bf78eee575e58b22a36a013f53e763cfcf3472adc55d6bff21f1b`.
- Adult identity master: `_ai/art/tamagotchi/references/tamagotchi_adult_reference.png` — SHA-256 `1e0f179668056cd1a8a2e98c62bb69fbd1e5dc9bb7066786e8b516b1a25c629e`.
- Phase 3 manifest: `_ai/art/tamagotchi/references/reference-manifest.json` — SHA-256 `2589fa6d70ca9e2f3fc66075b437bab518b87b5c58de9d03f812da09338c0598`.
- Child and adult are approved RGBA masters with transparent corners. The adult is the accepted later-stage derivation of the child.

## Frozen identity bible, palette, and negative constraints

```text
Use case: identity-preserve (stylized-concept for egg and mess).
Asset type: horizontal pixel-art animation source strip for later embedded-display sprites.
Frozen identity contract: preserve the rounded warm face-patch geometry, cream forehead notch, visible rounded ear, eye spacing, blunt screen-right snout, oval cream belly, chunky quill language, limb proportions, front-three-quarter screen-right view, strong dark outline, visual center, shared ground line, and upper-left flat-step lighting. The child remains rounder and shorter; the adult is modestly taller with a longer snout/limbs and developed quill lobes.
Palette contract: all intended semantic colors are #231612, #593528, #8A7568, #8C5033, #BE7544, #E3A568, #F28C78, #FFD994, #FFF0BF, and #FFFDF4. Use crisp hard-edged limited-palette pixel art, not painterly texture.
Transparency/composition contract: genuinely transparent RGBA background; alpha-zero strip corners and clearance around subjects; exactly one horizontal row of equal cells; left-to-right playback order; all pixels inside their cell.
Negative constraints: no room, floor, cast shadow, scenery, UI, text, label, number, logo, signature, watermark, emoji, baked checkerboard, separator, gutter, border, second character, clothing, persistent accessory, realistic spines, hands/fingers, gradient, glow, blur, soft painterly edge, high-frequency dithering, antialiased halo, commercial virtual-pet/hedgehog likeness, mirroring, or identity redesign.
```

Semantic palette: outline #231612; deepQuill #593528; neutralSupport #8A7568; midQuill #8C5033; lightQuill #BE7544; warmFace #E3A568; blushAccent #F28C78; warmBelly #FFD994; creamIdentityMark #FFF0BF; eyeHighlight #FFFDF4.

## Base executable prompt

```text
Use case: identity-preserve.
Asset type: horizontal pixel-art animation source strip for later 32x32 embedded-display sprites.
Create exactly {N} equally sized frames in one horizontal row, in the stated left-to-right beat order, with no gutters or separators.
Use the attached accepted identity master as the exact target (or as style/palette-only context for egg/mess). Preserve the frozen identity, front three-quarter screen-right view, scale, visual center, ground line, upper-left lighting, dark outline, and limited palette. Change only the named pose, expression, and permitted generic cue.
Use a genuinely transparent RGBA background. One pet per cell except egg/mess. No text, UI, room, floor, shadow, scenery, logo, watermark, second character, gradient, glow, soft edge, antialiased halo, branded trait, or unrelated prop.
```

## `EGG-v1`

- Accepted source: `_ai/art/tamagotchi/generated/tamagotchi_egg_strip.png`
- Reference input(s): `_ai/art/tamagotchi/references/tamagotchi_child_reference.png` (stylePaletteOnly, `dc23b6f8a95bf78eee575e58b22a36a013f53e763cfcf3472adc55d6bff21f1b`).
- Generation artifact: `exec-5b9a8390-b0dd-4b48-83c6-854d901878a8.png`; tool/model: OpenAI built-in image-generation tool / `gpt-image 2.0`; date: 2026-09-15; output hint: not exposed.
- Complete executed prompt:

```text
Use case: stylized-concept.
Asset type: horizontal pixel-art animation source strip for later 32x32 embedded-display sprites.
Create exactly 2 equally sized logical frame(s) in one horizontal row, no gutter, separator, caption, label, border, or other panel.
Frame order: 0: intact upright egg tilted slightly left; 1: same intact egg tilted slightly right on same base/center.
Input role: use the attached accepted child desk-hog only as a style/palette reference; do not draw any pet.
Preserve the frozen face-patch geometry, cream forehead notch, visible ear, eye placement, blunt snout direction, cream belly patch, quill language, stage anatomy, screen-right front-three-quarter view, fixed center, common implied ground line, strong dark outline, and upper-left flat-step lighting. For egg/mess, preserve only style/palette; draw no pet.
Use only intended semantic palette colors #231612, #593528, #8A7568, #8C5033, #BE7544, #E3A568, #F28C78, #FFD994, #FFF0BF, #FFFDF4; crisp coarse hard-edged pixel art readable under nearest-neighbor 32x32 preview.
Use a genuinely transparent RGBA background with alpha-zero corners and transparent clearance around every cell subject. One pet at most per cell; egg/mess contain no pet; no artwork crosses an equal split boundary.
No room, floor, cast shadow, scenery, UI, words, letters, numbers, logo, signature, watermark, emoji, checkerboard, separator, colored gutter, second character, clothing, persistent accessory, hands/fingers, realistic spines, gradients, glow, blur, painterly edge, dither, antialiased halo, mirrored view, commercial virtual-pet/hedgehog trait, or unlisted prop.
```
- Acceptance: exact horizontal frame count and edge-clear split geometry passed; source corners are transparent; story/order was reviewed at source and NN target-preview scale.

## `CHILD-IDLE-v2`

- Accepted source: `_ai/art/tamagotchi/generated/tamagotchi_child_idle_strip.png`
- Reference input(s): `_ai/art/tamagotchi/references/tamagotchi_child_reference.png` (identity, `dc23b6f8a95bf78eee575e58b22a36a013f53e763cfcf3472adc55d6bff21f1b`).
- Generation artifact: `exec-82fe90ef-7240-4756-959b-549f7b4b2ff9.png`; tool/model: OpenAI built-in image-generation tool / `gpt-image 2.0`; date: 2026-09-15; output hint: not exposed.
- Complete executed prompt:

```text
Use case: identity-preserve.
Asset type: horizontal pixel-art animation source strip for later 32x32 embedded-display sprites.
Create exactly 3 equally sized logical frame(s) in one horizontal row, no gutter, separator, caption, label, border, or other panel.
Frame order: 0: canonical alert neutral; 1: very small blink/lower-body bob; 2: gentle return/rise close to frame 0.
Input role: use the attached accepted child desk-hog as the exact identity edit target.
Preserve the frozen face-patch geometry, cream forehead notch, visible ear, eye placement, blunt snout direction, cream belly patch, quill language, stage anatomy, screen-right front-three-quarter view, fixed center, common implied ground line, strong dark outline, and upper-left flat-step lighting. For egg/mess, preserve only style/palette; draw no pet.
Use only intended semantic palette colors #231612, #593528, #8A7568, #8C5033, #BE7544, #E3A568, #F28C78, #FFD994, #FFF0BF, #FFFDF4; crisp coarse hard-edged pixel art readable under nearest-neighbor 32x32 preview.
Use a genuinely transparent RGBA background with alpha-zero corners and transparent clearance around every cell subject. One pet at most per cell; egg/mess contain no pet; no artwork crosses an equal split boundary.
No room, floor, cast shadow, scenery, UI, words, letters, numbers, logo, signature, watermark, emoji, checkerboard, separator, colored gutter, second character, clothing, persistent accessory, hands/fingers, realistic spines, gradients, glow, blur, painterly edge, dither, antialiased halo, mirrored view, commercial virtual-pet/hedgehog trait, or unlisted prop.
```
- Acceptance: exact horizontal frame count and edge-clear split geometry passed; source corners are transparent; story/order was reviewed at source and NN target-preview scale.
- `CHILD-IDLE-v1`: Rejected: the first candidate enlarged/reinterpreted face and quill proportions instead of retaining the master.
- `CHILD-IDLE-v2`: accepted targeted revision; unmodified artifact is the fixed source above.

## `ADULT-IDLE-v1`

- Accepted source: `_ai/art/tamagotchi/generated/tamagotchi_adult_idle_strip.png`
- Reference input(s): `_ai/art/tamagotchi/references/tamagotchi_adult_reference.png` (identity, `1e0f179668056cd1a8a2e98c62bb69fbd1e5dc9bb7066786e8b516b1a25c629e`).
- Generation artifact: `exec-843a5282-1418-41e4-9a3b-599647a9713f.png`; tool/model: OpenAI built-in image-generation tool / `gpt-image 2.0`; date: 2026-09-15; output hint: not exposed.
- Complete executed prompt:

```text
Use case: identity-preserve.
Asset type: horizontal pixel-art animation source strip for later 32x32 embedded-display sprites.
Create exactly 3 equally sized logical frame(s) in one horizontal row, no gutter, separator, caption, label, border, or other panel.
Frame order: 0: canonical adult alert neutral; 1: subtle breath/blink; 2: gentle return close to frame 0.
Input role: use the attached accepted adult desk-hog as the exact identity edit target.
Preserve the frozen face-patch geometry, cream forehead notch, visible ear, eye placement, blunt snout direction, cream belly patch, quill language, stage anatomy, screen-right front-three-quarter view, fixed center, common implied ground line, strong dark outline, and upper-left flat-step lighting. For egg/mess, preserve only style/palette; draw no pet.
Use only intended semantic palette colors #231612, #593528, #8A7568, #8C5033, #BE7544, #E3A568, #F28C78, #FFD994, #FFF0BF, #FFFDF4; crisp coarse hard-edged pixel art readable under nearest-neighbor 32x32 preview.
Use a genuinely transparent RGBA background with alpha-zero corners and transparent clearance around every cell subject. One pet at most per cell; egg/mess contain no pet; no artwork crosses an equal split boundary.
No room, floor, cast shadow, scenery, UI, words, letters, numbers, logo, signature, watermark, emoji, checkerboard, separator, colored gutter, second character, clothing, persistent accessory, hands/fingers, realistic spines, gradients, glow, blur, painterly edge, dither, antialiased halo, mirrored view, commercial virtual-pet/hedgehog trait, or unlisted prop.
```
- Acceptance: exact horizontal frame count and edge-clear split geometry passed; source corners are transparent; story/order was reviewed at source and NN target-preview scale.

## `CHILD-FEED-v1`

- Accepted source: `_ai/art/tamagotchi/generated/tamagotchi_child_feed_strip.png`
- Reference input(s): `_ai/art/tamagotchi/references/tamagotchi_child_reference.png` (identity, `dc23b6f8a95bf78eee575e58b22a36a013f53e763cfcf3472adc55d6bff21f1b`).
- Generation artifact: `exec-d53f370b-4e58-412a-b530-74259de5c6ea.png`; tool/model: OpenAI built-in image-generation tool / `gpt-image 2.0`; date: 2026-09-15; output hint: not exposed.
- Complete executed prompt:

```text
Use case: identity-preserve.
Asset type: horizontal pixel-art animation source strip for later 32x32 embedded-display sprites.
Create exactly 3 equally sized logical frame(s) in one horizontal row, no gutter, separator, caption, label, border, or other panel.
Frame order: 0: anticipates tiny generic food morsel beside snout; 1: leans/bites with morsel contacting mouth; 2: upright satisfied chew/swallow; morsel reduced or gone.
Input role: use the attached accepted child desk-hog as the exact identity edit target.
Preserve the frozen face-patch geometry, cream forehead notch, visible ear, eye placement, blunt snout direction, cream belly patch, quill language, stage anatomy, screen-right front-three-quarter view, fixed center, common implied ground line, strong dark outline, and upper-left flat-step lighting. For egg/mess, preserve only style/palette; draw no pet.
Use only intended semantic palette colors #231612, #593528, #8A7568, #8C5033, #BE7544, #E3A568, #F28C78, #FFD994, #FFF0BF, #FFFDF4; crisp coarse hard-edged pixel art readable under nearest-neighbor 32x32 preview.
Use a genuinely transparent RGBA background with alpha-zero corners and transparent clearance around every cell subject. One pet at most per cell; egg/mess contain no pet; no artwork crosses an equal split boundary.
No room, floor, cast shadow, scenery, UI, words, letters, numbers, logo, signature, watermark, emoji, checkerboard, separator, colored gutter, second character, clothing, persistent accessory, hands/fingers, realistic spines, gradients, glow, blur, painterly edge, dither, antialiased halo, mirrored view, commercial virtual-pet/hedgehog trait, or unlisted prop.
```
- Acceptance: exact horizontal frame count and edge-clear split geometry passed; source corners are transparent; story/order was reviewed at source and NN target-preview scale.

## `ADULT-FEED-v1`

- Accepted source: `_ai/art/tamagotchi/generated/tamagotchi_adult_feed_strip.png`
- Reference input(s): `_ai/art/tamagotchi/references/tamagotchi_adult_reference.png` (identity, `1e0f179668056cd1a8a2e98c62bb69fbd1e5dc9bb7066786e8b516b1a25c629e`).
- Generation artifact: `exec-a55b5ff8-4b4e-4ff1-8164-ffffb083ce0a.png`; tool/model: OpenAI built-in image-generation tool / `gpt-image 2.0`; date: 2026-09-15; output hint: not exposed.
- Complete executed prompt:

```text
Use case: identity-preserve.
Asset type: horizontal pixel-art animation source strip for later 32x32 embedded-display sprites.
Create exactly 3 equally sized logical frame(s) in one horizontal row, no gutter, separator, caption, label, border, or other panel.
Frame order: 0: anticipates same tiny generic food morsel beside snout; 1: leans/bites with morsel contacting mouth; 2: upright satisfied chew/swallow; morsel reduced or gone.
Input role: use the attached accepted adult desk-hog as the exact identity edit target.
Preserve the frozen face-patch geometry, cream forehead notch, visible ear, eye placement, blunt snout direction, cream belly patch, quill language, stage anatomy, screen-right front-three-quarter view, fixed center, common implied ground line, strong dark outline, and upper-left flat-step lighting. For egg/mess, preserve only style/palette; draw no pet.
Use only intended semantic palette colors #231612, #593528, #8A7568, #8C5033, #BE7544, #E3A568, #F28C78, #FFD994, #FFF0BF, #FFFDF4; crisp coarse hard-edged pixel art readable under nearest-neighbor 32x32 preview.
Use a genuinely transparent RGBA background with alpha-zero corners and transparent clearance around every cell subject. One pet at most per cell; egg/mess contain no pet; no artwork crosses an equal split boundary.
No room, floor, cast shadow, scenery, UI, words, letters, numbers, logo, signature, watermark, emoji, checkerboard, separator, colored gutter, second character, clothing, persistent accessory, hands/fingers, realistic spines, gradients, glow, blur, painterly edge, dither, antialiased halo, mirrored view, commercial virtual-pet/hedgehog trait, or unlisted prop.
```
- Acceptance: exact horizontal frame count and edge-clear split geometry passed; source corners are transparent; story/order was reviewed at source and NN target-preview scale.

## `CHILD-PLAY-v1`

- Accepted source: `_ai/art/tamagotchi/generated/tamagotchi_child_play_strip.png`
- Reference input(s): `_ai/art/tamagotchi/references/tamagotchi_child_reference.png` (identity, `dc23b6f8a95bf78eee575e58b22a36a013f53e763cfcf3472adc55d6bff21f1b`).
- Generation artifact: `exec-b8c69a76-59bf-4558-8bd8-1bf5989d9315.png`; tool/model: OpenAI built-in image-generation tool / `gpt-image 2.0`; date: 2026-09-15; output hint: not exposed.
- Complete executed prompt:

```text
Use case: identity-preserve.
Asset type: horizontal pixel-art animation source strip for later 32x32 embedded-display sprites.
Create exactly 3 equally sized logical frame(s) in one horizontal row, no gutter, separator, caption, label, border, or other panel.
Frame order: 0: playful ready/lean with one tiny ball; 1: contact/hop or paw motion with ball; 2: happy recovery with ball settled nearby.
Input role: use the attached accepted child desk-hog as the exact identity edit target.
Preserve the frozen face-patch geometry, cream forehead notch, visible ear, eye placement, blunt snout direction, cream belly patch, quill language, stage anatomy, screen-right front-three-quarter view, fixed center, common implied ground line, strong dark outline, and upper-left flat-step lighting. For egg/mess, preserve only style/palette; draw no pet.
Use only intended semantic palette colors #231612, #593528, #8A7568, #8C5033, #BE7544, #E3A568, #F28C78, #FFD994, #FFF0BF, #FFFDF4; crisp coarse hard-edged pixel art readable under nearest-neighbor 32x32 preview.
Use a genuinely transparent RGBA background with alpha-zero corners and transparent clearance around every cell subject. One pet at most per cell; egg/mess contain no pet; no artwork crosses an equal split boundary.
No room, floor, cast shadow, scenery, UI, words, letters, numbers, logo, signature, watermark, emoji, checkerboard, separator, colored gutter, second character, clothing, persistent accessory, hands/fingers, realistic spines, gradients, glow, blur, painterly edge, dither, antialiased halo, mirrored view, commercial virtual-pet/hedgehog trait, or unlisted prop.
```
- Acceptance: exact horizontal frame count and edge-clear split geometry passed; source corners are transparent; story/order was reviewed at source and NN target-preview scale.

## `ADULT-PLAY-v1`

- Accepted source: `_ai/art/tamagotchi/generated/tamagotchi_adult_play_strip.png`
- Reference input(s): `_ai/art/tamagotchi/references/tamagotchi_adult_reference.png` (identity, `1e0f179668056cd1a8a2e98c62bb69fbd1e5dc9bb7066786e8b516b1a25c629e`).
- Generation artifact: `exec-fcb4d4e7-cd28-431e-81a3-b8bada5dece5.png`; tool/model: OpenAI built-in image-generation tool / `gpt-image 2.0`; date: 2026-09-15; output hint: not exposed.
- Complete executed prompt:

```text
Use case: identity-preserve.
Asset type: horizontal pixel-art animation source strip for later 32x32 embedded-display sprites.
Create exactly 3 equally sized logical frame(s) in one horizontal row, no gutter, separator, caption, label, border, or other panel.
Frame order: 0: adult ready/lean with same tiny ball; 1: contact/hop or paw motion with ball; 2: happy recovery with ball settled nearby.
Input role: use the attached accepted adult desk-hog as the exact identity edit target.
Preserve the frozen face-patch geometry, cream forehead notch, visible ear, eye placement, blunt snout direction, cream belly patch, quill language, stage anatomy, screen-right front-three-quarter view, fixed center, common implied ground line, strong dark outline, and upper-left flat-step lighting. For egg/mess, preserve only style/palette; draw no pet.
Use only intended semantic palette colors #231612, #593528, #8A7568, #8C5033, #BE7544, #E3A568, #F28C78, #FFD994, #FFF0BF, #FFFDF4; crisp coarse hard-edged pixel art readable under nearest-neighbor 32x32 preview.
Use a genuinely transparent RGBA background with alpha-zero corners and transparent clearance around every cell subject. One pet at most per cell; egg/mess contain no pet; no artwork crosses an equal split boundary.
No room, floor, cast shadow, scenery, UI, words, letters, numbers, logo, signature, watermark, emoji, checkerboard, separator, colored gutter, second character, clothing, persistent accessory, hands/fingers, realistic spines, gradients, glow, blur, painterly edge, dither, antialiased halo, mirrored view, commercial virtual-pet/hedgehog trait, or unlisted prop.
```
- Acceptance: exact horizontal frame count and edge-clear split geometry passed; source corners are transparent; story/order was reviewed at source and NN target-preview scale.

## `CHILD-CLEAN-v1`

- Accepted source: `_ai/art/tamagotchi/generated/tamagotchi_child_clean_strip.png`
- Reference input(s): `_ai/art/tamagotchi/references/tamagotchi_child_reference.png` (identity, `dc23b6f8a95bf78eee575e58b22a36a013f53e763cfcf3472adc55d6bff21f1b`).
- Generation artifact: `exec-727f548b-8185-4fc4-a621-9256ea3c1648.png`; tool/model: OpenAI built-in image-generation tool / `gpt-image 2.0`; date: 2026-09-15; output hint: not exposed.
- Complete executed prompt:

```text
Use case: identity-preserve.
Asset type: horizontal pixel-art animation source strip for later 32x32 embedded-display sprites.
Create exactly 3 equally sized logical frame(s) in one horizontal row, no gutter, separator, caption, label, border, or other panel.
Frame order: 0: one small bubble/sparkle appears; 1: two or three localized bubbles/sparkles pass body; 2: clean pleased child with final small sparkle.
Input role: use the attached accepted child desk-hog as the exact identity edit target.
Preserve the frozen face-patch geometry, cream forehead notch, visible ear, eye placement, blunt snout direction, cream belly patch, quill language, stage anatomy, screen-right front-three-quarter view, fixed center, common implied ground line, strong dark outline, and upper-left flat-step lighting. For egg/mess, preserve only style/palette; draw no pet.
Use only intended semantic palette colors #231612, #593528, #8A7568, #8C5033, #BE7544, #E3A568, #F28C78, #FFD994, #FFF0BF, #FFFDF4; crisp coarse hard-edged pixel art readable under nearest-neighbor 32x32 preview.
Use a genuinely transparent RGBA background with alpha-zero corners and transparent clearance around every cell subject. One pet at most per cell; egg/mess contain no pet; no artwork crosses an equal split boundary.
No room, floor, cast shadow, scenery, UI, words, letters, numbers, logo, signature, watermark, emoji, checkerboard, separator, colored gutter, second character, clothing, persistent accessory, hands/fingers, realistic spines, gradients, glow, blur, painterly edge, dither, antialiased halo, mirrored view, commercial virtual-pet/hedgehog trait, or unlisted prop.
```
- Acceptance: exact horizontal frame count and edge-clear split geometry passed; source corners are transparent; story/order was reviewed at source and NN target-preview scale.

## `ADULT-CLEAN-v2`

- Accepted source: `_ai/art/tamagotchi/generated/tamagotchi_adult_clean_strip.png`
- Reference input(s): `_ai/art/tamagotchi/references/tamagotchi_adult_reference.png` (identity, `1e0f179668056cd1a8a2e98c62bb69fbd1e5dc9bb7066786e8b516b1a25c629e`).
- Generation artifact: `exec-74083350-4f6b-4115-8a8b-1d4bb0d098d1.png`; tool/model: OpenAI built-in image-generation tool / `gpt-image 2.0`; date: 2026-09-15; output hint: not exposed.
- Complete executed prompt:

```text
Use case: identity-preserve.
Asset type: horizontal pixel-art animation source strip for later 32x32 embedded-display sprites.
Create exactly 3 equally sized logical frame(s) in one horizontal row, no gutter, separator, caption, label, border, or other panel.
Frame order: 0: one small bubble/sparkle appears; 1: two or three localized bubbles/sparkles pass body; 2: clean pleased adult with final small sparkle.
Input role: use the attached accepted adult desk-hog as the exact identity edit target.
Preserve the frozen face-patch geometry, cream forehead notch, visible ear, eye placement, blunt snout direction, cream belly patch, quill language, stage anatomy, screen-right front-three-quarter view, fixed center, common implied ground line, strong dark outline, and upper-left flat-step lighting. For egg/mess, preserve only style/palette; draw no pet.
Use only intended semantic palette colors #231612, #593528, #8A7568, #8C5033, #BE7544, #E3A568, #F28C78, #FFD994, #FFF0BF, #FFFDF4; crisp coarse hard-edged pixel art readable under nearest-neighbor 32x32 preview.
Use a genuinely transparent RGBA background with alpha-zero corners and transparent clearance around every cell subject. One pet at most per cell; egg/mess contain no pet; no artwork crosses an equal split boundary.
No room, floor, cast shadow, scenery, UI, words, letters, numbers, logo, signature, watermark, emoji, checkerboard, separator, colored gutter, second character, clothing, persistent accessory, hands/fingers, realistic spines, gradients, glow, blur, painterly edge, dither, antialiased halo, mirrored view, commercial virtual-pet/hedgehog trait, or unlisted prop.
```
- Acceptance: exact horizontal frame count and edge-clear split geometry passed; source corners are transparent; story/order was reviewed at source and NN target-preview scale.
- `ADULT-CLEAN-v1`: Rejected: a colored fringe formed around the localized sparkle effect; v2 retained the action while reducing that fringe.
- `ADULT-CLEAN-v2`: accepted targeted revision; unmodified artifact is the fixed source above.

## `CHILD-REST-v1`

- Accepted source: `_ai/art/tamagotchi/generated/tamagotchi_child_rest_strip.png`
- Reference input(s): `_ai/art/tamagotchi/references/tamagotchi_child_reference.png` (identity, `dc23b6f8a95bf78eee575e58b22a36a013f53e763cfcf3472adc55d6bff21f1b`).
- Generation artifact: `exec-5fb9d5ac-5fa2-4d03-bd78-82465b404a31.png`; tool/model: OpenAI built-in image-generation tool / `gpt-image 2.0`; date: 2026-09-15; output hint: not exposed.
- Complete executed prompt:

```text
Use case: identity-preserve.
Asset type: horizontal pixel-art animation source strip for later 32x32 embedded-display sprites.
Create exactly 2 equally sized logical frame(s) in one horizontal row, no gutter, separator, caption, label, border, or other panel.
Frame order: 0: child lowers onto same floor plane, eyes closing; 1: settled compact breathing pose with closed eyes.
Input role: use the attached accepted child desk-hog as the exact identity edit target.
Preserve the frozen face-patch geometry, cream forehead notch, visible ear, eye placement, blunt snout direction, cream belly patch, quill language, stage anatomy, screen-right front-three-quarter view, fixed center, common implied ground line, strong dark outline, and upper-left flat-step lighting. For egg/mess, preserve only style/palette; draw no pet.
Use only intended semantic palette colors #231612, #593528, #8A7568, #8C5033, #BE7544, #E3A568, #F28C78, #FFD994, #FFF0BF, #FFFDF4; crisp coarse hard-edged pixel art readable under nearest-neighbor 32x32 preview.
Use a genuinely transparent RGBA background with alpha-zero corners and transparent clearance around every cell subject. One pet at most per cell; egg/mess contain no pet; no artwork crosses an equal split boundary.
No room, floor, cast shadow, scenery, UI, words, letters, numbers, logo, signature, watermark, emoji, checkerboard, separator, colored gutter, second character, clothing, persistent accessory, hands/fingers, realistic spines, gradients, glow, blur, painterly edge, dither, antialiased halo, mirrored view, commercial virtual-pet/hedgehog trait, or unlisted prop.
```
- Acceptance: exact horizontal frame count and edge-clear split geometry passed; source corners are transparent; story/order was reviewed at source and NN target-preview scale.

## `ADULT-REST-v1`

- Accepted source: `_ai/art/tamagotchi/generated/tamagotchi_adult_rest_strip.png`
- Reference input(s): `_ai/art/tamagotchi/references/tamagotchi_adult_reference.png` (identity, `1e0f179668056cd1a8a2e98c62bb69fbd1e5dc9bb7066786e8b516b1a25c629e`).
- Generation artifact: `exec-04c99088-1a4b-49fb-a684-cc9ef3fd30b2.png`; tool/model: OpenAI built-in image-generation tool / `gpt-image 2.0`; date: 2026-09-15; output hint: not exposed.
- Complete executed prompt:

```text
Use case: identity-preserve.
Asset type: horizontal pixel-art animation source strip for later 32x32 embedded-display sprites.
Create exactly 2 equally sized logical frame(s) in one horizontal row, no gutter, separator, caption, label, border, or other panel.
Frame order: 0: adult lowers onto same floor plane, eyes closing; 1: settled compact breathing pose with closed eyes.
Input role: use the attached accepted adult desk-hog as the exact identity edit target.
Preserve the frozen face-patch geometry, cream forehead notch, visible ear, eye placement, blunt snout direction, cream belly patch, quill language, stage anatomy, screen-right front-three-quarter view, fixed center, common implied ground line, strong dark outline, and upper-left flat-step lighting. For egg/mess, preserve only style/palette; draw no pet.
Use only intended semantic palette colors #231612, #593528, #8A7568, #8C5033, #BE7544, #E3A568, #F28C78, #FFD994, #FFF0BF, #FFFDF4; crisp coarse hard-edged pixel art readable under nearest-neighbor 32x32 preview.
Use a genuinely transparent RGBA background with alpha-zero corners and transparent clearance around every cell subject. One pet at most per cell; egg/mess contain no pet; no artwork crosses an equal split boundary.
No room, floor, cast shadow, scenery, UI, words, letters, numbers, logo, signature, watermark, emoji, checkerboard, separator, colored gutter, second character, clothing, persistent accessory, hands/fingers, realistic spines, gradients, glow, blur, painterly edge, dither, antialiased halo, mirrored view, commercial virtual-pet/hedgehog trait, or unlisted prop.
```
- Acceptance: exact horizontal frame count and edge-clear split geometry passed; source corners are transparent; story/order was reviewed at source and NN target-preview scale.

## `CHILD-SICK-v1`

- Accepted source: `_ai/art/tamagotchi/generated/tamagotchi_child_sick_strip.png`
- Reference input(s): `_ai/art/tamagotchi/references/tamagotchi_child_reference.png` (identity, `dc23b6f8a95bf78eee575e58b22a36a013f53e763cfcf3472adc55d6bff21f1b`).
- Generation artifact: `exec-25b19e71-2a6d-4567-b8a1-fa1875c05e0c.png`; tool/model: OpenAI built-in image-generation tool / `gpt-image 2.0`; date: 2026-09-15; output hint: not exposed.
- Complete executed prompt:

```text
Use case: identity-preserve.
Asset type: horizontal pixel-art animation source strip for later 32x32 embedded-display sprites.
Create exactly 2 equally sized logical frame(s) in one horizontal row, no gutter, separator, caption, label, border, or other panel.
Frame order: 0: child visibly slumped with uncomfortable eyes/mouth; 1: tiny weak sway/breath while slumped.
Input role: use the attached accepted child desk-hog as the exact identity edit target.
Preserve the frozen face-patch geometry, cream forehead notch, visible ear, eye placement, blunt snout direction, cream belly patch, quill language, stage anatomy, screen-right front-three-quarter view, fixed center, common implied ground line, strong dark outline, and upper-left flat-step lighting. For egg/mess, preserve only style/palette; draw no pet.
Use only intended semantic palette colors #231612, #593528, #8A7568, #8C5033, #BE7544, #E3A568, #F28C78, #FFD994, #FFF0BF, #FFFDF4; crisp coarse hard-edged pixel art readable under nearest-neighbor 32x32 preview.
Use a genuinely transparent RGBA background with alpha-zero corners and transparent clearance around every cell subject. One pet at most per cell; egg/mess contain no pet; no artwork crosses an equal split boundary.
No room, floor, cast shadow, scenery, UI, words, letters, numbers, logo, signature, watermark, emoji, checkerboard, separator, colored gutter, second character, clothing, persistent accessory, hands/fingers, realistic spines, gradients, glow, blur, painterly edge, dither, antialiased halo, mirrored view, commercial virtual-pet/hedgehog trait, or unlisted prop.
```
- Acceptance: exact horizontal frame count and edge-clear split geometry passed; source corners are transparent; story/order was reviewed at source and NN target-preview scale.

## `ADULT-SICK-v1`

- Accepted source: `_ai/art/tamagotchi/generated/tamagotchi_adult_sick_strip.png`
- Reference input(s): `_ai/art/tamagotchi/references/tamagotchi_adult_reference.png` (identity, `1e0f179668056cd1a8a2e98c62bb69fbd1e5dc9bb7066786e8b516b1a25c629e`).
- Generation artifact: `exec-8d396e67-713b-434f-9e22-1205a6a9a414.png`; tool/model: OpenAI built-in image-generation tool / `gpt-image 2.0`; date: 2026-09-15; output hint: not exposed.
- Complete executed prompt:

```text
Use case: identity-preserve.
Asset type: horizontal pixel-art animation source strip for later 32x32 embedded-display sprites.
Create exactly 2 equally sized logical frame(s) in one horizontal row, no gutter, separator, caption, label, border, or other panel.
Frame order: 0: adult visibly slumped with uncomfortable eyes/mouth; 1: tiny weak sway/breath while slumped.
Input role: use the attached accepted adult desk-hog as the exact identity edit target.
Preserve the frozen face-patch geometry, cream forehead notch, visible ear, eye placement, blunt snout direction, cream belly patch, quill language, stage anatomy, screen-right front-three-quarter view, fixed center, common implied ground line, strong dark outline, and upper-left flat-step lighting. For egg/mess, preserve only style/palette; draw no pet.
Use only intended semantic palette colors #231612, #593528, #8A7568, #8C5033, #BE7544, #E3A568, #F28C78, #FFD994, #FFF0BF, #FFFDF4; crisp coarse hard-edged pixel art readable under nearest-neighbor 32x32 preview.
Use a genuinely transparent RGBA background with alpha-zero corners and transparent clearance around every cell subject. One pet at most per cell; egg/mess contain no pet; no artwork crosses an equal split boundary.
No room, floor, cast shadow, scenery, UI, words, letters, numbers, logo, signature, watermark, emoji, checkerboard, separator, colored gutter, second character, clothing, persistent accessory, hands/fingers, realistic spines, gradients, glow, blur, painterly edge, dither, antialiased halo, mirrored view, commercial virtual-pet/hedgehog trait, or unlisted prop.
```
- Acceptance: exact horizontal frame count and edge-clear split geometry passed; source corners are transparent; story/order was reviewed at source and NN target-preview scale.

## `CHILD-DOCTOR-v1`

- Accepted source: `_ai/art/tamagotchi/generated/tamagotchi_child_doctor_strip.png`
- Reference input(s): `_ai/art/tamagotchi/references/tamagotchi_child_reference.png` (identity, `dc23b6f8a95bf78eee575e58b22a36a013f53e763cfcf3472adc55d6bff21f1b`).
- Generation artifact: `exec-60d87d6c-d5c3-42a5-aa31-6ff509a85903.png`; tool/model: OpenAI built-in image-generation tool / `gpt-image 2.0`; date: 2026-09-15; output hint: not exposed.
- Complete executed prompt:

```text
Use case: identity-preserve.
Asset type: horizontal pixel-art animation source strip for later 32x32 embedded-display sprites.
Create exactly 3 equally sized logical frame(s) in one horizontal row, no gutter, separator, caption, label, border, or other panel.
Frame order: 0: sick child with tiny generic medical cue adjacent; 1: cue approaches or treatment applies non-graphically; 2: cue recedes and child rises into recovery.
Input role: use the attached accepted child desk-hog as the exact identity edit target.
Preserve the frozen face-patch geometry, cream forehead notch, visible ear, eye placement, blunt snout direction, cream belly patch, quill language, stage anatomy, screen-right front-three-quarter view, fixed center, common implied ground line, strong dark outline, and upper-left flat-step lighting. For egg/mess, preserve only style/palette; draw no pet.
Use only intended semantic palette colors #231612, #593528, #8A7568, #8C5033, #BE7544, #E3A568, #F28C78, #FFD994, #FFF0BF, #FFFDF4; crisp coarse hard-edged pixel art readable under nearest-neighbor 32x32 preview.
Use a genuinely transparent RGBA background with alpha-zero corners and transparent clearance around every cell subject. One pet at most per cell; egg/mess contain no pet; no artwork crosses an equal split boundary.
No room, floor, cast shadow, scenery, UI, words, letters, numbers, logo, signature, watermark, emoji, checkerboard, separator, colored gutter, second character, clothing, persistent accessory, hands/fingers, realistic spines, gradients, glow, blur, painterly edge, dither, antialiased halo, mirrored view, commercial virtual-pet/hedgehog trait, or unlisted prop.
```
- Acceptance: exact horizontal frame count and edge-clear split geometry passed; source corners are transparent; story/order was reviewed at source and NN target-preview scale.

## `ADULT-DOCTOR-v1`

- Accepted source: `_ai/art/tamagotchi/generated/tamagotchi_adult_doctor_strip.png`
- Reference input(s): `_ai/art/tamagotchi/references/tamagotchi_adult_reference.png` (identity, `1e0f179668056cd1a8a2e98c62bb69fbd1e5dc9bb7066786e8b516b1a25c629e`).
- Generation artifact: `exec-29a07eaf-b588-419c-b3af-e7dd843f64d7.png`; tool/model: OpenAI built-in image-generation tool / `gpt-image 2.0`; date: 2026-09-15; output hint: not exposed.
- Complete executed prompt:

```text
Use case: identity-preserve.
Asset type: horizontal pixel-art animation source strip for later 32x32 embedded-display sprites.
Create exactly 3 equally sized logical frame(s) in one horizontal row, no gutter, separator, caption, label, border, or other panel.
Frame order: 0: sick adult with tiny generic medical cue adjacent; 1: cue approaches or treatment applies non-graphically; 2: cue recedes and adult rises into recovery.
Input role: use the attached accepted adult desk-hog as the exact identity edit target.
Preserve the frozen face-patch geometry, cream forehead notch, visible ear, eye placement, blunt snout direction, cream belly patch, quill language, stage anatomy, screen-right front-three-quarter view, fixed center, common implied ground line, strong dark outline, and upper-left flat-step lighting. For egg/mess, preserve only style/palette; draw no pet.
Use only intended semantic palette colors #231612, #593528, #8A7568, #8C5033, #BE7544, #E3A568, #F28C78, #FFD994, #FFF0BF, #FFFDF4; crisp coarse hard-edged pixel art readable under nearest-neighbor 32x32 preview.
Use a genuinely transparent RGBA background with alpha-zero corners and transparent clearance around every cell subject. One pet at most per cell; egg/mess contain no pet; no artwork crosses an equal split boundary.
No room, floor, cast shadow, scenery, UI, words, letters, numbers, logo, signature, watermark, emoji, checkerboard, separator, colored gutter, second character, clothing, persistent accessory, hands/fingers, realistic spines, gradients, glow, blur, painterly edge, dither, antialiased halo, mirrored view, commercial virtual-pet/hedgehog trait, or unlisted prop.
```
- Acceptance: exact horizontal frame count and edge-clear split geometry passed; source corners are transparent; story/order was reviewed at source and NN target-preview scale.

## `EVOLVE-v1`

- Accepted source: `_ai/art/tamagotchi/generated/tamagotchi_evolve_strip.png`
- Reference input(s): `_ai/art/tamagotchi/references/tamagotchi_child_reference.png` (identity, `dc23b6f8a95bf78eee575e58b22a36a013f53e763cfcf3472adc55d6bff21f1b`), `_ai/art/tamagotchi/references/tamagotchi_adult_reference.png` (identity, `1e0f179668056cd1a8a2e98c62bb69fbd1e5dc9bb7066786e8b516b1a25c629e`).
- Generation artifact: `exec-a8ef8767-0ade-4525-b6aa-3081ce046656.png`; tool/model: OpenAI built-in image-generation tool / `gpt-image 2.0`; date: 2026-09-15; output hint: not exposed.
- Complete executed prompt:

```text
Use case: identity-preserve.
Asset type: horizontal pixel-art animation source strip for later 32x32 embedded-display sprites.
Create exactly 4 equally sized logical frame(s) in one horizontal row, no gutter, separator, caption, label, border, or other panel.
Frame order: 0: canonical child at common anchor; 1: slightly lengthening torso/snout/limbs and developing quills; 2: later intermediate clearly nearer adult; 3: canonical adult at same anchor/ground line.
Input role: use attachment 1 as the exact child endpoint and attachment 2 as the exact adult endpoint.
Preserve the frozen face-patch geometry, cream forehead notch, visible ear, eye placement, blunt snout direction, cream belly patch, quill language, stage anatomy, screen-right front-three-quarter view, fixed center, common implied ground line, strong dark outline, and upper-left flat-step lighting. For egg/mess, preserve only style/palette; draw no pet.
Use only intended semantic palette colors #231612, #593528, #8A7568, #8C5033, #BE7544, #E3A568, #F28C78, #FFD994, #FFF0BF, #FFFDF4; crisp coarse hard-edged pixel art readable under nearest-neighbor 32x32 preview.
Use a genuinely transparent RGBA background with alpha-zero corners and transparent clearance around every cell subject. One pet at most per cell; egg/mess contain no pet; no artwork crosses an equal split boundary.
No room, floor, cast shadow, scenery, UI, words, letters, numbers, logo, signature, watermark, emoji, checkerboard, separator, colored gutter, second character, clothing, persistent accessory, hands/fingers, realistic spines, gradients, glow, blur, painterly edge, dither, antialiased halo, mirrored view, commercial virtual-pet/hedgehog trait, or unlisted prop.
```
- Acceptance: exact horizontal frame count and edge-clear split geometry passed; source corners are transparent; story/order was reviewed at source and NN target-preview scale.

## `MESS-v1`

- Accepted source: `_ai/art/tamagotchi/generated/tamagotchi_mess_strip.png`
- Reference input(s): `_ai/art/tamagotchi/references/tamagotchi_child_reference.png` (stylePaletteOnly, `dc23b6f8a95bf78eee575e58b22a36a013f53e763cfcf3472adc55d6bff21f1b`).
- Generation artifact: `exec-dbff6357-2ca1-4008-9557-c9773f0bfbfa.png`; tool/model: OpenAI built-in image-generation tool / `gpt-image 2.0`; date: 2026-09-15; output hint: not exposed.
- Complete executed prompt:

```text
Use case: stylized-concept.
Asset type: one-frame 16x16-target static overlay source.
Create exactly 1 equally sized logical frame(s) in one horizontal row, no gutter, separator, caption, label, border, or other panel.
Frame order: 0: one compact non-graphic readable mess pile/spill symbol on transparency.
Input role: use the attached accepted child desk-hog only as a style/palette reference; do not draw any pet.
Preserve the frozen face-patch geometry, cream forehead notch, visible ear, eye placement, blunt snout direction, cream belly patch, quill language, stage anatomy, screen-right front-three-quarter view, fixed center, common implied ground line, strong dark outline, and upper-left flat-step lighting. For egg/mess, preserve only style/palette; draw no pet.
Use only intended semantic palette colors #231612, #593528, #8A7568, #8C5033, #BE7544, #E3A568, #F28C78, #FFD994, #FFF0BF, #FFFDF4; crisp coarse hard-edged pixel art readable under nearest-neighbor 16x16 preview.
Use a genuinely transparent RGBA background with alpha-zero corners and transparent clearance around every cell subject. One pet at most per cell; egg/mess contain no pet; no artwork crosses an equal split boundary.
No room, floor, cast shadow, scenery, UI, words, letters, numbers, logo, signature, watermark, emoji, checkerboard, separator, colored gutter, second character, clothing, persistent accessory, hands/fingers, realistic spines, gradients, glow, blur, painterly edge, dither, antialiased halo, mirrored view, commercial virtual-pet/hedgehog trait, or unlisted prop.
```
- Acceptance: exact horizontal frame count and edge-clear split geometry passed; source corners are transparent; story/order was reviewed at source and NN target-preview scale.
