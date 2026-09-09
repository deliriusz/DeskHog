# Tamagotchi implementation phase 5: normalize and place final PNG frames

## Phase outcome

Convert the 17 accepted horizontal strips from phase 4 into the complete, deterministic runtime sprite tree consumed by `png2c.py`: exactly 44 RGBA pet frames at 32x32 and one RGBA mess frame at 16x16. Every output must have binary alpha, an approved limited palette, stable character scale, a common visual center and ground line, globally unique names, and a reproducible machine-readable inventory.

This phase implements asset-processing tooling and produces final PNG inputs. It does not generate or redesign artwork and does not run the PNG-to-C generation owned by phase 6.

## Scope

Phase 5 includes:

- adding `tools/process_tamagotchi_sprites.py` using Pillow;
- consuming the accepted phase-4 strip manifest without changing the approved source strips;
- recording every deterministic cleanup, crop, scale, anchor, and palette decision in a processing manifest;
- splitting strips only at declared equal-width boundaries;
- performing only explicitly approved background cleanup;
- reducing alpha to 0 or 255 and eliminating transparent-edge RGB residue;
- mapping visible pixels to the approved palette when explicitly requested, without dithering;
- resizing with nearest-neighbor sampling only;
- placing each pet at a shared center and ground line on a 32x32 RGBA canvas;
- placing the mess overlay on a 16x16 RGBA canvas;
- transactionally replacing only the 17 phase-owned Tamagotchi output directories;
- validating names, sizes, alpha, palette, bounds, counts, ordering, and byte cost;
- emitting a stable runtime inventory for phase 6;
- proving a clean second run makes no changes.

## Non-goals

Do not:

- invoke image generation or create new poses;
- repair identity drift, missing limbs, clipped artwork, unreadable actions, or a bad evolution transition;
- infer frame separators, gutters, checkerboards, matte colors, subject masks, or anatomical anchors;
- smooth, interpolate, redraw, rotate, mirror, or use AI upscaling;
- use bilinear, bicubic, Lanczos, antialiasing, or adaptive per-frame scaling;
- put source strips, reference masters, previews, or contact sheets under `raw-png/`;
- modify the existing `raw-png/walking/` frames;
- run `png2c.py`, edit `include/sprites/`, or validate compiled LVGL descriptors; those are phase-6 responsibilities;
- change firmware, LVGL, model, card, or build code.

If normalization would require artistic judgment, return the affected strip to phase 4. Phase 5 may normalize pixels and placement; it must not invent artwork.

## Repository constraints that drive the design

`png2c.py` recursively sorts `raw-png/**/*.png`, groups frames by the first directory below `raw-png/`, and writes every per-frame `.c` and `.h` file into the one flat `include/sprites/` directory. Its C identifier is derived only from the basename by replacing `-` and `.` with `_`; it does not include the source directory and does not protect against other invalid characters or collisions.

Therefore:

- each animation gets one first-level, underscore-only directory;
- each PNG basename is globally unique across all of `raw-png/`, not merely within its group;
- filenames use zero-padded two-digit indices so lexicographic order is animation order;
- all names must match `^[a-z][a-z0-9_]*$` before the `.png` extension;
- no two basenames may become equal after `png2c.py`'s `-`/`.` to `_` normalization;
- only runtime-ready PNGs may live under `raw-png/` because PlatformIO runs `png2c.py` before every build and compiles the generated C;
- phase 5 must not treat a successful later build as proof of conversion, because `png2c.py` exits successfully without regeneration when Pillow or NumPy is unavailable.

The existing walking sources are 80x80, 8-bit RGBA PNGs with alpha values limited to 0 and 255. `png2c.py` converts their pixels to BGRA bytes and declares `LV_COLOR_FORMAT_ARGB8888`. Phase-5 outputs intentionally preserve the RGBA PNG/binary-alpha convention while reducing runtime pet canvases to 32x32.

## Prerequisites and input gate

### Required phase-3 inputs

The following approved identity artifacts must exist and their recorded SHA-256 values must still match:

```text
_ai/art/tamagotchi/references/tamagotchi_child_reference.png
_ai/art/tamagotchi/references/tamagotchi_adult_reference.png
_ai/art/tamagotchi/references/reference-manifest.json
_ai/art/tamagotchi/prompts.md
```

