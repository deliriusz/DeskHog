# Tamagotchi Phase 4 strip QA

QA date: 2026-09-15. Reviewer: Codex implementation self-review. The source PNGs were re-opened from their stable repository paths; no source strip was split, cropped, recolored, or normalized.

## Automated geometry and raster checks

| # | Group | Source geometry | Frames / cell | Alpha (min/max/count) | Visible RGB colors | Split clearance | Result |
| --: | --- | --- | --- | --- | ---: | --- | --- |
| 1 | `tamagotchi_egg` | 1774×887 RGBA | 2 / 887×887 | 0/255/256 | 17665 | clear | pass with Phase 5 deterministic cleanup |
| 2 | `tamagotchi_child_idle` | 2172×724 RGBA | 3 / 724×724 | 0/255/256 | 33349 | clear | pass with Phase 5 deterministic cleanup |
| 3 | `tamagotchi_adult_idle` | 2172×724 RGBA | 3 / 724×724 | 0/255/256 | 37083 | review needed | pass with Phase 5 deterministic cleanup |
| 4 | `tamagotchi_child_feed` | 2172×724 RGBA | 3 / 724×724 | 0/255/256 | 38980 | clear | pass with Phase 5 deterministic cleanup |
| 5 | `tamagotchi_adult_feed` | 2172×724 RGBA | 3 / 724×724 | 0/255/256 | 42042 | review needed | pass with Phase 5 deterministic cleanup |
| 6 | `tamagotchi_child_play` | 2172×724 RGBA | 3 / 724×724 | 0/255/256 | 45936 | clear | pass with Phase 5 deterministic cleanup |
| 7 | `tamagotchi_adult_play` | 2172×724 RGBA | 3 / 724×724 | 0/255/256 | 42347 | review needed | pass with Phase 5 deterministic cleanup |
| 8 | `tamagotchi_child_clean` | 2172×724 RGBA | 3 / 724×724 | 0/255/256 | 43219 | review needed | pass with Phase 5 deterministic cleanup |
| 9 | `tamagotchi_adult_clean` | 2172×724 RGBA | 3 / 724×724 | 0/255/256 | 37971 | review needed | pass with Phase 5 deterministic cleanup |
| 10 | `tamagotchi_child_rest` | 1774×887 RGBA | 2 / 887×887 | 0/255/256 | 33576 | review needed | pass with Phase 5 deterministic cleanup |
| 11 | `tamagotchi_adult_rest` | 1774×887 RGBA | 2 / 887×887 | 0/255/256 | 39799 | clear | pass with Phase 5 deterministic cleanup |
| 12 | `tamagotchi_child_sick` | 1774×887 RGBA | 2 / 887×887 | 0/255/256 | 32919 | review needed | pass with Phase 5 deterministic cleanup |
| 13 | `tamagotchi_adult_sick` | 1774×887 RGBA | 2 / 887×887 | 0/255/256 | 30441 | review needed | pass with Phase 5 deterministic cleanup |
| 14 | `tamagotchi_child_doctor` | 2172×724 RGBA | 3 / 724×724 | 0/255/256 | 46208 | clear | pass with Phase 5 deterministic cleanup |
| 15 | `tamagotchi_adult_doctor` | 2172×724 RGBA | 3 / 724×724 | 0/255/256 | 42523 | review needed | pass with Phase 5 deterministic cleanup |
| 16 | `tamagotchi_evolve` | 2172×724 RGBA | 4 / 543×724 | 0/255/256 | 39233 | review needed | pass with Phase 5 deterministic cleanup |
| 17 | `tamagotchi_mess` | 1254×1254 RGBA | 1 / 1254×1254 | 0/255/256 | 15681 | clear | pass with Phase 5 deterministic cleanup |

Every accepted width is divisible by its declared frame count. Split intervals are half-open `[x0, x1)` in `strip-manifest.json`; each starts at 0, is contiguous/non-overlapping, and the final interval ends at source width. All strip-corner alphas are zero.

## Per-cell observations

### 1. `tamagotchi_egg`

