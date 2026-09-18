# Generated assets

DeskHog embeds its portal, fonts, and sprites into firmware. PlatformIO runs three generation scripts before every build, and the generated C/C++ files under `include/` are tracked.

Always edit source assets and review the resulting generated diff.

## Source-to-output map

| Source | Generator | Output |
|---|---|---|
| `html/portal.html`, `.css`, `.js` | `htmlconvert.py` | `include/html_portal.h` |
| TTF files under `typography/` | `ttf2c.py` | `include/fonts/*.c`, `*.h`, and `fonts.h` |
| PNG files under `raw-png/` | `png2c.py` | `include/sprites/*.c`, `*.h`, `sprites.c`, and `sprites.h` |

The PlatformIO `src_filter` explicitly compiles generated font and sprite `.c` files.

## Portal

`htmlconvert.py` reads the three portal source files, replaces the CSS link and JavaScript script tag with inline content, escapes it as a C string, and writes `PORTAL_HTML` in program memory.

- Preview source changes from `html/portal.html` during development.
- Do not add CDN, hosted font, or other network dependencies; the portal must work before upstream Wi-Fi is configured.
- The generated header can be much larger than the raw page because of C-string escaping; judge the embedded content and final firmware size.

No separate portal-size budget is currently enforced. Use final firmware size and the OTA slot limit as the constraints. Define any future portal budget in build or CI configuration, not prose.

## Fonts

`ttf2c.py` invokes `npx lv_font_conv` with 4-bit glyphs and no compression. It currently generates:

| Symbol | Source | Size / use |
|---|---|---|
| `font_label` | Inter Regular | 15 px labels |
| `font_value` | Inter SemiBold | 16 px values |
| `font_value_large` | Inter SemiBold | 36 px prominent values |
| `font_loud_noises` | LoudNoises | 20 px decorative/game text |

`Style` exposes these through lazy static getters. Use the shared getters instead of referencing generated font symbols throughout UI code.

The script requires Node.js/npm and runs `npm install --no-save lv_font_conv`. A missing npm installation fails font generation and therefore the build.

When PlatformIO imports `ttf2c.py`, it uses PlatformIO's project directory; a
direct invocation uses the directory containing the script. All four required
font inputs are checked before conversion, and a missing input or failed
conversion exits non-zero instead of allowing stale generated C files to be
compiled. A successful run prints `Successfully processed 4 of 4 fonts` and
`All fonts were successfully converted to LVGL format!`.

## Sprites

`png2c.py` recursively reads `raw-png/**/*.png`, converts each image to RGBA, and writes BGRA bytes for LVGL `ARGB8888` descriptors. It groups images by the first subdirectory under `raw-png/` and creates an ordered pointer array such as:

```cpp
walking_sprites[]
walking_sprites_count
```

File names determine C symbol names and sorted animation order. Use stable, sortable names such as numbered frame suffixes.

The script needs Pillow and NumPy in PlatformIO's Python environment. If they are missing, it prints a warning and exits successfully without regenerating sprites; stale generated files may therefore remain in the build.

## Using sprites

```cpp
#include "sprites/sprites.h"

lv_obj_t* animation = lv_animimg_create(parent);
lv_animimg_set_src(animation,
                   reinterpret_cast<const void**>(walking_sprites),
                   walking_sprites_count);
```

ARGB8888 uses four bytes per source pixel before compiler/linker effects. Crop transparent space, minimize dimensions and frames, and check the firmware map/size after changes.

## Review checklist

- Source files, not just generated output, are included in the change.
- The normal PlatformIO build regenerated every expected file.
- Generated diffs contain only intentional changes.
- Font character ranges include every required glyph.
- Sprite names produce valid unique C identifiers and deterministic ordering.
- The portal has no remote dependencies, and its contribution to final firmware size was reviewed.
- The final firmware fits a `0x1F0000` OTA slot.