Use the reference manifest's approved palette, identity marks, facing direction, target center, and target ground line as review constraints. A mismatch does not authorize phase 5 to redraw a frame.

### Required phase-4 inputs

Phase 4 hands off:

```text
_ai/art/tamagotchi/generated/strip-manifest.json
_ai/art/tamagotchi/generated/<group>_strip.png
_ai/art/tamagotchi/generated/animation-prompts.md
_ai/art/tamagotchi/generated/strip-qa.md
_ai/art/tamagotchi/generated/tamagotchi-animation-contact-sheet.png
```

`strip-manifest.json` schema version 1 must contain all 17 groups and, for each group, the stable group name, source path and SHA-256, source byte size, width and height, PNG color mode and alpha facts, horizontal layout, exact frame count, equal frame width, frame height, explicit `[x0, x1)` split boundaries, target runtime size, identity references and hashes, ordered frame intent, palette observations, facing and lighting, observed source center/ground line, loop mode, acceptance record, and any `requiredPhase5Corrections`.

Every accepted strip must be one horizontal row with no label, gutter, separator, or extra cell. Its width must equal `frameCount * frameWidth`; its height must equal `frameHeight`; and its declared split boundaries must cover `[0, width)` exactly once without gaps or overlap. Any violation returns to phase 4 rather than triggering inferred splitting.

### Start gate

Before writing outputs:

1. Preserve unrelated worktree changes and record the pre-phase status.
2. Confirm Pillow is importable in the PlatformIO Python environment.
3. Parse both phase-3 and phase-4 manifests and verify every referenced file hash.
4. Confirm the phase-4 group set and counts match the fixed inventory below.
5. Confirm every strip has a real alpha channel and at least one transparent and one visible pixel in every frame.
6. Reject an opaque rectangle, baked checkerboard, source clipping, or unapproved identity/palette drift.

No `raw-png/` change is allowed until all source checks pass.

## Fixed runtime inventory

| Group / output directory | Count | Canvas | Output basenames |
| --- | ---: | ---: | --- |
| `tamagotchi_egg` | 2 | 32x32 | `tamagotchi_egg_00.png` ... `_01.png` |
| `tamagotchi_child_idle` | 3 | 32x32 | `tamagotchi_child_idle_00.png` ... `_02.png` |
| `tamagotchi_adult_idle` | 3 | 32x32 | `tamagotchi_adult_idle_00.png` ... `_02.png` |
| `tamagotchi_child_feed` | 3 | 32x32 | `tamagotchi_child_feed_00.png` ... `_02.png` |
| `tamagotchi_adult_feed` | 3 | 32x32 | `tamagotchi_adult_feed_00.png` ... `_02.png` |
| `tamagotchi_child_play` | 3 | 32x32 | `tamagotchi_child_play_00.png` ... `_02.png` |
| `tamagotchi_adult_play` | 3 | 32x32 | `tamagotchi_adult_play_00.png` ... `_02.png` |
| `tamagotchi_child_clean` | 3 | 32x32 | `tamagotchi_child_clean_00.png` ... `_02.png` |
| `tamagotchi_adult_clean` | 3 | 32x32 | `tamagotchi_adult_clean_00.png` ... `_02.png` |
| `tamagotchi_child_rest` | 2 | 32x32 | `tamagotchi_child_rest_00.png` ... `_01.png` |
| `tamagotchi_adult_rest` | 2 | 32x32 | `tamagotchi_adult_rest_00.png` ... `_01.png` |
| `tamagotchi_child_sick` | 2 | 32x32 | `tamagotchi_child_sick_00.png` ... `_01.png` |
| `tamagotchi_adult_sick` | 2 | 32x32 | `tamagotchi_adult_sick_00.png` ... `_01.png` |
| `tamagotchi_child_doctor` | 3 | 32x32 | `tamagotchi_child_doctor_00.png` ... `_02.png` |
| `tamagotchi_adult_doctor` | 3 | 32x32 | `tamagotchi_adult_doctor_00.png` ... `_02.png` |
| `tamagotchi_evolve` | 4 | 32x32 | `tamagotchi_evolve_00.png` ... `_03.png` |
| `tamagotchi_mess` | 1 | 16x16 | `tamagotchi_mess_00.png` |