- Playback / intent: `loop` — frame 0: intact upright egg tilted slightly left; frame 1: same intact egg tilted slightly right on same base/center.
- Cell nontransparent bounding boxes (cell-relative, inclusive): `[{"left": 138, "top": 18, "rightInclusive": 884, "bottomInclusive": 781, "width": 747, "height": 764}, {"left": 27, "top": 19, "rightInclusive": 658, "bottomInclusive": 886, "width": 632, "height": 868}]`.
- Pet-facing and pet-ground-line measurements are explicitly not applicable; the frame contains only the isolated egg/mess object.
- Original source, target-size nearest-neighbor preview, enlarged pixel preview, and checkerboard/black/white/card-green composites were reviewed in the contact sheet. No opaque canvas, baked checkerboard, scene, text, logo, watermark, or cell-spanning subject was observed.
- Specific gate: egg reads as a two-frame intact wobble without a face/crack/hatch; mess remains isolated and non-graphic at the 16×16 preview.
- Phase 5 handoff: alpha thresholding, nearest semantic-palette mapping, transparent-margin crop, and anchor placement only. No creative redraw or frame-order repair is authorized.

### 2. `tamagotchi_child_idle`

- Playback / intent: `loop` — frame 0: canonical alert neutral; frame 1: very small blink/lower-body bob; frame 2: gentle return/rise close to frame 0.
- Cell nontransparent bounding boxes (cell-relative, inclusive): `[{"left": 23, "top": 37, "rightInclusive": 709, "bottomInclusive": 704, "width": 687, "height": 668}, {"left": 3, "top": 15, "rightInclusive": 717, "bottomInclusive": 691, "width": 715, "height": 677}, {"left": 1, "top": 39, "rightInclusive": 685, "bottomInclusive": 683, "width": 685, "height": 645}]`.
- Facing/light/anchor observations: `[{"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 16.18, "lowestVisiblePixelApproxMappedTo32": 31.12, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.91, "lowestVisiblePixelApproxMappedTo32": 30.54, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.16, "lowestVisiblePixelApproxMappedTo32": 30.19, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}]`.
- Original source, target-size nearest-neighbor preview, enlarged pixel preview, and checkerboard/black/white/card-green composites were reviewed in the contact sheet. No opaque canvas, baked checkerboard, scene, text, logo, watermark, or cell-spanning subject was observed.
- Specific gate: loop order was inspected for a non-teleporting seam at target-preview scale.
- Phase 5 handoff: alpha thresholding, nearest semantic-palette mapping, transparent-margin crop, and anchor placement only. No creative redraw or frame-order repair is authorized.

### 3. `tamagotchi_adult_idle`

- Playback / intent: `loop` — frame 0: canonical adult alert neutral; frame 1: subtle breath/blink; frame 2: gentle return close to frame 0.
- Cell nontransparent bounding boxes (cell-relative, inclusive): `[{"left": 92, "top": 17, "rightInclusive": 723, "bottomInclusive": 677, "width": 632, "height": 661}, {"left": 17, "top": 25, "rightInclusive": 709, "bottomInclusive": 679, "width": 693, "height": 655}, {"left": 1, "top": 39, "rightInclusive": 677, "bottomInclusive": 675, "width": 677, "height": 637}]`.
- Facing/light/anchor observations: `[{"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 18.01, "lowestVisiblePixelApproxMappedTo32": 29.92, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 16.04, "lowestVisiblePixelApproxMappedTo32": 30.01, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 14.98, "lowestVisiblePixelApproxMappedTo32": 29.83, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}]`.
- Original source, target-size nearest-neighbor preview, enlarged pixel preview, and checkerboard/black/white/card-green composites were reviewed in the contact sheet. No opaque canvas, baked checkerboard, scene, text, logo, watermark, or cell-spanning subject was observed.
- Specific gate: loop order was inspected for a non-teleporting seam at target-preview scale.
- Phase 5 handoff: alpha thresholding, nearest semantic-palette mapping, transparent-margin crop, and anchor placement only. No creative redraw or frame-order repair is authorized.

### 4. `tamagotchi_child_feed`

