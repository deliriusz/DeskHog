# Phase 6 implementation plan: convert and validate embedded sprites

## Outcome

Convert the final Phase 5 Tamagotchi PNGs into the repository's tracked LVGL C descriptors, prove that the generated source is complete and deterministic, compile it through the production PlatformIO environment, measure its actual firmware cost, and render one representative frame on the Feather before gameplay code begins.

This phase owns generated sprite outputs under `include/sprites/`. It does not create or revise artwork, normalize PNGs, implement animation/gameplay, or change the sprite generator's pixel format. If a source PNG is wrong, return it to Phase 5 instead of compensating in generated C.

## Repository facts that constrain this phase

- `png2c.py` recursively sorts `raw-png/**/*.png`, groups frames by the first directory under `raw-png/`, and flattens every per-frame `.c`/`.h` output into `include/sprites/`.
- A PNG basename becomes its C suffix after replacing `-` and `.` with `_`. The first-level directory is used directly as the group symbol prefix. Neither rule fully sanitizes C identifiers.
- Pixel data is emitted in BGRA byte order and described as `LV_COLOR_FORMAT_ARGB8888`.
- `platformio.ini` runs `png2c.py` as a pre-build script and compiles `include/sprites/*.c` through `src_filter`.
- Missing Pillow or NumPy makes `png2c.py` print a warning and exit **successfully** without regeneration. A successful build alone is therefore insufficient proof.
- The generator rewrites `sprites.c` and `sprites.h`, but does not delete obsolete per-frame `.c`/`.h` files. Because PlatformIO compiles the wildcard, stale C files must be removed explicitly.
- The existing walking set is the regression fixture: six 80x80 descriptors, 25,600 bytes each, in `walking_sprites`. Phase 6 must not alter those PNGs or per-frame generated files.
- The application slot is `0x1F0000` = 2,031,616 bytes. There is no asset filesystem; all generated pixels consume application-image space.

## Required input and prerequisites

Do not start conversion until all of the following are true:

1. Phase 1 has supplied the clean baseline build result, baseline `firmware.bin` byte size, reported flash/RAM usage, and the commit/worktree state used for that measurement.
2. Phase 5 has supplied exactly 44 RGBA 32x32 pet frames and one RGBA 16x16 mess frame under `raw-png/`, with binary alpha, globally unique basenames, fixed center/ground line, and no reference sheets or source strips in `raw-png/`.
3. Phase 5's manifest lists every relative path, group, basename, dimensions, mode, and source hash. Its reported totals must be 17 groups, 45 files, and 181,248 ARGB8888 bytes.
4. All Tamagotchi first-level directories and basenames use only lowercase ASCII letters, digits, and underscores. Basenames begin with a letter or underscore and end in zero-padded `_00`, `_01`, etc. No sanitized basename collides with any PNG anywhere else under `raw-png/`, because the generated output directory is flat.
5. `git status --short` is recorded. Preserve unrelated changes; in the currently inspected worktree `.vscode/extensions.json` is modified and must not be touched.

The expected manifest is:

| Group / generated array | Frames | Dimensions | Raw bytes |
| --- | ---: | ---: | ---: |
| `tamagotchi_egg_sprites` | 2 | 32x32 | 8,192 |
| `tamagotchi_child_idle_sprites` | 3 | 32x32 | 12,288 |
| `tamagotchi_adult_idle_sprites` | 3 | 32x32 | 12,288 |
| `tamagotchi_child_feed_sprites` | 3 | 32x32 | 12,288 |
| `tamagotchi_adult_feed_sprites` | 3 | 32x32 | 12,288 |
| `tamagotchi_child_play_sprites` | 3 | 32x32 | 12,288 |
| `tamagotchi_adult_play_sprites` | 3 | 32x32 | 12,288 |
| `tamagotchi_child_clean_sprites` | 3 | 32x32 | 12,288 |
| `tamagotchi_adult_clean_sprites` | 3 | 32x32 | 12,288 |
| `tamagotchi_child_rest_sprites` | 2 | 32x32 | 8,192 |
| `tamagotchi_adult_rest_sprites` | 2 | 32x32 | 8,192 |
| `tamagotchi_child_sick_sprites` | 2 | 32x32 | 8,192 |
| `tamagotchi_adult_sick_sprites` | 2 | 32x32 | 8,192 |
| `tamagotchi_child_doctor_sprites` | 3 | 32x32 | 12,288 |
| `tamagotchi_adult_doctor_sprites` | 3 | 32x32 | 12,288 |
| `tamagotchi_evolve_sprites` | 4 | 32x32 | 16,384 |
| `tamagotchi_mess_sprites` | 1 | 16x16 | 1,024 |