The totals are non-negotiable for this phase: 17 groups, 45 files, 44 32x32 pet frames, and one 16x16 mess frame. The 32x32 count is 2 egg + 6 idle + 6 feed + 6 play + 6 clean + 4 rest + 4 sick + 6 doctor + 4 evolve = 44.

## Files added or owned by this phase

```text
tools/process_tamagotchi_sprites.py
_ai/art/tamagotchi/sprite-processing-manifest.json
_ai/art/tamagotchi/runtime-sprite-manifest.json
raw-png/tamagotchi_egg/*.png
raw-png/tamagotchi_child_idle/*.png
raw-png/tamagotchi_adult_idle/*.png
raw-png/tamagotchi_child_feed/*.png
raw-png/tamagotchi_adult_feed/*.png
raw-png/tamagotchi_child_play/*.png
raw-png/tamagotchi_adult_play/*.png
raw-png/tamagotchi_child_clean/*.png
raw-png/tamagotchi_adult_clean/*.png
raw-png/tamagotchi_child_rest/*.png
raw-png/tamagotchi_adult_rest/*.png
raw-png/tamagotchi_child_sick/*.png
raw-png/tamagotchi_adult_sick/*.png
raw-png/tamagotchi_child_doctor/*.png
raw-png/tamagotchi_adult_doctor/*.png
raw-png/tamagotchi_evolve/*.png
raw-png/tamagotchi_mess/*.png
```

The processing manifest is reviewed configuration. The runtime sprite manifest is deterministic generated evidence. It must not contain a generation timestamp or absolute path, because either would make clean reruns differ.

## Tool CLI contract

Implement an argparse CLI with these exact commands:

```text
process_tamagotchi_sprites.py inspect --source-manifest PATH
process_tamagotchi_sprites.py build --manifest PATH --output-root PATH --report PATH [--prune-owned]
process_tamagotchi_sprites.py check --manifest PATH --output-root PATH --report PATH
```

Behavior:

- `inspect` is read-only. It validates phase-4 hashes/layout, prints per-frame alpha/bounds/color facts, and identifies conditions requiring explicit processing-manifest decisions. It may suggest values but must not infer or save background removal, palette mapping, crop, scale, or anchors.
- `build` validates every source and transformation, renders the entire expected tree in a temporary directory, validates that staged tree, then updates the owned output files and writes the report last. Without `--prune-owned`, unexpected PNGs in owned directories are a failure. With `--prune-owned`, only stale `.png` files inside the exact 17 allowlisted directories may be removed.
- `check` is strictly read-only. It rebuilds expected pixels in memory or a temporary directory, compares every expected output and the report byte-for-byte, and fails on a missing, changed, or extra owned PNG.
- all output and diagnostic iteration is sorted by group name and numeric frame index;
- success exits 0; manifest/source contract failures exit 2; image/placement validation failures exit 3; output drift or stale-owned-output failures exit 4;
- errors name the group/frame and violated rule without silently continuing;
- use repository-relative POSIX paths in manifests and reports, never machine-specific absolute paths.

The script must expose a tool schema/version constant and print the Pillow version. PNGs are saved as canonical RGBA with metadata stripped, `optimize=False`, and one fixed compression level. Record the expected Pillow version in the processing manifest and fail with a clear reproducibility error when it differs; changing it requires an intentional manifest update and a reviewed full-output diff.

## Processing manifest contract

Create `_ai/art/tamagotchi/sprite-processing-manifest.json` with `schemaVersion: 1`. It must reference the phase-4 manifest by relative path and SHA-256 and must declare the fixed output root, tool version, Pillow version, the exact 17 owned output directories, global palette definitions, and one entry per group.

Each group entry must contain:

- `group`, `sourceGroup`, and `sourceSha256` matching phase 4;
- `frameCount` and `targetSize` matching the fixed inventory;
- `backgroundRemoval`;
- `alphaThreshold`;
- `palette` mode and ordered palette name/colors;
- one common rational nearest-neighbor scale for the group;
- target canvas anchor and ground line;
- an ordered `frames` array with `index`, explicit source crop box, source anatomical center/ground anchor, optional reviewed integer target offset, and expected output basename.

