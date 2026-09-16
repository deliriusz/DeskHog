#!/usr/bin/env python3
"""Validate the Tamagotchi PNG inputs and generated LVGL sprite descriptors.

This deliberately reads only source and generated files.  `png2c.py` exits zero
when Pillow or NumPy is unavailable, so this verifier is the proof that a
conversion really produced complete, usable assets.
"""

from __future__ import annotations

import re
import sys
from collections import Counter
from pathlib import Path
from typing import NamedTuple

from PIL import Image


ROOT = Path(__file__).resolve().parent.parent
RAW_PNG_DIR = ROOT / "raw-png"
SPRITE_DIR = ROOT / "include" / "sprites"
IDENTIFIER_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")


class SpriteGroup(NamedTuple):
    name: str
    frame_count: int
    width: int
    height: int


# This is the authoritative Phase 6 source manifest.  Keep all source,
# descriptor, aggregate, and total checks derived from this one structure.
TAMAGOTCHI_GROUPS = (
    SpriteGroup("tamagotchi_egg", 2, 32, 32),
    SpriteGroup("tamagotchi_child_idle", 3, 32, 32),
    SpriteGroup("tamagotchi_adult_idle", 3, 32, 32),
    SpriteGroup("tamagotchi_child_feed", 3, 32, 32),
    SpriteGroup("tamagotchi_adult_feed", 3, 32, 32),
    SpriteGroup("tamagotchi_child_play", 3, 32, 32),
    SpriteGroup("tamagotchi_adult_play", 3, 32, 32),
    SpriteGroup("tamagotchi_child_clean", 3, 32, 32),
    SpriteGroup("tamagotchi_adult_clean", 3, 32, 32),
    SpriteGroup("tamagotchi_child_rest", 2, 32, 32),
    SpriteGroup("tamagotchi_adult_rest", 2, 32, 32),
    SpriteGroup("tamagotchi_child_sick", 2, 32, 32),
    SpriteGroup("tamagotchi_adult_sick", 2, 32, 32),
    SpriteGroup("tamagotchi_child_doctor", 3, 32, 32),
    SpriteGroup("tamagotchi_adult_doctor", 3, 32, 32),
    SpriteGroup("tamagotchi_evolve", 4, 32, 32),
    SpriteGroup("tamagotchi_mess", 1, 16, 16),
)

WALKING_SUFFIXES = tuple(f"Normal_Walking_{index:02d}" for index in range(1, 7))


def c_suffix(path: Path) -> str:
    """Match png2c.py's filename-to-C-symbol transformation exactly."""
    return path.stem.replace("-", "_").replace(".", "_")