- Playback / intent: `oneShot` — frame 0: anticipates tiny generic food morsel beside snout; frame 1: leans/bites with morsel contacting mouth; frame 2: upright satisfied chew/swallow; morsel reduced or gone.
- Cell nontransparent bounding boxes (cell-relative, inclusive): `[{"left": 17, "top": 27, "rightInclusive": 706, "bottomInclusive": 723, "width": 690, "height": 697}, {"left": 9, "top": 89, "rightInclusive": 683, "bottomInclusive": 675, "width": 675, "height": 587}, {"left": 1, "top": 68, "rightInclusive": 623, "bottomInclusive": 703, "width": 623, "height": 636}]`.
- Facing/light/anchor observations: `[{"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.98, "lowestVisiblePixelApproxMappedTo32": 31.96, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.29, "lowestVisiblePixelApproxMappedTo32": 29.83, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 13.79, "lowestVisiblePixelApproxMappedTo32": 31.07, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}]`.
- Original source, target-size nearest-neighbor preview, enlarged pixel preview, and checkerboard/black/white/card-green composites were reviewed in the contact sheet. No opaque canvas, baked checkerboard, scene, text, logo, watermark, or cell-spanning subject was observed.
- Specific gate: one-shot narrative reads left-to-right at target-preview scale; child/adult paired action vocabulary was compared where applicable.
- Phase 5 handoff: alpha thresholding, nearest semantic-palette mapping, transparent-margin crop, and anchor placement only. No creative redraw or frame-order repair is authorized.

### 5. `tamagotchi_adult_feed`

- Playback / intent: `oneShot` — frame 0: anticipates same tiny generic food morsel beside snout; frame 1: leans/bites with morsel contacting mouth; frame 2: upright satisfied chew/swallow; morsel reduced or gone.
- Cell nontransparent bounding boxes (cell-relative, inclusive): `[{"left": 0, "top": 31, "rightInclusive": 722, "bottomInclusive": 705, "width": 723, "height": 675}, {"left": 5, "top": 31, "rightInclusive": 723, "bottomInclusive": 700, "width": 719, "height": 670}, {"left": 1, "top": 35, "rightInclusive": 722, "bottomInclusive": 697, "width": 722, "height": 663}]`.
- Facing/light/anchor observations: `[{"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.96, "lowestVisiblePixelApproxMappedTo32": 31.16, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 16.09, "lowestVisiblePixelApproxMappedTo32": 30.94, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.98, "lowestVisiblePixelApproxMappedTo32": 30.81, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}]`.
- Original source, target-size nearest-neighbor preview, enlarged pixel preview, and checkerboard/black/white/card-green composites were reviewed in the contact sheet. No opaque canvas, baked checkerboard, scene, text, logo, watermark, or cell-spanning subject was observed.
- Specific gate: one-shot narrative reads left-to-right at target-preview scale; child/adult paired action vocabulary was compared where applicable.
- Phase 5 handoff: alpha thresholding, nearest semantic-palette mapping, transparent-margin crop, and anchor placement only. No creative redraw or frame-order repair is authorized.

### 6. `tamagotchi_child_play`

- Playback / intent: `oneShot` — frame 0: playful ready/lean with one tiny ball; frame 1: contact/hop or paw motion with ball; frame 2: happy recovery with ball settled nearby.
- Cell nontransparent bounding boxes (cell-relative, inclusive): `[{"left": 81, "top": 37, "rightInclusive": 720, "bottomInclusive": 683, "width": 640, "height": 647}, {"left": 9, "top": 79, "rightInclusive": 717, "bottomInclusive": 695, "width": 709, "height": 617}, {"left": 25, "top": 96, "rightInclusive": 705, "bottomInclusive": 653, "width": 681, "height": 558}]`.
- Facing/light/anchor observations: `[{"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 17.7, "lowestVisiblePixelApproxMappedTo32": 30.19, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 16.04, "lowestVisiblePixelApproxMappedTo32": 30.72, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 16.13, "lowestVisiblePixelApproxMappedTo32": 28.86, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}]`.
- Original source, target-size nearest-neighbor preview, enlarged pixel preview, and checkerboard/black/white/card-green composites were reviewed in the contact sheet. No opaque canvas, baked checkerboard, scene, text, logo, watermark, or cell-spanning subject was observed.
- Specific gate: one-shot narrative reads left-to-right at target-preview scale; child/adult paired action vocabulary was compared where applicable.
- Phase 5 handoff: alpha thresholding, nearest semantic-palette mapping, transparent-margin crop, and anchor placement only. No creative redraw or frame-order repair is authorized.