Use half-open crop rectangles `[left, top, right, bottom]`. Coordinates are integers in the split frame cell. `sourceAnchorX` is the pet body's reviewed horizontal center, not the combined pet-plus-prop bounding-box center. `sourceGroundY` is the row on which the pet's feet rest. Props and effects do not redefine either anchor. For each group, use one rational scale for all frames; never fit each frame independently.

Conceptually:

```json
{
  "schemaVersion": 1,
  "toolVersion": 1,
  "pillowVersion": "<verified version>",
  "sourceManifest": "_ai/art/tamagotchi/generated/strip-manifest.json",
  "sourceManifestSha256": "<sha256>",
  "outputRoot": "raw-png",
  "ownedOutputDirectories": ["tamagotchi_adult_clean", "...all 17, sorted..."],
  "palettes": {
    "pet": ["#RRGGBB", "...approved ordered colors..."],
    "pet_and_props": ["#RRGGBB", "...approved ordered colors..."]
  },
  "groups": [
    {
      "group": "tamagotchi_child_idle",
      "sourceGroup": "tamagotchi_child_idle",
      "sourceSha256": "<sha256>",
      "frameCount": 3,
      "targetSize": [32, 32],
      "backgroundRemoval": {"mode": "none"},
      "alphaThreshold": 128,
      "palette": {"mode": "map_nearest", "name": "pet", "metric": "srgb_squared", "dither": false},
      "scale": {"numerator": 1, "denominator": 4, "filter": "nearest"},
      "placement": {"targetAnchorX": 16, "targetGroundY": 28},
      "frames": [
        {
          "index": 0,
          "crop": [0, 0, 96, 96],
          "sourceAnchorX": 48,
          "sourceGroundY": 84,
          "targetOffset": [0, 0],
          "output": "tamagotchi_child_idle_00.png"
        }
      ]
    }
  ]
}
```

The numbers above are schema examples, not approved art measurements. Populate them only after `inspect` and visual review. Require exact key sets or explicitly document optional keys; reject unknown keys so a typo cannot silently disable a transform.

### Placement rules

For all 32x32 groups, the default target anatomical center is x=16 and the shared foot ground line is y=28, leaving rows 29-31 transparent below a normally grounded pet. For the 16x16 mess, use an independently reviewed icon anchor, normally x=8 and y=14. Any group exception must be explicit and justified in the processing manifest.

Apply the group rational scale with Pillow nearest-neighbor only, using one documented integer rounding rule for resized dimensions and anchor coordinates. Compute the paste origin so the scaled anatomical anchor lands exactly on the target anchor plus the reviewed integer offset. Before compositing, prove the full resized crop falls within the target canvas; never clip overflow. After compositing, recompute the opaque bounding box and fail if it touches a forbidden edge, loses the expected ground relation, or becomes empty.

The crop may remove only transparent padding or explicitly approved removable background. It must not cut visible pixels. Keep scale constant within a group and compare child groups to the approved child occupancy and adult groups to the approved adult occupancy. A pose may move limbs or include a prop, but the torso must not appear to grow or shrink between animations.

## Deterministic pixel pipeline

Process every frame in this order:

1. Verify the strip hash and split the exact phase-4 `[x0, x1)` cell. Do not resample during splitting.
2. Convert to straight 8-bit RGBA while requiring that the source actually has alpha.
3. Apply only the manifest-declared background-removal mode.
4. Apply binary alpha: alpha below the declared threshold becomes 0; alpha at or above it becomes 255. The default threshold is 128, but every group records it.
5. Set every fully transparent pixel to canonical `(0, 0, 0, 0)` so invisible matte RGB cannot create noisy diffs or future halos.
6. Apply the palette policy only to alpha-255 pixels.
7. Validate and crop to the explicit half-open rectangle.
8. Resize with Pillow `Resampling.NEAREST` and the manifest's common rational group scale.
9. Paste on a fresh transparent target canvas using the reviewed anatomical anchor and ground line; fail instead of clipping.
10. Validate final mode, size, alpha values, palette, nonempty content, bounds, anchor, output name, and group count.
11. Save canonical PNG bytes to staging and calculate file and raw-pixel hashes.

### Background-removal policy

`backgroundRemoval.mode` supports only:

- `none`: required for correctly transparent strips;
- `border_key`: an exceptional, reviewer-approved cleanup with exact `keyColor`, integer `maxChannelDelta`, `connectivity: 4`, `approvedBy`, and `reason` fields.