The arithmetic is exact: `44 * 32 * 32 * 4 + 1 * 16 * 16 * 4 = 44 * 4096 + 1024 = 181,248 bytes = 177 KiB`. The initial 200 KiB budget is 204,800 bytes, leaving 23,552 bytes (23 KiB) for descriptor tables, pointer arrays, alignment, and other linker effects. The existing 153,600-byte walking set belongs to the Phase 1 baseline and is not charged again to the Tamagotchi delta.

## Step 1: prove the conversion environment

Use PlatformIO's interpreter explicitly. Do not use unqualified `python3`; on the inspected machine it has Pillow but lacks NumPy, while the PlatformIO environment has Python 3.12.3, Pillow 12.1.0, and NumPy 2.4.1.

Run this proof before touching generated files:

```sh
~/.platformio/penv/bin/python --version
~/.platformio/penv/bin/python -c 'from PIL import Image; import PIL, numpy, sys; print("interpreter=" + sys.executable); print("Pillow=" + PIL.__version__); print("NumPy=" + numpy.__version__)'
~/.platformio/penv/bin/pio --version
```

The import command must exit zero and identify `/home/deliriusz/.platformio/penv/bin/python`. Record the versions in the phase execution notes. If either import fails, install the missing packages into this exact environment, rerun the proof, and only then continue. Never accept the generator's warning-and-zero-exit fallback as success.

## Step 2: implement a read-only sprite verifier

Add `tools/validate_tamagotchi_sprites.py` using only the Python standard library plus Pillow. It must be non-mutating and return nonzero on any discrepancy. Keep the 17-group manifest above in one data structure so source, generated, and aggregate checks cannot drift.

The verifier must:

1. Enumerate the expected zero-padded frame paths from the manifest and compare them as an exact set with Tamagotchi PNGs under `raw-png/`.
2. Open every PNG and require the manifest dimensions, `RGBA` mode, nonempty visible bounds, alpha values restricted to 0 or 255, and `width * height * 4` expected raw bytes.
3. Scan every PNG under `raw-png/`, apply the generator's basename transformation, and fail on duplicate output filenames or C symbols. Validate first-level directory names as C identifiers too.
4. Derive each expected `include/sprites/sprite_<basename>.c` and `.h`; report missing and extra Tamagotchi outputs separately.
5. Parse every expected C file and require its source basename, matching header include, `sprite_<basename>_map`, descriptor symbol, `LV_COLOR_FORMAT_ARGB8888`, exact width, exact height, exact `data_size`, and `.data` pointer. Count the numeric byte literals inside the map initializer and require exactly `data_size` values, each from 0 through 255.
6. Parse every expected header and require exactly one `extern const lv_img_dsc_t sprite_<basename>;` declaration with C linkage guards.
7. Parse `sprites.h` and require exactly one include for every generated frame header plus exactly one array and `uint8_t` count declaration for each of the 17 Tamagotchi groups.
8. Parse `sprites.c` and require each Tamagotchi pointer array to contain exactly the expected descriptor addresses in lexicographic source-path order, followed by the matching `sizeof(array) / sizeof(array[0])` count definition.
9. Require the pre-existing `walking_sprites` array and count to remain present and ordered `Normal-Walking_01` through `_06`.
10. Print a concise summary: 17 Tamagotchi groups, 45 descriptors, 44 descriptors at 4096 bytes, one at 1024 bytes, total 181,248 bytes, plus any missing/stale paths.

Invoke it with the same interpreter:

```sh
~/.platformio/penv/bin/python tools/validate_tamagotchi_sprites.py
```

Before conversion it may fail only for expected missing generated outputs. Source-manifest, dimensions, alpha, naming, collision, or unexpected-source failures must go back to Phase 5.

## Step 3: remove stale generated files safely

Compute, display, and review sets; never clean `include/sprites/` wholesale and never use a wildcard delete.