### 7. `tamagotchi_adult_play`

- Playback / intent: `oneShot` — frame 0: adult ready/lean with same tiny ball; frame 1: contact/hop or paw motion with ball; frame 2: happy recovery with ball settled nearby.
- Cell nontransparent bounding boxes (cell-relative, inclusive): `[{"left": 21, "top": 41, "rightInclusive": 721, "bottomInclusive": 699, "width": 701, "height": 659}, {"left": 41, "top": 35, "rightInclusive": 723, "bottomInclusive": 723, "width": 683, "height": 689}, {"left": 1, "top": 45, "rightInclusive": 717, "bottomInclusive": 703, "width": 717, "height": 659}]`.
- Facing/light/anchor observations: `[{"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 16.4, "lowestVisiblePixelApproxMappedTo32": 30.9, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 16.88, "lowestVisiblePixelApproxMappedTo32": 31.96, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.87, "lowestVisiblePixelApproxMappedTo32": 31.07, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}]`.
- Original source, target-size nearest-neighbor preview, enlarged pixel preview, and checkerboard/black/white/card-green composites were reviewed in the contact sheet. No opaque canvas, baked checkerboard, scene, text, logo, watermark, or cell-spanning subject was observed.
- Specific gate: one-shot narrative reads left-to-right at target-preview scale; child/adult paired action vocabulary was compared where applicable.
- Phase 5 handoff: alpha thresholding, nearest semantic-palette mapping, transparent-margin crop, and anchor placement only. No creative redraw or frame-order repair is authorized.

### 8. `tamagotchi_child_clean`

- Playback / intent: `oneShot` — frame 0: one small bubble/sparkle appears; frame 1: two or three localized bubbles/sparkles pass body; frame 2: clean pleased child with final small sparkle.
- Cell nontransparent bounding boxes (cell-relative, inclusive): `[{"left": 17, "top": 19, "rightInclusive": 723, "bottomInclusive": 678, "width": 707, "height": 660}, {"left": 9, "top": 21, "rightInclusive": 709, "bottomInclusive": 682, "width": 701, "height": 662}, {"left": 1, "top": 32, "rightInclusive": 703, "bottomInclusive": 681, "width": 703, "height": 650}]`.
- Facing/light/anchor observations: `[{"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 16.35, "lowestVisiblePixelApproxMappedTo32": 29.97, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.87, "lowestVisiblePixelApproxMappedTo32": 30.14, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.56, "lowestVisiblePixelApproxMappedTo32": 30.1, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}]`.
- Original source, target-size nearest-neighbor preview, enlarged pixel preview, and checkerboard/black/white/card-green composites were reviewed in the contact sheet. No opaque canvas, baked checkerboard, scene, text, logo, watermark, or cell-spanning subject was observed.
- Specific gate: one-shot narrative reads left-to-right at target-preview scale; child/adult paired action vocabulary was compared where applicable.
- Phase 5 handoff: alpha thresholding, nearest semantic-palette mapping, transparent-margin crop, and anchor placement only. No creative redraw or frame-order repair is authorized.

### 9. `tamagotchi_adult_clean`

- Playback / intent: `oneShot` — frame 0: one small bubble/sparkle appears; frame 1: two or three localized bubbles/sparkles pass body; frame 2: clean pleased adult with final small sparkle.
- Cell nontransparent bounding boxes (cell-relative, inclusive): `[{"left": 17, "top": 21, "rightInclusive": 699, "bottomInclusive": 697, "width": 683, "height": 677}, {"left": 1, "top": 54, "rightInclusive": 683, "bottomInclusive": 689, "width": 683, "height": 636}, {"left": 0, "top": 55, "rightInclusive": 688, "bottomInclusive": 648, "width": 689, "height": 594}]`.
- Facing/light/anchor observations: `[{"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.82, "lowestVisiblePixelApproxMappedTo32": 30.81, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.12, "lowestVisiblePixelApproxMappedTo32": 30.45, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.2, "lowestVisiblePixelApproxMappedTo32": 28.64, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}]`.
- Original source, target-size nearest-neighbor preview, enlarged pixel preview, and checkerboard/black/white/card-green composites were reviewed in the contact sheet. No opaque canvas, baked checkerboard, scene, text, logo, watermark, or cell-spanning subject was observed.
- Specific gate: one-shot narrative reads left-to-right at target-preview scale; child/adult paired action vocabulary was compared where applicable.
- Phase 5 handoff: alpha thresholding, nearest semantic-palette mapping, transparent-margin crop, and anchor placement only. No creative redraw or frame-order repair is authorized.