`border_key` flood-fills only pixels connected to the outer border whose RGB channels are each within the declared delta of the key color. It must not globally delete matching interior colors. There is no automatic corner sampling, dominant-color inference, fuzzy subject segmentation, checkerboard removal, or default tolerance. Print the removed-pixel count per frame and inspect the staged composited previews. If the matte shares colors with the outline or artwork, if removal creates holes, or if the image is fully opaque without an unambiguous approved key, return it to phase 4.

### Palette policy

Palette mode is either:

- `validate`: retain RGB values and fail if any visible pixel is outside the ordered approved palette;
- `map_nearest`: map each visible RGB value to the nearest approved color using squared Euclidean distance in 8-bit sRGB, resolving equal distances by first palette order.

Never use adaptive palette generation, dithering, randomness, gamma-dependent system color conversion, or alpha-weighted compositing. The approved palette begins with phase 3's canonical character colors and may add only phase-4-approved generic prop/effect colors. Review any palette addition rather than allowing the tool to invent it. Print colors changed and maximum mapping distance; an unexpectedly large distance is a failed-art signal, not permission for aggressive quantization.

## Output and stale-file ownership

The script owns `.png` files only inside the exact 17 directories listed in the processing manifest. It must never delete or rewrite:

- `raw-png/walking/`;
- any other current or future `raw-png/` directory;
- phase-3 references or phase-4 strips;
- any file under `include/sprites/`.

Build all 45 outputs under a temporary directory first. Promote them only after the complete staged set passes. Treat any unexpected non-PNG file inside an owned directory as a hard failure and preserve it. Without `--prune-owned`, list stale owned PNGs and fail. With `--prune-owned`, remove only those listed stale PNGs after the staged set passes, then remove an empty owned directory only if it is one of the exact allowlisted directories. Never clean by wildcard prefix such as `raw-png/tamagotchi_*`.

Phase 5 owns stale runtime PNG cleanup. Phase 6 owns stale generated `include/sprites/sprite_<basename>.c/.h` cleanup after `png2c.py`; phase 5 must merely report renamed/removed basenames in its handoff.

## Runtime report contract

Generate `_ai/art/tamagotchi/runtime-sprite-manifest.json` deterministically, sorted by group and numeric frame index. Include:

- schema/tool/Pillow versions and the processing-manifest SHA-256;
- source strip path and SHA-256 for each group;
- group count, target dimensions, ordered runtime relative paths, and loop mode;
- for every PNG: basename, `png2c.py`-normalized C identifier, file SHA-256, canonical raw RGBA-pixel SHA-256, byte size, mode, dimensions, alpha values, visible-color list/count, opaque bounding box, target anchor/ground line, and estimated ARGB8888 bytes;
- total group count, descriptor/file count, 32x32 frame count, 16x16 frame count, and total estimated ARGB8888 bytes;
- stale runtime basenames pruned during this exact build, or an empty array.

Do not include timestamps, temporary paths, platform-dependent separators, unordered maps, or absolute paths.

## ARGB8888 byte accounting

`png2c.py` emits four uncompressed bytes per source pixel regardless of how few colors are visible:

```text
one 32x32 frame = 32 * 32 * 4 = 4,096 bytes
44 pet frames   = 44 * 4,096    = 180,224 bytes
one 16x16 frame = 16 * 16 * 4   = 1,024 bytes
total           = 181,248 bytes = 177 KiB
```

The script must print those per-group and aggregate values and fail unless the aggregate is exactly 181,248 bytes for the fixed inventory. This is raw descriptor pixel data only; phase 6 compares actual firmware growth and compiler/linker overhead to the 200 KiB Tamagotchi sprite budget and the `0x1F0000` OTA slot.

## Validation workflow