- `expectedPerFrame` is the exact `.c`/`.h` set derived from every current PNG basename using `png2c.py`'s transformation.
- `actualPerFrame` is the exact set of `include/sprites/sprite_*.c` and `.h` files, excluding aggregate `sprites.c` and `sprites.h`.
- `stale = actualPerFrame - expectedPerFrame`; `missing = expectedPerFrame - actualPerFrame`.

For every stale path, first prove in the Phase 5 manifest and `git diff -- raw-png include/sprites` that its source was deliberately renamed or removed. Remove only the explicit stale paths printed by the verifier, using `git rm -- <exact-paths>` for tracked files or an `apply_patch` file deletion. Do not pipe the computed list into `rm`, use `rm -rf`, or delete the walking outputs. If an extra is unrelated to Tamagotchi or cannot be tied to an intentional source change, stop and ask its owner rather than deleting it.

Run the verifier again. At this point stale must be empty; missing generated paths remain expected until conversion.

## Step 4: perform direct, observable conversion

From the repository root, run the generator explicitly with the proven PlatformIO interpreter:

```sh
~/.platformio/penv/bin/python png2c.py
```

Do not accept the run unless it prints `Successfully processed` and the per-group summary. In the expected Phase 6 tree, the summary is 51 total sprites: the six existing walking frames plus 45 Tamagotchi frames. It must list all 17 Tamagotchi groups with the manifest counts and `walking: 6`. If another legitimate sprite group has been added since the baseline, reconcile the higher total against the full source inventory rather than hard-coding 51 as an unconditional future count.

Run the verifier immediately. All missing, extra, descriptor, aggregate, count, and ordering sets must now be empty.

Expected naming for a source such as `raw-png/tamagotchi_child_idle/tamagotchi_child_idle_00.png` is:

```text
include/sprites/sprite_tamagotchi_child_idle_00.c
include/sprites/sprite_tamagotchi_child_idle_00.h
sprite_tamagotchi_child_idle_00
tamagotchi_child_idle_sprites[]
tamagotchi_child_idle_sprites_count
```

Every group follows this mapping. Frames `_00`, `_01`, and so on are required because the generator's global path sort then provides deterministic animation order.

## Step 5: prove regeneration is deterministic

Hash the complete generated sprite tree, rerun conversion with unchanged sources, hash it again, and compare:

```sh
find include/sprites -maxdepth 1 -type f -print0 | sort -z | xargs -0 sha256sum > /tmp/tamagotchi-sprites-pass1.sha256
~/.platformio/penv/bin/python png2c.py
find include/sprites -maxdepth 1 -type f -print0 | sort -z | xargs -0 sha256sum > /tmp/tamagotchi-sprites-pass2.sha256
diff -u /tmp/tamagotchi-sprites-pass1.sha256 /tmp/tamagotchi-sprites-pass2.sha256
~/.platformio/penv/bin/python tools/validate_tamagotchi_sprites.py
```

`diff` must be empty. A difference with unchanged inputs is a generator defect or nondeterministic input and blocks the phase.

## Step 6: review source-to-generated completeness and diffs

Review both source and generated sides:

```sh
git status --short
git diff --stat -- raw-png include/sprites tools/validate_tamagotchi_sprites.py
git diff -- include/sprites/sprites.h include/sprites/sprites.c
git diff -- include/sprites/sprite_Normal_Walking_01.c include/sprites/sprite_Normal_Walking_01.h
git diff -- include/sprites/sprite_Normal_Walking_02.c include/sprites/sprite_Normal_Walking_02.h
git diff -- include/sprites/sprite_Normal_Walking_03.c include/sprites/sprite_Normal_Walking_03.h
git diff -- include/sprites/sprite_Normal_Walking_04.c include/sprites/sprite_Normal_Walking_04.h
git diff -- include/sprites/sprite_Normal_Walking_05.c include/sprites/sprite_Normal_Walking_05.h
git diff -- include/sprites/sprite_Normal_Walking_06.c include/sprites/sprite_Normal_Walking_06.h
```

Acceptance rules:

- all 45 final runtime PNGs are present and every one has exactly one generated C/H pair;
- no reference image, high-resolution source, contact sheet, or animation strip is under `raw-png/` or represented in `include/sprites/`;
- no orphaned Tamagotchi C/H file remains;
- aggregate files contain the 17 new arrays/counts and preserve the existing walking group;
- existing walking per-frame files have empty diffs;
- generated C changes are purely BGRA byte maps and matching descriptors; no hand edits appear;
- unrelated worktree changes remain untouched.