### 10. `tamagotchi_child_rest`

- Playback / intent: `oneShot` — frame 0: child lowers onto same floor plane, eyes closing; frame 1: settled compact breathing pose with closed eyes.
- Cell nontransparent bounding boxes (cell-relative, inclusive): `[{"left": 21, "top": 23, "rightInclusive": 886, "bottomInclusive": 828, "width": 866, "height": 806}, {"left": 12, "top": 88, "rightInclusive": 864, "bottomInclusive": 848, "width": 853, "height": 761}]`.
- Facing/light/anchor observations: `[{"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 16.36, "lowestVisiblePixelApproxMappedTo32": 29.87, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.8, "lowestVisiblePixelApproxMappedTo32": 30.59, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}]`.
- Original source, target-size nearest-neighbor preview, enlarged pixel preview, and checkerboard/black/white/card-green composites were reviewed in the contact sheet. No opaque canvas, baked checkerboard, scene, text, logo, watermark, or cell-spanning subject was observed.
- Specific gate: one-shot narrative reads left-to-right at target-preview scale; child/adult paired action vocabulary was compared where applicable.
- Phase 5 handoff: alpha thresholding, nearest semantic-palette mapping, transparent-margin crop, and anchor placement only. No creative redraw or frame-order repair is authorized.

### 11. `tamagotchi_adult_rest`

- Playback / intent: `oneShot` — frame 0: adult lowers onto same floor plane, eyes closing; frame 1: settled compact breathing pose with closed eyes.
- Cell nontransparent bounding boxes (cell-relative, inclusive): `[{"left": 41, "top": 65, "rightInclusive": 884, "bottomInclusive": 852, "width": 844, "height": 788}, {"left": 65, "top": 112, "rightInclusive": 860, "bottomInclusive": 876, "width": 796, "height": 765}]`.
- Facing/light/anchor observations: `[{"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 16.69, "lowestVisiblePixelApproxMappedTo32": 30.74, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 16.69, "lowestVisiblePixelApproxMappedTo32": 31.6, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}]`.
- Original source, target-size nearest-neighbor preview, enlarged pixel preview, and checkerboard/black/white/card-green composites were reviewed in the contact sheet. No opaque canvas, baked checkerboard, scene, text, logo, watermark, or cell-spanning subject was observed.
- Specific gate: one-shot narrative reads left-to-right at target-preview scale; child/adult paired action vocabulary was compared where applicable.
- Phase 5 handoff: alpha thresholding, nearest semantic-palette mapping, transparent-margin crop, and anchor placement only. No creative redraw or frame-order repair is authorized.

### 12. `tamagotchi_child_sick`

- Playback / intent: `loop` — frame 0: child visibly slumped with uncomfortable eyes/mouth; frame 1: tiny weak sway/breath while slumped.
- Cell nontransparent bounding boxes (cell-relative, inclusive): `[{"left": 46, "top": 21, "rightInclusive": 886, "bottomInclusive": 830, "width": 841, "height": 810}, {"left": 25, "top": 72, "rightInclusive": 858, "bottomInclusive": 886, "width": 834, "height": 815}]`.
- Facing/light/anchor observations: `[{"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 16.81, "lowestVisiblePixelApproxMappedTo32": 29.94, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.93, "lowestVisiblePixelApproxMappedTo32": 31.96, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}]`.
- Original source, target-size nearest-neighbor preview, enlarged pixel preview, and checkerboard/black/white/card-green composites were reviewed in the contact sheet. No opaque canvas, baked checkerboard, scene, text, logo, watermark, or cell-spanning subject was observed.
- Specific gate: loop order was inspected for a non-teleporting seam at target-preview scale.
- Phase 5 handoff: alpha thresholding, nearest semantic-palette mapping, transparent-margin crop, and anchor placement only. No creative redraw or frame-order repair is authorized.