1. Run `inspect` and reconcile every observation with the phase-4 manifest.
2. Populate the processing manifest with reviewed crops, anchors, scale, alpha, background, and palette decisions.
3. Run `build` without pruning. If it reports stale owned PNGs, review the exact list.
4. Rerun with `--prune-owned` only after confirming every listed file belongs to an obsolete phase-5 output.
5. Inspect a temporary contact sheet at native size, 2x intended display size, and 8x nearest-neighbor pixel-review size. Composite all outputs over black, white, saturated magenta, checkerboard, and the card background color.
6. Play each group in numeric order. Check silhouette, action readability, frame continuity, scale, body center, feet, props, and ground line. Compare all child groups to child idle and all adult groups to adult idle.
7. Inspect the 16x16 mess at actual size and at the intended card placement. It must read independently and must not be normalized as a 32x32 pet.
8. Run `check`; it must report 17 groups, 45 PNGs, 44 at 32x32, one at 16x16, and 181,248 ARGB8888 bytes.
9. Run the same `build` command again without pruning, then run `git diff --exit-code` limited to the phase-owned outputs. There must be no byte change.
10. Review repository status and verify `raw-png/walking/`, `include/sprites/`, source strips, reference art, and unrelated files did not change.

### Required machine checks

- all 17 expected groups exist and no group is missing or extra;
- counts match the fixed inventory and indices are contiguous from `00`;
- all 45 paths and `png2c.py`-normalized identifiers are globally unique across the complete existing raw PNG tree;
- 44 images are exactly 32x32 RGBA and one is exactly 16x16 RGBA;
- every frame has an alpha channel with unique values exactly `{0, 255}`;
- every frame contains at least one alpha-0 and one alpha-255 pixel;
- visible RGB values are all in the selected approved palette;
- no crop discards alpha-255 pixels and no placement clips content;
- opaque bounds and anchor/ground relationships match declared expectations;
- PNG metadata is stripped and encoding settings are fixed;
- runtime report paths, hashes, counts, dimensions, and byte totals match files on disk;
- a second render matches every PNG pixel hash, PNG file hash, and report byte-for-byte.

Visual approval remains required because hashes cannot detect identity drift, jitter, unreadable actions, bad palette mapping, or a misplaced anatomical center.

## Exact execution commands

Run from the repository root:

```sh
git status --short
~/.platformio/penv/bin/python -c "from PIL import Image; print(Image.__version__)"
~/.platformio/penv/bin/python tools/process_tamagotchi_sprites.py inspect \
  --source-manifest _ai/art/tamagotchi/generated/strip-manifest.json
~/.platformio/penv/bin/python tools/process_tamagotchi_sprites.py build \
  --manifest _ai/art/tamagotchi/sprite-processing-manifest.json \
  --output-root raw-png \
  --report _ai/art/tamagotchi/runtime-sprite-manifest.json
```

If and only if the preceding build names reviewed stale phase-5 PNGs:

```sh
~/.platformio/penv/bin/python tools/process_tamagotchi_sprites.py build \
  --manifest _ai/art/tamagotchi/sprite-processing-manifest.json \
  --output-root raw-png \
  --report _ai/art/tamagotchi/runtime-sprite-manifest.json \
  --prune-owned
```

Then prove output consistency and idempotency:

```sh
~/.platformio/penv/bin/python tools/process_tamagotchi_sprites.py check \
  --manifest _ai/art/tamagotchi/sprite-processing-manifest.json \
  --output-root raw-png \
  --report _ai/art/tamagotchi/runtime-sprite-manifest.json
~/.platformio/penv/bin/python tools/process_tamagotchi_sprites.py build \
  --manifest _ai/art/tamagotchi/sprite-processing-manifest.json \
  --output-root raw-png \
  --report _ai/art/tamagotchi/runtime-sprite-manifest.json
git status --short
git diff --check -- tools/process_tamagotchi_sprites.py \
  _ai/art/tamagotchi/sprite-processing-manifest.json \
  _ai/art/tamagotchi/runtime-sprite-manifest.json raw-png
```

Do not run `png2c.py` in this phase. Phase 6 begins with that conversion after this handoff is accepted.

## Failure branches