## Step 7: build through the production path

Run the canonical environment, which deliberately invokes the generators again and compiles the wildcard generated C sources:

```sh
~/.platformio/penv/bin/pio run -e adafruit_feather_esp32s3_reversetft
```

The build must exit zero. Inspect its generator output and require the same successful sprite summary; the green build status is not enough. Run the verifier after the build and repeat the deterministic hash/diff check if any generated file changed. There is no enabled native test environment, so do not report native tests as run or passed.

## Step 8: calculate actual firmware and OTA cost

Read the post-build image size exactly:

```sh
stat -c %s .pio/build/adafruit_feather_esp32s3_reversetft/firmware.bin
~/.platformio/packages/toolchain-xtensa-esp-elf/bin/xtensa-esp32s3-elf-size .pio/build/adafruit_feather_esp32s3_reversetft/firmware.elf
~/.platformio/packages/toolchain-xtensa-esp-elf/bin/xtensa-esp32s3-elf-nm -S --size-sort .pio/build/adafruit_feather_esp32s3_reversetft/firmware.elf | rg 'sprite_tamagotchi_.*_map'
```

Record and calculate:

```text
slotBytes              = 0x1F0000 = 2,031,616
rawTamagotchiBytes      = 44 * 4096 + 1024 = 181,248
spriteBudgetBytes       = 200 * 1024 = 204,800
budgetSlackBeforeOther  = 204,800 - 181,248 = 23,552
actualFirmwareDelta     = postFirmwareBinBytes - baselineFirmwareBinBytes
postBuildSlotHeadroom   = 2,031,616 - postFirmwareBinBytes
baselineSlotHeadroom    = 2,031,616 - baselineFirmwareBinBytes
```

Require all of the following:

- `postFirmwareBinBytes <= 2,031,616` so the artifact fits either OTA application slot;
- `postBuildSlotHeadroom > 0` with enough documented margin for later model, UI, persistence, and clock phases—not merely one byte of nominal fit;
- the 45 `sprite_tamagotchi_*_map` ELF symbols sum to exactly 181,248 bytes;
- `actualFirmwareDelta <= 204,800` for the initial sprite budget, or a documented explanation proves unrelated compiled changes contaminated the baseline comparison.

Do not confuse 181,248 bytes (177 KiB) of raw Tamagotchi pixel maps with the final binary delta. Descriptors, 17 pointer arrays, count symbols, alignment, and link behavior contribute additional bytes. If the isolated delta exceeds 200 KiB, the phase fails even if the OTA slot still fits: return to the sprite manifest and reduce action frame counts first, then repeat conversion and all validation. If the image exceeds the slot, no OTA or hardware upload is allowed.

## Step 9: one-frame hardware smoke test

This is the only phase gate that cannot be completed in the host/container. Use a temporary, narrowly scoped LVGL smoke path or the earliest Phase 10 sprite view to display one representative 32x32 generated descriptor—prefer `tamagotchi_child_idle_00`—on the actual Adafruit ESP32-S3 Reverse TFT Feather.

1. Upload only after the production build and slot checks pass:

   ```sh
   ~/.platformio/penv/bin/pio run -e adafruit_feather_esp32s3_reversetft -t upload
   ~/.platformio/penv/bin/pio device monitor --baud 115200
   ```

2. Create one LVGL image on the LVGL-owning core/task, set the generated descriptor as its source, and use `lv_image_set_scale(image, 512)` for 2x display. Do not add a task or call LVGL from a background context.
3. Visually confirm transparent pixels reveal the room/background, colors are correct rather than red/blue swapped (BGRA contract), the source is natively 32x32, the visible result is approximately 64x64, the character center and ground line match its reference/neighbor frame, and no quill/foot is clipped.
4. Photograph or record the tested frame, firmware commit, board, and result. Remove any temporary smoke-only UI wiring after the observation, rebuild, and verify generated files remain unchanged.