def relative(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


class Validation:
    def __init__(self) -> None:
        self.errors: list[str] = []
        self.missing: list[str] = []
        self.stale: list[str] = []

    def fail(self, message: str) -> None:
        self.errors.append(message)

    def require(self, condition: bool, message: str) -> None:
        if not condition:
            self.fail(message)

    def report_paths(self, heading: str, paths: list[str]) -> None:
        if paths:
            print(f"{heading} ({len(paths)}):")
            for path in paths:
                print(f"  {path}")

    def finish(self) -> int:
        self.report_paths("Missing generated outputs", self.missing)
        self.report_paths("Stale generated outputs", self.stale)
        discrepancy_count = len(self.errors) + len(self.missing) + len(self.stale)
        if discrepancy_count:
            print(f"Validation failed with {discrepancy_count} discrepancy(s):")
            for error in self.errors:
                print(f"  {error}")
            return 1

        print(
            "Validated 17 Tamagotchi groups, 45 descriptors: "
            "44 at 4096 bytes and 1 at 1024 bytes (181248 bytes total)."
        )
        return 0


def expected_tamagotchi_sources() -> dict[Path, SpriteGroup]:
    expected: dict[Path, SpriteGroup] = {}
    for group in TAMAGOTCHI_GROUPS:
        for index in range(group.frame_count):
            name = f"{group.name}_{index:02d}.png"
            expected[RAW_PNG_DIR / group.name / name] = group
    return expected


def expected_generated_paths(png_paths: list[Path]) -> set[Path]:
    expected: set[Path] = set()
    for png_path in png_paths:
        suffix = c_suffix(png_path)
        expected.add(SPRITE_DIR / f"sprite_{suffix}.c")
        expected.add(SPRITE_DIR / f"sprite_{suffix}.h")
    return expected


def expected_group_suffixes(group: SpriteGroup) -> list[str]:
    return [f"{group.name}_{index:02d}" for index in range(group.frame_count)]


def validate_png_inputs(validation: Validation, expected_sources: dict[Path, SpriteGroup]) -> list[Path]:
    png_paths = sorted(RAW_PNG_DIR.rglob("*.png"))
    actual_tamagotchi = {path for path in png_paths if path.parts[-2].startswith("tamagotchi_")}
    expected_tamagotchi = set(expected_sources)

    missing_sources = sorted(expected_tamagotchi - actual_tamagotchi)
    unexpected_sources = sorted(actual_tamagotchi - expected_tamagotchi)
    for path in missing_sources:
        validation.fail(f"missing source PNG: {relative(path)}")
    for path in unexpected_sources:
        validation.fail(f"unexpected Tamagotchi source PNG: {relative(path)}")

    for path, group in expected_sources.items():
        if not path.exists():
            continue
        try:
            with Image.open(path) as image:
                validation.require(
                    image.size == (group.width, group.height),
                    f"{relative(path)} dimensions are {image.size}, expected {(group.width, group.height)}",
                )
                validation.require(
                    image.mode == "RGBA",
                    f"{relative(path)} mode is {image.mode}, expected RGBA",
                )
                alpha = image.getchannel("A") if image.mode == "RGBA" else None
                if alpha is not None:
                    alpha_values = {
                        value for value, count in enumerate(alpha.histogram()) if count
                    }
                    validation.require(
                        alpha_values <= {0, 255},
                        f"{relative(path)} has non-binary alpha values",
                    )
                    visible_bounds = alpha.getbbox()
                    validation.require(
                        visible_bounds is not None,
                        f"{relative(path)} has no visible pixels",
                    )
                validation.require(
                    image.width * image.height * 4 == group.width * group.height * 4,
                    f"{relative(path)} has {image.width * image.height * 4} raw bytes, "
                    f"expected {group.width * group.height * 4}",
                )
        except OSError as error:
            validation.fail(f"cannot open {relative(path)}: {error}")

    outputs: dict[str, list[Path]] = {}
    symbols: dict[str, list[Path]] = {}
    for path in png_paths:
        try:
            group_name = path.relative_to(RAW_PNG_DIR).parts[0]
        except ValueError:
            validation.fail(f"PNG outside raw-png: {path}")
            continue
        validation.require(
            bool(IDENTIFIER_RE.fullmatch(group_name)),
            f"generator group is not a C identifier: {group_name}",
        )
        suffix = c_suffix(path)
        outputs.setdefault(f"sprite_{suffix}", []).append(path)
        symbols.setdefault(f"sprite_{suffix}", []).append(path)
        validation.require(
            bool(IDENTIFIER_RE.fullmatch(suffix)),
            f"sprite suffix is not a C identifier: {relative(path)} -> {suffix}",
        )

    for output_name, paths in outputs.items():
        if len(paths) > 1:
            names = ", ".join(relative(path) for path in paths)
            validation.fail(f"duplicate generated output {output_name}: {names}")
    for symbol, paths in symbols.items():
        if len(paths) > 1:
            names = ", ".join(relative(path) for path in paths)
            validation.fail(f"duplicate C symbol {symbol}: {names}")

    return png_paths


def validate_descriptor(validation: Validation, source: Path, group: SpriteGroup) -> None:
    suffix = c_suffix(source)
    c_path = SPRITE_DIR / f"sprite_{suffix}.c"
    h_path = SPRITE_DIR / f"sprite_{suffix}.h"
    descriptor = f"sprite_{suffix}"
    data_size = group.width * group.height * 4

    if not c_path.exists() or not h_path.exists():
        return

    c_text = c_path.read_text(encoding="utf-8")
    h_text = h_path.read_text(encoding="utf-8")
    source_name = source.name
    validation.require(
        f"generated from {source_name}" in c_text and f"Image data for {source_name}" in c_text,
        f"{relative(c_path)} does not identify source {source_name}",
    )
    validation.require(
        f'#include "sprite_{suffix}.h"' in c_text,
        f"{relative(c_path)} does not include its matching header",
    )

    map_match = re.search(
        rf"static const uint8_t {re.escape(descriptor)}_map\[\]\s*=\s*\{{(.*?)\n\}};",
        c_text,
        re.DOTALL,
    )
    validation.require(map_match is not None, f"{relative(c_path)} lacks {descriptor}_map initializer")
    if map_match is not None:
        literal_tokens = re.findall(r"\b\d+\b", map_match.group(1))
        invalid = [token for token in literal_tokens if not 0 <= int(token) <= 255]
        validation.require(
            len(literal_tokens) == data_size,
            f"{relative(c_path)} has {len(literal_tokens)} map bytes, expected {data_size}",
        )
        validation.require(
            not invalid,
            f"{relative(c_path)} has map byte literal outside 0..255",
        )

    descriptor_match = re.search(
        rf"const lv_img_dsc_t {re.escape(descriptor)}\s*=\s*\{{(.*?)\n\}};",
        c_text,
        re.DOTALL,
    )
    validation.require(descriptor_match is not None, f"{relative(c_path)} lacks descriptor {descriptor}")
    if descriptor_match is not None:
        body = descriptor_match.group(1)
        for expected in (
            "LV_COLOR_FORMAT_ARGB8888",
            f".w = {group.width}",
            f".h = {group.height}",
            f".data_size = {data_size}",
            f".data = {descriptor}_map",
        ):
            validation.require(expected in body, f"{relative(c_path)} lacks descriptor field {expected}")

    declaration = rf"extern const lv_img_dsc_t {re.escape(descriptor)};"
    validation.require(
        len(re.findall(declaration, h_text)) == 1,
        f"{relative(h_path)} must contain exactly one {declaration}",
    )
    linkage_guard = re.compile(
        r'#ifdef __cplusplus\s+extern "C" \{\s+#endif.*?#ifdef __cplusplus\s+\}\s+#endif',
        re.DOTALL,
    )
    validation.require(
        linkage_guard.search(h_text) is not None,
        f"{relative(h_path)} lacks C linkage guards",
    )


def validate_generated_files(
    validation: Validation, png_paths: list[Path], expected_sources: dict[Path, SpriteGroup]
) -> None:
    expected_outputs = expected_generated_paths(png_paths)
    actual_outputs = {
        path
        for path in SPRITE_DIR.glob("sprite_*")
        if path.is_file() and path.suffix in {".c", ".h"}
    }
    missing = sorted(expected_outputs - actual_outputs)
    stale = sorted(actual_outputs - expected_outputs)
    validation.missing.extend(relative(path) for path in missing)
    validation.stale.extend(relative(path) for path in stale)

    for source, group in expected_sources.items():
        validate_descriptor(validation, source, group)


def validate_aggregates(validation: Validation) -> None:
    sprites_h = SPRITE_DIR / "sprites.h"
    sprites_c = SPRITE_DIR / "sprites.c"
    for path in (sprites_h, sprites_c):
        validation.require(path.exists(), f"missing aggregate file: {relative(path)}")
    if not sprites_h.exists() or not sprites_c.exists():
        return

    h_text = sprites_h.read_text(encoding="utf-8")
    c_text = sprites_c.read_text(encoding="utf-8")
    includes = re.findall(r'^#include "(sprite_[^"]+\.h)"$', h_text, re.MULTILINE)
    include_counts = Counter(includes)

    for group in TAMAGOTCHI_GROUPS:
        suffixes = expected_group_suffixes(group)
        group_outputs = {
            SPRITE_DIR / f"sprite_{suffix}{extension}"
            for suffix in suffixes
            for extension in (".c", ".h")
        }
        # Before conversion, missing C/H files are the only expected failures.
        # Aggregate mismatches are meaningful only after every group descriptor
        # exists, and suppressing them keeps the pre-conversion report focused.
        if not all(path.exists() for path in group_outputs):
            continue
        for suffix in suffixes:
            header = f"sprite_{suffix}.h"
            validation.require(
                include_counts[header] == 1,
                f"{relative(sprites_h)} must include {header} exactly once",
            )
        _validate_group_aggregate(validation, h_text, c_text, group.name, suffixes)

    _validate_group_aggregate(validation, h_text, c_text, "walking", list(WALKING_SUFFIXES))
    _validate_linker_root(validation, c_text)


def _validate_linker_root(validation: Validation, c_text: str) -> None:
    generator_groups = set()
    for png_path in RAW_PNG_DIR.rglob("*.png"):
        parts = png_path.relative_to(RAW_PNG_DIR).parts
        generator_groups.add(parts[0] if len(parts) > 1 else "root")
    expected_arrays = [f"{group}_sprites" for group in sorted(generator_groups)]

    root_match = re.search(
        r"const void\* const deskhog_all_sprite_groups\[\]\s*=\s*\{(.*?)\n\};",
        c_text,
        re.DOTALL,
    )
    validation.require(
        root_match is not None,
        f"{relative(SPRITE_DIR / 'sprites.c')} lacks deskhog_all_sprite_groups linker root",
    )
    if root_match is not None:
        actual_arrays = re.findall(r"\b([A-Za-z_][A-Za-z0-9_]*_sprites),", root_match.group(1))
        validation.require(
            actual_arrays == expected_arrays,
            f"{relative(SPRITE_DIR / 'sprites.c')} linker root does not contain every generated group",
        )

    expected_count = (
        "const uint8_t deskhog_all_sprite_groups_count = "
        "sizeof(deskhog_all_sprite_groups) / sizeof(deskhog_all_sprite_groups[0]);"
    )
    validation.require(
        c_text.count(expected_count) == 1,
        f"{relative(SPRITE_DIR / 'sprites.c')} lacks the linker-root count definition",
    )


def _validate_group_aggregate(
    validation: Validation, h_text: str, c_text: str, group_name: str, suffixes: list[str]
) -> None:
    array_name = f"{group_name}_sprites"
    count_name = f"{array_name}_count"
    declaration = re.escape(f"extern const lv_img_dsc_t* {array_name}[];")
    count_declaration = re.escape(f"extern const uint8_t {count_name};")
    validation.require(
        len(re.findall(declaration, h_text)) == 1,
        f"{relative(SPRITE_DIR / 'sprites.h')} must declare {array_name} exactly once",
    )
    validation.require(
        len(re.findall(count_declaration, h_text)) == 1,
        f"{relative(SPRITE_DIR / 'sprites.h')} must declare {count_name} exactly once",
    )

    array_match = re.search(
        rf"const lv_img_dsc_t\* {re.escape(array_name)}\[\]\s*=\s*\{{(.*?)\n\}};",
        c_text,
        re.DOTALL,
    )
    validation.require(array_match is not None, f"{relative(SPRITE_DIR / 'sprites.c')} lacks {array_name}")
    if array_match is not None:
        addresses = re.findall(r"&sprite_([A-Za-z_][A-Za-z0-9_]*)", array_match.group(1))
        validation.require(
            addresses == suffixes,
            f"{relative(SPRITE_DIR / 'sprites.c')} {array_name} ordering/content does not match source order",
        )

    expected_count = f"const uint8_t {count_name} = sizeof({array_name}) / sizeof({array_name}[0]);"
    validation.require(
        c_text.count(expected_count) == 1,
        f"{relative(SPRITE_DIR / 'sprites.c')} must define {count_name} using sizeof(array) / sizeof(array[0])",
    )


def main() -> int:
    validation = Validation()
    expected_sources = expected_tamagotchi_sources()
    png_paths = validate_png_inputs(validation, expected_sources)
    validate_generated_files(validation, png_paths, expected_sources)
    validate_aggregates(validation)
    return validation.finish()


if __name__ == "__main__":
    sys.exit(main())