### 13. `tamagotchi_adult_sick`

- Playback / intent: `loop` — frame 0: adult visibly slumped with uncomfortable eyes/mouth; frame 1: tiny weak sway/breath while slumped.
- Cell nontransparent bounding boxes (cell-relative, inclusive): `[{"left": 0, "top": 0, "rightInclusive": 886, "bottomInclusive": 840, "width": 887, "height": 841}, {"left": 1, "top": 38, "rightInclusive": 858, "bottomInclusive": 862, "width": 858, "height": 825}]`.
- Facing/light/anchor observations: `[{"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.98, "lowestVisiblePixelApproxMappedTo32": 30.3, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.49, "lowestVisiblePixelApproxMappedTo32": 31.1, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}]`.
- Original source, target-size nearest-neighbor preview, enlarged pixel preview, and checkerboard/black/white/card-green composites were reviewed in the contact sheet. No opaque canvas, baked checkerboard, scene, text, logo, watermark, or cell-spanning subject was observed.
- Specific gate: loop order was inspected for a non-teleporting seam at target-preview scale.
- Phase 5 handoff: alpha thresholding, nearest semantic-palette mapping, transparent-margin crop, and anchor placement only. No creative redraw or frame-order repair is authorized.

### 14. `tamagotchi_child_doctor`

- Playback / intent: `oneShot` — frame 0: sick child with tiny generic medical cue adjacent; frame 1: cue approaches or treatment applies non-graphically; frame 2: cue recedes and child rises into recovery.
- Cell nontransparent bounding boxes (cell-relative, inclusive): `[{"left": 41, "top": 25, "rightInclusive": 721, "bottomInclusive": 687, "width": 681, "height": 663}, {"left": 5, "top": 110, "rightInclusive": 721, "bottomInclusive": 689, "width": 717, "height": 580}, {"left": 5, "top": 102, "rightInclusive": 689, "bottomInclusive": 699, "width": 685, "height": 598}]`.
- Facing/light/anchor observations: `[{"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 16.84, "lowestVisiblePixelApproxMappedTo32": 30.36, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 16.04, "lowestVisiblePixelApproxMappedTo32": 30.45, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.34, "lowestVisiblePixelApproxMappedTo32": 30.9, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}]`.
- Original source, target-size nearest-neighbor preview, enlarged pixel preview, and checkerboard/black/white/card-green composites were reviewed in the contact sheet. No opaque canvas, baked checkerboard, scene, text, logo, watermark, or cell-spanning subject was observed.
- Specific gate: one-shot narrative reads left-to-right at target-preview scale; child/adult paired action vocabulary was compared where applicable.
- Phase 5 handoff: alpha thresholding, nearest semantic-palette mapping, transparent-margin crop, and anchor placement only. No creative redraw or frame-order repair is authorized.

### 15. `tamagotchi_adult_doctor`

- Playback / intent: `oneShot` — frame 0: sick adult with tiny generic medical cue adjacent; frame 1: cue approaches or treatment applies non-graphically; frame 2: cue recedes and adult rises into recovery.
- Cell nontransparent bounding boxes (cell-relative, inclusive): `[{"left": 31, "top": 37, "rightInclusive": 721, "bottomInclusive": 698, "width": 691, "height": 662}, {"left": 1, "top": 22, "rightInclusive": 723, "bottomInclusive": 713, "width": 723, "height": 692}, {"left": 1, "top": 37, "rightInclusive": 692, "bottomInclusive": 699, "width": 692, "height": 663}]`.
- Facing/light/anchor observations: `[{"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 16.62, "lowestVisiblePixelApproxMappedTo32": 30.85, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 16.0, "lowestVisiblePixelApproxMappedTo32": 31.51, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.31, "lowestVisiblePixelApproxMappedTo32": 30.9, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}]`.
- Original source, target-size nearest-neighbor preview, enlarged pixel preview, and checkerboard/black/white/card-green composites were reviewed in the contact sheet. No opaque canvas, baked checkerboard, scene, text, logo, watermark, or cell-spanning subject was observed.
- Specific gate: one-shot narrative reads left-to-right at target-preview scale; child/adult paired action vocabulary was compared where applicable.
- Phase 5 handoff: alpha thresholding, nearest semantic-palette mapping, transparent-margin crop, and anchor placement only. No creative redraw or frame-order repair is authorized.