Host checks can prove PNG mode/alpha, generated bytes, descriptor fields, ordering, symbol sizes, compilation, and OTA fit. Only the physical TFT can prove the complete driver/LVGL color interpretation, real transparency appearance, 2x scaling, and panel clipping. If hardware is unavailable, mark this single gate `BLOCKED — hardware not available`; do not claim Phase 6 complete or infer display correctness from desktop inspection.

## Failure and rollback branches

- **Pillow/NumPy proof fails:** stop before generation; repair the PlatformIO Python environment and rerun the import proof. Never fall back to stale outputs.
- **Manifest/count/dimension/alpha/naming failure:** do not edit generated C. Return the named frames to Phase 5, rerun its deterministic processor, then restart Phase 6 from the source audit.
- **Flattened output or symbol collision:** rename the source basenames to globally unique valid identifiers in Phase 5 and regenerate. Directory separation does not prevent collisions.
- **Stale output detected:** prove its source removal, delete only the explicit stale C/H paths, regenerate aggregate files, and rerun exact-set validation. Unexplained or unrelated stale files block deletion.
- **Descriptor or aggregate mismatch:** discard only the affected generated outputs by regenerating from the authoritative PNGs. If a clean rerun repeats the mismatch, fix `png2c.py` deliberately and revalidate both Tamagotchi and walking outputs; never hand-patch generated data.
- **Nondeterministic second pass:** retain source PNGs, revert generated-only changes to the last known-good generated state, diagnose ordering/content nondeterminism, and do not proceed to build.
- **Walking regression:** revert Tamagotchi-phase changes that touched walking inputs/outputs, confirm their sources are unchanged, regenerate, and require an empty walking per-frame diff.
- **Compile failure:** first distinguish malformed generated C/symbol collision from a toolchain error. Fix the source naming or generator, regenerate, then rebuild; do not work around it in gameplay code.
- **Sprite delta over 200 KiB:** preserve the art references, reduce lower-priority action frame counts in the source manifest (not dimensions below legibility first), reprocess in Phase 5, and rerun the whole phase.
- **Firmware exceeds `0x1F0000`:** do not upload or attempt OTA. Reduce assets and rebuild until the binary fits with documented remaining headroom.
- **Hardware shows wrong colors, alpha, scale, or clipping:** keep the failed evidence; compare a known walking descriptor on the same path. Fix source normalization for art-specific problems or the generator/usage contract for systemic problems, then regenerate and repeat. Do not swap channels or paint backgrounds by hand in generated files.

Rollback is source-led: final PNGs and `png2c.py` are authoritative, while all `include/sprites/` products are reproducible. Preserve unrelated user changes and never reset the whole worktree. A failed Phase 6 should leave either the prior known-good generated set or a clearly reviewed, reproducible regenerated set—not a mixture of old and new files.

## Deliverables

- 45 final Tamagotchi PNGs from Phase 5 retained as authoritative inputs.
- 45 matching generated `.c` files and 45 matching `.h` files under `include/sprites/`.
- Updated tracked `include/sprites/sprites.c` and `sprites.h` with 17 Tamagotchi arrays and counts plus the unchanged walking group.
- Read-only `tools/validate_tamagotchi_sprites.py` and a successful validation summary.
- Dependency/interpreter proof, deterministic two-pass hash proof, reviewed source/generated diff, and successful production build record.
- Baseline/post-build byte table including raw map total, actual firmware delta, sprite-budget slack, slot size, and remaining OTA headroom.
- Physical-board smoke-test evidence for one 2x frame, or an explicit hardware blocker.

## Exit criteria and handoff to Phase 7/10

Phase 6 is complete only when all 17 group arrays and counts are exact, all 45 per-frame descriptors pass structural/byte-size checks, total Tamagotchi map data is exactly 181,248 bytes, conversion is reproducible, stale files are absent, walking outputs are unchanged, the production build succeeds within the 2,031,616-byte slot and 200 KiB sprite budget, and one frame has passed the TFT smoke test.

Hand off to the model/UI phases:

- the exact group symbols and `uint8_t` counts from `sprites.h`;
- confirmation that frame order is lexicographic `_00.._NN`;
- descriptor dimensions and 2x scale expectation (`512` in LVGL's 256-based scale);
- actual binary delta and remaining OTA headroom;
- the hardware smoke result and any panel-specific observation;
- the rule that later phases include `sprites/sprites.h`, use the group arrays without copying pixel data, and never edit generated C/H files.