| Failure | Required response |
| --- | --- |
| Missing Pillow or version mismatch | Stop; restore/use the phase-1-verified PlatformIO Python environment. Do not fall back to a different resizer. |
| Source or manifest hash mismatch | Stop and determine whether phase 3/4 was intentionally revised. Reapprove and update manifests; never process an untracked revision. |
| Wrong/missing/extra group or count | Return to phase 4 or formally revise the parent sprite scope. Do not fabricate, duplicate, or drop frames. |
| Strip width is not exactly divisible or boundaries disagree | Return to phase 4. Do not guess separators or uneven cells. |
| Missing alpha, fully opaque matte, or baked checkerboard | Return to phase 4 unless a reviewer approves an unambiguous `border_key` rule. Never infer the background. |
| `border_key` damages outline/interior colors | Discard staged output and return the strip to phase 4. |
| Crop removes visible pixels | Correct the explicit crop if it was a data-entry error; otherwise return clipped source art to phase 4. |
| Nearest-neighbor result is unreadable at 32x32/16x16 | Return to phase 4. Do not use smoothing, sharpening, or redraw in the processor. |
| Palette mapping has large distance or changes identity/prop readability | Select/reapprove an explicit palette or regenerate the strip; do not use adaptive quantization. |
| Frame cannot fit while retaining scale and anchor | Return to phase 4 for reframing. Never silently downscale one frame or clip it. |
| Center/ground jitter | Correct reviewed per-frame source anchors/offsets if source art is sound; return to phase 4 if body scale or pose itself drifts. |
| Name/symbol collision | Rename the group/output contract coherently before generation and recheck all references. Never overwrite another descriptor. |
| Stale owned PNGs | Review the exact list, then use `--prune-owned`; never perform broad cleanup. |
| Unexpected non-PNG in an owned directory | Stop and preserve it for human review. |
| Second run differs | Treat as nondeterminism; fix ordering, metadata, version pin, rounding, or encoding before handoff. |
| ARGB estimate is not 181,248 bytes | Stop: size/count/dimensions violate the fixed scope. Do not hand off to phase 6. |

## Deliverables

Phase 5 delivers:

1. a reviewed deterministic Pillow processor at `tools/process_tamagotchi_sprites.py`;
2. a complete, hash-bound processing contract at `_ai/art/tamagotchi/sprite-processing-manifest.json`;
3. exactly 45 final runtime PNGs in the 17 named `raw-png/` directories;
4. `_ai/art/tamagotchi/runtime-sprite-manifest.json` containing every path, order, dimension, mode, hash, palette/bounds fact, and byte estimate;
5. recorded visual approval of native, 2x, enlarged, composited, and animated previews;
6. a clean `check` result and clean rerun/no-diff evidence;
7. an explicit list of any runtime basenames removed so phase 6 can remove only their stale generated C/H counterparts.

## Exit criteria

Phase 5 is complete only when:

- all required phase-3 and phase-4 inputs and hashes validate;
- exactly 44 32x32 pet frames and one 16x16 mess frame exist, with no source/reference PNG under `raw-png/`;
- every output is nonempty RGBA with alpha values exactly 0/255, only approved colors, and no clipped pixels or halo;
- every pet uses the reviewed common center and y=28 ground line with stable apparent scale;
- the mess passes independent 16x16 validation;
- directories, basenames, C identifiers, and lexicographic frame order are globally collision-free;
- the output report proves exactly 181,248 bytes of future ARGB8888 pixel data;
- a second build and `check` reproduce identical PNGs and report;
- existing walking assets, generated C/H files, phase-3/4 art, and unrelated worktree changes remain untouched;
- a reviewer confirms the child/adult identity, actions, sickness, doctor, evolution, egg, and mess still read at native size.

## Handoff to phase 6

Phase 6 receives the final `raw-png/` tree and `_ai/art/tamagotchi/runtime-sprite-manifest.json` as its source of truth. It must expect 17 group arrays and 45 LVGL descriptors. For each 32x32 PNG it should verify `LV_COLOR_FORMAT_ARGB8888`, width/height 32, and `data_size = 4096`; for `tamagotchi_mess_00.png` it should verify 16x16 and `data_size = 1024`. Group array order must match the runtime manifest's ordered paths.

Phase 6 owns running `png2c.py`, proving Pillow and NumPy were actually available to it, reviewing `include/sprites/sprites.h` and `sprites.c`, validating all generated `.c/.h` files and BGRA bytes, cleaning only explicitly identified stale generated files, rebuilding firmware, measuring actual binary growth against the 200 KiB asset budget and OTA slot, and testing representative frames on hardware.

If phase 6 finds wrong color order, dimensions, descriptors, grouping, or a `png2c.py` collision, it returns the exact runtime path and evidence to phase 5. If it finds artistic unreadability, the issue goes back to phase 4; phase 6 must not hand-edit generated C and phase 5 must not conceal the problem with destructive normalization.