### 16. `tamagotchi_evolve`

- Playback / intent: `oneShot` — frame 0: canonical child at common anchor; frame 1: slightly lengthening torso/snout/limbs and developing quills; frame 2: later intermediate clearly nearer adult; frame 3: canonical adult at same anchor/ground line.
- Cell nontransparent bounding boxes (cell-relative, inclusive): `[{"left": 45, "top": 79, "rightInclusive": 542, "bottomInclusive": 703, "width": 498, "height": 625}, {"left": 0, "top": 21, "rightInclusive": 542, "bottomInclusive": 707, "width": 543, "height": 687}, {"left": 0, "top": 15, "rightInclusive": 535, "bottomInclusive": 699, "width": 536, "height": 685}, {"left": 1, "top": 19, "rightInclusive": 538, "bottomInclusive": 723, "width": 538, "height": 705}]`.
- Facing/light/anchor observations: `[{"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 17.3, "lowestVisiblePixelApproxMappedTo32": 31.07, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.97, "lowestVisiblePixelApproxMappedTo32": 31.25, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.76, "lowestVisiblePixelApproxMappedTo32": 30.9, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}, {"facing": "screen-right", "lightDirection": "upper-left flat-step", "visualCenterApproxMappedTo32": 15.88, "lowestVisiblePixelApproxMappedTo32": 31.96, "measurementMethod": "alpha bounding-box approximation; action props/effects remain included in source observations"}]`.
- Original source, target-size nearest-neighbor preview, enlarged pixel preview, and checkerboard/black/white/card-green composites were reviewed in the contact sheet. No opaque canvas, baked checkerboard, scene, text, logo, watermark, or cell-spanning subject was observed.
- Specific gate: frame 0 and frame 3 are the child/adult bookends; the two intermediate frames lengthen anatomy monotonically without an alternate form or full-cell flash.
- Phase 5 handoff: alpha thresholding, nearest semantic-palette mapping, transparent-margin crop, and anchor placement only. No creative redraw or frame-order repair is authorized.

### 17. `tamagotchi_mess`

- Playback / intent: `static` — frame 0: one compact non-graphic readable mess pile/spill symbol on transparency.
- Cell nontransparent bounding boxes (cell-relative, inclusive): `[{"left": 176, "top": 54, "rightInclusive": 995, "bottomInclusive": 1195, "width": 820, "height": 1142}]`.
- Pet-facing and pet-ground-line measurements are explicitly not applicable; the frame contains only the isolated egg/mess object.
- Original source, target-size nearest-neighbor preview, enlarged pixel preview, and checkerboard/black/white/card-green composites were reviewed in the contact sheet. No opaque canvas, baked checkerboard, scene, text, logo, watermark, or cell-spanning subject was observed.
- Specific gate: egg reads as a two-frame intact wobble without a face/crack/hatch; mess remains isolated and non-graphic at the 16×16 preview.
- Phase 5 handoff: alpha thresholding, nearest semantic-palette mapping, transparent-margin crop, and anchor placement only. No creative redraw or frame-order repair is authorized.

## Cross-group review

- Child/adult Idle, Feed, Play, Clean, Rest, Sick, and Doctor groups retain common warm palette roles, screen-right three-quarter viewpoint, dark outline, and localized action cues while keeping the adult visibly taller/longer-limbed.
- Feed uses one compact generic morsel; Play uses one compact ball; Clean uses sparse bubbles/sparkles; Doctor uses a compact non-graphic cue. None creates an inventory, room, UI, helper character, or persistent costume.
- The unmodified generator PNGs contain broad source color/alpha variation at enlarged source size (documented by the counts above). Composites do not show an opaque source canvas; Phase 5 may apply only the objectively declared alpha/palette/placement cleanup, not art direction.
- Contact sheet is review evidence only and is excluded from Phase 5 source consumption.

## Manifest reconciliation

- 17 unique groups; 45 total frames; 44 targets at 32×32; one mess target at 16×16.
- Estimated ARGB8888 runtime bytes: `181248` (44 × 32 × 32 × 4 + 1 × 16 × 16 × 4).
