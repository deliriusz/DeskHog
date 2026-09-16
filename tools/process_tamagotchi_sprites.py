#!/usr/bin/env python3
"""Deterministically normalize the accepted Tamagotchi strip handoff.

This tool deliberately has a narrow remit: it turns hash-bound, horizontally
split source strips into the reviewed runtime PNG tree.  It never attempts to
find a background, crop, palette, scale, or anatomical anchor on its own.
Those decisions live in sprite-processing-manifest.json and are validated
strictly here so a typo cannot silently change the rendered sprites.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import sys
import tempfile
from collections import deque
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable

from PIL import Image


TOOL_SCHEMA_VERSION = 1
TOOL_VERSION = 1
PNG_COMPRESSION_LEVEL = 9
NAME_PATTERN = re.compile(r"^[a-z][a-z0-9_]*$")

# Kept as data rather than inferred from either manifest.  The phase contract
# is intentionally a second check on the accepted Phase-4 handoff.
FIXED_INVENTORY: dict[str, tuple[int, tuple[int, int]]] = {
    "tamagotchi_adult_clean": (3, (32, 32)),
    "tamagotchi_adult_doctor": (3, (32, 32)),
    "tamagotchi_adult_feed": (3, (32, 32)),
    "tamagotchi_adult_idle": (3, (32, 32)),
    "tamagotchi_adult_play": (3, (32, 32)),
    "tamagotchi_adult_rest": (2, (32, 32)),
    "tamagotchi_adult_sick": (2, (32, 32)),
    "tamagotchi_child_clean": (3, (32, 32)),
    "tamagotchi_child_doctor": (3, (32, 32)),
    "tamagotchi_child_feed": (3, (32, 32)),
    "tamagotchi_child_idle": (3, (32, 32)),
    "tamagotchi_child_play": (3, (32, 32)),
    "tamagotchi_child_rest": (2, (32, 32)),
    "tamagotchi_child_sick": (2, (32, 32)),
    "tamagotchi_egg": (2, (32, 32)),
    "tamagotchi_evolve": (4, (32, 32)),
    "tamagotchi_mess": (1, (16, 16)),
}


class ContractError(Exception):
    """A manifest or source handoff is not the approved contract."""


class ImageValidationError(Exception):
    """Pixels, crops, placement, or rendering facts are invalid."""


class OutputDriftError(Exception):
    """The owned output tree or deterministic report does not match."""


@dataclass(frozen=True)
class RenderedFrame:
    group: str
    index: int
    basename: str
    relative_path: str
    image: Image.Image
    palette_name: str
    target_anchor_x: int
    target_ground_y: int
    colors_changed: int
    max_mapping_distance: int


def repository_root() -> Path:
    return Path(__file__).resolve().parent.parent


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load_json(path: Path) -> dict[str, Any]:
    try:
        with path.open("r", encoding="utf-8") as file:
            result = json.load(file)
    except FileNotFoundError as error:
        raise ContractError(f"missing manifest: {display_path(path)}") from error
    except json.JSONDecodeError as error:
        raise ContractError(f"invalid JSON in {display_path(path)}: {error}") from error
    if not isinstance(result, dict):
        raise ContractError(f"manifest must be an object: {display_path(path)}")
    return result


def stable_json_bytes(value: dict[str, Any]) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode("utf-8")


def display_path(path: Path) -> str:
    root = repository_root()
    try:
        return path.resolve().relative_to(root).as_posix()
    except ValueError:
        return path.as_posix()


def repository_path(value: str, label: str) -> Path:
    if not isinstance(value, str) or not value:
        raise ContractError(f"{label} must be a non-empty repository-relative POSIX path")
    candidate = Path(value)
    if candidate.is_absolute() or "\\" in value or ".." in candidate.parts:
        raise ContractError(f"{label} must be a safe repository-relative POSIX path: {value!r}")
    root = repository_root().resolve()
    resolved = (root / candidate).resolve()
    try:
        resolved.relative_to(root)
    except ValueError as error:
        raise ContractError(f"{label} escapes the repository: {value!r}") from error
    return resolved


def relative_repository_path(path: Path) -> str:
    try:
        return path.resolve().relative_to(repository_root().resolve()).as_posix()
    except ValueError as error:
        raise ContractError(f"path is outside the repository: {path}") from error


def require_keys(
    value: dict[str, Any], required: set[str], label: str, optional: set[str] | None = None
) -> None:
    optional = optional or set()
    actual = set(value)
    missing = sorted(required - actual)
    unknown = sorted(actual - required - optional)
    if missing or unknown:
        problems: list[str] = []
        if missing:
            problems.append("missing " + ", ".join(missing))
        if unknown:
            problems.append("unknown " + ", ".join(unknown))
        raise ContractError(f"{label} has invalid keys ({'; '.join(problems)})")


def require_integer(value: Any, label: str, minimum: int | None = None) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise ContractError(f"{label} must be an integer")
    if minimum is not None and value < minimum:
        raise ContractError(f"{label} must be at least {minimum}")
    return value


def require_pair(value: Any, label: str) -> tuple[int, int]:
    if not isinstance(value, list) or len(value) != 2:
        raise ContractError(f"{label} must be a two-integer array")
    return (
        require_integer(value[0], f"{label}[0]"),
        require_integer(value[1], f"{label}[1]"),
    )


def require_quad(value: Any, label: str) -> tuple[int, int, int, int]:
    if not isinstance(value, list) or len(value) != 4:
        raise ContractError(f"{label} must be a four-integer half-open rectangle")
    return tuple(require_integer(item, f"{label}[{index}]") for index, item in enumerate(value))  # type: ignore[return-value]


def rounded_scaled_dimension(value: int, numerator: int, denominator: int) -> int:
    """Round positive dimensions to nearest integer, with .5 rounded upward."""
    return (value * numerator + denominator // 2) // denominator


def scaled_pixel_coordinate(value: int, numerator: int, denominator: int) -> int:
    """Map a source pixel index using the same end-inclusive rounding as size.

    A source crop's last pixel maps to the resized image's last pixel.  This
    makes the declared ground pixel unambiguous even for fractional scales.
    """
    return rounded_scaled_dimension(value + 1, numerator, denominator) - 1


def hex_to_rgb(value: str, label: str) -> tuple[int, int, int]:
    if not isinstance(value, str) or not re.fullmatch(r"#[0-9A-F]{6}", value):
        raise ContractError(f"{label} must be an uppercase #RRGGBB color")
    return (int(value[1:3], 16), int(value[3:5], 16), int(value[5:7], 16))


def rgb_to_hex(rgb: tuple[int, int, int]) -> str:
    return "#{:02X}{:02X}{:02X}".format(*rgb)


def alpha_values(image: Image.Image) -> list[int]:
    return sorted(set(image.getchannel("A").get_flattened_data()))


def alpha_bbox(image: Image.Image) -> tuple[int, int, int, int] | None:
    alpha = image.getchannel("A")
    return alpha.getbbox()


def visible_colors(image: Image.Image, palette: list[tuple[int, int, int]] | None = None) -> list[tuple[int, int, int]]:
    pixels = image.get_flattened_data()
    used = {(red, green, blue) for red, green, blue, alpha in pixels if alpha == 255}
    if palette is None:
        return sorted(used)
    return [color for color in palette if color in used]


def ensure_rgba_source(image: Image.Image, label: str) -> Image.Image:
    if "A" not in image.getbands():
        raise ImageValidationError(f"{label} has no real alpha channel")
    return image.convert("RGBA")


def source_groups_by_name(source_manifest: dict[str, Any], source_path: Path) -> dict[str, dict[str, Any]]:
    if source_manifest.get("schemaVersion") != 1:
        raise ContractError(f"{display_path(source_path)} schemaVersion must be 1")
    for key, expected in {
        "expectedGroupCount": 17,
        "expectedFrameCount": 45,
        "target32FrameCount": 44,
        "target16FrameCount": 1,
        "estimatedArgb8888Bytes": 181248,
    }.items():
        if source_manifest.get(key) != expected:
            raise ContractError(f"{display_path(source_path)} {key} must be {expected}")
    groups = source_manifest.get("groups")
    if not isinstance(groups, list):
        raise ContractError(f"{display_path(source_path)} groups must be an array")
    result: dict[str, dict[str, Any]] = {}
    for group in groups:
        if not isinstance(group, dict) or not isinstance(group.get("group"), str):
            raise ContractError("phase-4 group entries must contain a string group")
        name = group["group"]
        if name in result:
            raise ContractError(f"duplicate phase-4 group: {name}")
        result[name] = group
    if set(result) != set(FIXED_INVENTORY):
        missing = sorted(set(FIXED_INVENTORY) - set(result))
        extra = sorted(set(result) - set(FIXED_INVENTORY))
        raise ContractError(f"phase-4 groups differ from fixed inventory (missing={missing}, extra={extra})")
    return result


def validate_phase3_binding(source_manifest: dict[str, Any], source_path: Path) -> None:
    binding = source_manifest.get("phase3Manifest")
    if not isinstance(binding, dict):
        raise ContractError("phase-4 manifest must bind phase3Manifest")
    require_keys(binding, {"relativePath", "sha256"}, "phase3Manifest")
    phase3_path = repository_path(binding["relativePath"], "phase3Manifest.relativePath")
    expected_hash = binding["sha256"]
    if not isinstance(expected_hash, str) or sha256_file(phase3_path) != expected_hash:
        raise ContractError(f"phase-3 manifest hash mismatch: {display_path(phase3_path)}")
    phase3 = load_json(phase3_path)
    if phase3.get("schemaVersion") != 1:
        raise ContractError("phase-3 reference manifest schemaVersion must be 1")
    masters = phase3.get("acceptedMasters")
    if not isinstance(masters, dict) or set(masters) != {"child", "adult"}:
        raise ContractError("phase-3 acceptedMasters must contain child and adult")
    for name in ("child", "adult"):
        master = masters[name]
        if not isinstance(master, dict):
            raise ContractError(f"phase-3 {name} master must be an object")
        path = repository_path(master.get("relativePath"), f"phase-3 {name} master path")
        expected = master.get("sha256")
        if not isinstance(expected, str) or sha256_file(path) != expected:
            raise ContractError(f"phase-3 {name} master hash mismatch: {display_path(path)}")
    inputs = phase3.get("generationInputs")
    if not isinstance(inputs, list):
        raise ContractError("phase-3 generationInputs must be an array")
    for index, item in enumerate(inputs):
        if not isinstance(item, dict):
            raise ContractError(f"phase-3 generationInputs[{index}] must be an object")
        path = repository_path(item.get("relativePath"), f"phase-3 generation input {index} path")
        expected = item.get("sha256")
        if not isinstance(expected, str) or sha256_file(path) != expected:
            raise ContractError(f"phase-3 generation input hash mismatch: {display_path(path)}")
    prompts = repository_path("_ai/art/tamagotchi/prompts.md", "phase-3 prompts")
    if not prompts.is_file():
        raise ContractError("missing required phase-3 prompts.md")


def validate_source_group(group: dict[str, Any], name: str) -> list[Image.Image]:
    required = {
        "group",
        "source",
        "sha256",
        "byteSize",
        "width",
        "height",
        "colorMode",
        "frameCount",
        "layout",
        "frameWidth",
        "frameHeight",
        "splitBoundaries",
        "targetRuntimeWidth",
        "targetRuntimeHeight",
        "playback",
        "frameIntent",
        "identityReferences",
        "promptId",
        "revision",
        "generation",
        "approvalDate",
        "reviewer",
        "acceptanceNotes",
        "observed",
        "paletteComparison",
        "requiredPhase5Corrections",
    }
    require_keys(group, required, f"phase-4 group {name}")
    count, target = FIXED_INVENTORY[name]
    if group["group"] != name:
        raise ContractError(f"phase-4 group record has mismatched name: {name}")
    if group["layout"] != "horizontal":
        raise ContractError(f"{name} layout must be horizontal")
    if group["frameCount"] != count:
        raise ContractError(f"{name} frameCount must be {count}")
    if (group["targetRuntimeWidth"], group["targetRuntimeHeight"]) != target:
        raise ContractError(f"{name} target runtime size must be {target[0]}x{target[1]}")
    source_path = repository_path(group["source"], f"{name} source")
    if not source_path.is_file():
        raise ContractError(f"{name} source is missing: {display_path(source_path)}")
    if sha256_file(source_path) != group["sha256"]:
        raise ContractError(f"{name} source SHA-256 mismatch: {display_path(source_path)}")
    if source_path.stat().st_size != group["byteSize"]:
        raise ContractError(f"{name} source byte size mismatch: {display_path(source_path)}")
    try:
        with Image.open(source_path) as opened:
            image = ensure_rgba_source(opened, f"{name} strip")
    except OSError as error:
        raise ImageValidationError(f"cannot read {name} strip: {error}") from error
    if image.mode != "RGBA" or group["colorMode"] != "RGBA":
        raise ImageValidationError(f"{name} strip must be RGBA")
    if image.size != (group["width"], group["height"]):
        raise ImageValidationError(f"{name} strip dimensions differ from its manifest")
    if group["height"] != group["frameHeight"]:
        raise ContractError(f"{name} frameHeight must equal source height")
    if group["width"] != group["frameCount"] * group["frameWidth"]:
        raise ContractError(f"{name} width must equal frameCount * frameWidth")
    boundaries = group["splitBoundaries"]
    if not isinstance(boundaries, list) or len(boundaries) != count:
        raise ContractError(f"{name} splitBoundaries must have {count} entries")
    frames: list[Image.Image] = []
    expected_x = 0
    for index, boundary in enumerate(boundaries):
        if not isinstance(boundary, dict):
            raise ContractError(f"{name} split boundary {index} must be an object")
        require_keys(boundary, {"index", "x0", "x1"}, f"{name} split boundary {index}")
        if boundary["index"] != index or boundary["x0"] != expected_x:
            raise ContractError(f"{name} split boundary {index} is not contiguous")
        x0 = require_integer(boundary["x0"], f"{name} split boundary {index} x0", 0)
        x1 = require_integer(boundary["x1"], f"{name} split boundary {index} x1", 1)
        if x1 - x0 != group["frameWidth"]:
            raise ContractError(f"{name} split boundary {index} is not equal-width")
        expected_x = x1
        frame = image.crop((x0, 0, x1, image.height))
        values = alpha_values(frame)
        if 0 not in values or not any(value > 0 for value in values):
            raise ImageValidationError(f"{name} frame {index} must contain transparent and visible pixels")
        frames.append(frame)
    if expected_x != image.width:
        raise ContractError(f"{name} split boundaries do not cover the strip exactly")
    return frames


def load_and_validate_source_manifest(source_path: Path) -> tuple[dict[str, Any], dict[str, dict[str, Any]], dict[str, list[Image.Image]]]:
    source_manifest = load_json(source_path)
    groups = source_groups_by_name(source_manifest, source_path)
    validate_phase3_binding(source_manifest, source_path)
    frames_by_group: dict[str, list[Image.Image]] = {}
    for name in sorted(groups):
        frames_by_group[name] = validate_source_group(groups[name], name)
    return source_manifest, groups, frames_by_group


def validate_palette_definitions(value: Any) -> dict[str, list[tuple[int, int, int]]]:
    if not isinstance(value, dict) or not value:
        raise ContractError("palettes must be a non-empty object")
    palettes: dict[str, list[tuple[int, int, int]]] = {}
    for name in sorted(value):
        if not NAME_PATTERN.fullmatch(name):
            raise ContractError(f"invalid palette name: {name!r}")
        colors = value[name]
        if not isinstance(colors, list) or not colors:
            raise ContractError(f"palette {name} must be a non-empty color array")
        parsed = [hex_to_rgb(color, f"palette {name} color {index}") for index, color in enumerate(colors)]
        if len(set(parsed)) != len(parsed):
            raise ContractError(f"palette {name} contains duplicate colors")
        palettes[name] = parsed
    return palettes


def validate_background_removal(value: Any, label: str) -> None:
    if not isinstance(value, dict) or not isinstance(value.get("mode"), str):
        raise ContractError(f"{label} backgroundRemoval must declare a mode")
    mode = value["mode"]
    if mode == "none":
        require_keys(value, {"mode"}, f"{label} backgroundRemoval")
        return
    if mode == "border_key":
        require_keys(
            value,
            {"mode", "keyColor", "maxChannelDelta", "connectivity", "approvedBy", "reason"},
            f"{label} backgroundRemoval",
        )
        hex_to_rgb(value["keyColor"], f"{label} backgroundRemoval keyColor")
        require_integer(value["maxChannelDelta"], f"{label} backgroundRemoval maxChannelDelta", 0)
        if value["connectivity"] != 4:
            raise ContractError(f"{label} backgroundRemoval connectivity must be 4")
        if not isinstance(value["approvedBy"], str) or not value["approvedBy"].strip():
            raise ContractError(f"{label} border_key requires approvedBy")
        if not isinstance(value["reason"], str) or not value["reason"].strip():
            raise ContractError(f"{label} border_key requires a reason")
        return
    raise ContractError(f"{label} has unsupported backgroundRemoval mode: {mode!r}")


def validate_processing_manifest(
    manifest_path: Path, source_manifest: dict[str, Any], source_groups: dict[str, dict[str, Any]]
) -> tuple[dict[str, Any], dict[str, list[tuple[int, int, int]]]]:
    manifest = load_json(manifest_path)
    root_keys = {
        "schemaVersion",
        "toolVersion",
        "pillowVersion",
        "sourceManifest",
        "sourceManifestSha256",
        "outputRoot",
        "ownedOutputDirectories",
        "palettes",
        "groups",
        "rounding",
        "visualReview",
    }
    require_keys(manifest, root_keys, "processing manifest")
    if manifest["schemaVersion"] != TOOL_SCHEMA_VERSION or manifest["toolVersion"] != TOOL_VERSION:
        raise ContractError("processing manifest schema/tool version is unsupported")
    if manifest["pillowVersion"] != Image.__version__:
        raise ContractError(
            f"Pillow reproducibility mismatch: manifest requires {manifest['pillowVersion']}, running {Image.__version__}"
        )
    source_path = repository_path(manifest["sourceManifest"], "processing manifest sourceManifest")
    if source_path != repository_path(relative_repository_path(Path(source_manifest["_path"])), "source manifest"):
        raise ContractError("processing manifest sourceManifest does not match the supplied source manifest")
    if manifest["sourceManifestSha256"] != sha256_file(source_path):
        raise ContractError("processing manifest sourceManifestSha256 mismatch")
    if manifest["outputRoot"] != "raw-png":
        raise ContractError("processing manifest outputRoot must be raw-png")
    rounding = manifest["rounding"]
    if not isinstance(rounding, dict):
        raise ContractError("processing manifest rounding must be an object")
    require_keys(rounding, {"dimension", "coordinate"}, "processing manifest rounding")
    if rounding != {"dimension": "nearest_half_up", "coordinate": "one_based_nearest_half_up_minus_one"}:
        raise ContractError("processing manifest rounding rule is unsupported")
    review = manifest["visualReview"]
    if not isinstance(review, dict):
        raise ContractError("processing manifest visualReview must be an object")
    require_keys(review, {"reviewer", "result", "checks", "notes"}, "processing manifest visualReview")
    if not isinstance(review["reviewer"], str) or not review["reviewer"].strip():
        raise ContractError("processing manifest visualReview reviewer must be non-empty")
    if review["result"] != "accepted":
        raise ContractError("processing manifest visualReview result must be accepted")
    expected_review_checks = [
        "native_1x",
        "two_x",
        "eight_x_nearest",
        "black",
        "white",
        "magenta",
        "checkerboard",
        "card_background",
        "numeric_animation_order",
    ]
    if review["checks"] != expected_review_checks or not isinstance(review["notes"], str) or not review["notes"].strip():
        raise ContractError("processing manifest visualReview checks or notes are invalid")
    owned = manifest["ownedOutputDirectories"]
    if not isinstance(owned, list) or owned != sorted(FIXED_INVENTORY):
        raise ContractError("ownedOutputDirectories must be exactly the sorted fixed inventory")
    palettes = validate_palette_definitions(manifest["palettes"])
    groups = manifest["groups"]
    if not isinstance(groups, list) or len(groups) != len(FIXED_INVENTORY):
        raise ContractError("processing manifest must contain exactly 17 groups")
    expected_order = sorted(FIXED_INVENTORY)
    for expected_name, group in zip(expected_order, groups):
        validate_processing_group(group, expected_name, source_groups[expected_name], palettes)
    return manifest, palettes


def validate_processing_group(
    group: Any,
    name: str,
    source_group: dict[str, Any],
    palettes: dict[str, list[tuple[int, int, int]]],
) -> None:
    if not isinstance(group, dict):
        raise ContractError(f"processing group {name} must be an object")
    required = {
        "group",
        "sourceGroup",
        "sourceSha256",
        "frameCount",
        "targetSize",
        "backgroundRemoval",
        "alphaThreshold",
        "palette",
        "scale",
        "placement",
        "frames",
    }
    require_keys(group, required, f"processing group {name}")
    count, target = FIXED_INVENTORY[name]
    if group["group"] != name or group["sourceGroup"] != name:
        raise ContractError(f"processing group {name} source/group name mismatch")
    if group["sourceSha256"] != source_group["sha256"]:
        raise ContractError(f"processing group {name} sourceSha256 mismatch")
    if group["frameCount"] != count or tuple(group["targetSize"]) != target:
        raise ContractError(f"processing group {name} does not match fixed frame count or target size")
    validate_background_removal(group["backgroundRemoval"], name)
    threshold = require_integer(group["alphaThreshold"], f"{name} alphaThreshold", 1)
    if threshold > 255:
        raise ContractError(f"{name} alphaThreshold must not exceed 255")
    palette = group["palette"]
    if not isinstance(palette, dict):
        raise ContractError(f"{name} palette must be an object")
    require_keys(palette, {"mode", "name", "metric", "dither", "maxSquaredDistance"}, f"{name} palette")
    if palette["mode"] not in {"validate", "map_nearest"}:
        raise ContractError(f"{name} palette mode is unsupported")
    if palette["name"] not in palettes or palette["metric"] != "srgb_squared" or palette["dither"] is not False:
        raise ContractError(f"{name} palette declaration is invalid")
    require_integer(palette["maxSquaredDistance"], f"{name} palette maxSquaredDistance", 0)
    scale = group["scale"]
    if not isinstance(scale, dict):
        raise ContractError(f"{name} scale must be an object")
    require_keys(scale, {"numerator", "denominator", "filter"}, f"{name} scale")
    numerator = require_integer(scale["numerator"], f"{name} scale numerator", 1)
    denominator = require_integer(scale["denominator"], f"{name} scale denominator", 1)
    if numerator >= denominator or scale["filter"] != "nearest":
        raise ContractError(f"{name} scale must be a downscale using nearest")
    placement = group["placement"]
    if not isinstance(placement, dict):
        raise ContractError(f"{name} placement must be an object")
    require_keys(placement, {"targetAnchorX", "targetGroundY"}, f"{name} placement")
    anchor_x = require_integer(placement["targetAnchorX"], f"{name} targetAnchorX", 0)
    ground_y = require_integer(placement["targetGroundY"], f"{name} targetGroundY", 0)
    if (anchor_x, ground_y) != ((8, 14) if target == (16, 16) else (16, 28)):
        raise ContractError(f"{name} must use its shared reviewed target anchor and ground line")
    frames = group["frames"]
    if not isinstance(frames, list) or len(frames) != count:
        raise ContractError(f"{name} processing frames must contain {count} entries")
    for index, frame in enumerate(frames):
        if not isinstance(frame, dict):
            raise ContractError(f"{name} frame {index} must be an object")
        require_keys(
            frame,
            {"index", "crop", "sourceAnchorX", "sourceGroundY", "targetOffset", "output"},
            f"{name} frame {index}",
        )
        if frame["index"] != index:
            raise ContractError(f"{name} frame indices must be numeric and contiguous from zero")
        left, top, right, bottom = require_quad(frame["crop"], f"{name} frame {index} crop")
        if left < 0 or top < 0 or right <= left or bottom <= top:
            raise ContractError(f"{name} frame {index} has an invalid crop")
        if right > source_group["frameWidth"] or bottom > source_group["frameHeight"]:
            raise ContractError(f"{name} frame {index} crop exceeds its split cell")
        source_anchor_x = require_integer(frame["sourceAnchorX"], f"{name} frame {index} sourceAnchorX", left)
        source_ground_y = require_integer(frame["sourceGroundY"], f"{name} frame {index} sourceGroundY", top)
        if source_anchor_x >= right or source_ground_y >= bottom:
            raise ContractError(f"{name} frame {index} anchor must be inside its crop")
        offset = require_pair(frame["targetOffset"], f"{name} frame {index} targetOffset")
        if not all(isinstance(item, int) for item in offset):
            raise ContractError(f"{name} frame {index} targetOffset must be integers")
        expected_output = f"{name}_{index:02d}.png"
        if frame["output"] != expected_output:
            raise ContractError(f"{name} frame {index} output must be {expected_output}")
        basename = Path(frame["output"]).stem
        if not NAME_PATTERN.fullmatch(basename):
            raise ContractError(f"{name} frame {index} output basename is invalid")


def apply_border_key(image: Image.Image, rule: dict[str, Any]) -> tuple[Image.Image, int]:
    key = hex_to_rgb(rule["keyColor"], "border_key keyColor")
    delta = rule["maxChannelDelta"]
    width, height = image.size
    pixels = image.load()
    queue: deque[tuple[int, int]] = deque()
    visited: set[tuple[int, int]] = set()

    def matches(x: int, y: int) -> bool:
        red, green, blue, _ = pixels[x, y]
        return all(abs(actual - expected) <= delta for actual, expected in zip((red, green, blue), key))

    for x in range(width):
        queue.extend(((x, 0), (x, height - 1)))
    for y in range(1, height - 1):
        queue.extend(((0, y), (width - 1, y)))
    removed = 0
    while queue:
        x, y = queue.popleft()
        if (x, y) in visited or not matches(x, y):
            continue
        visited.add((x, y))
        pixels[x, y] = (0, 0, 0, 0)
        removed += 1
        for neighbor_x, neighbor_y in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
            if 0 <= neighbor_x < width and 0 <= neighbor_y < height:
                queue.append((neighbor_x, neighbor_y))
    return image, removed


def normalize_alpha(image: Image.Image, threshold: int) -> Image.Image:
    data = bytearray(image.tobytes())
    for offset in range(0, len(data), 4):
        if data[offset + 3] < threshold:
            data[offset : offset + 4] = b"\x00\x00\x00\x00"
        else:
            data[offset + 3] = 255
    return Image.frombytes("RGBA", image.size, bytes(data))


def nearest_palette_color(
    color: tuple[int, int, int], palette: list[tuple[int, int, int]]
) -> tuple[tuple[int, int, int], int]:
    best = palette[0]
    best_distance = sum((actual - expected) ** 2 for actual, expected in zip(color, best))
    for candidate in palette[1:]:
        distance = sum((actual - expected) ** 2 for actual, expected in zip(color, candidate))
        if distance < best_distance:
            best, best_distance = candidate, distance
    return best, best_distance


def apply_palette(
    image: Image.Image, policy: dict[str, Any], palette: list[tuple[int, int, int]], label: str
) -> tuple[Image.Image, int, int]:
    data = bytearray(image.tobytes())
    changed = 0
    maximum_distance = 0
    for offset in range(0, len(data), 4):
        if data[offset + 3] != 255:
            continue
        original = (data[offset], data[offset + 1], data[offset + 2])
        mapped, distance = nearest_palette_color(original, palette)
        maximum_distance = max(maximum_distance, distance)
        if policy["mode"] == "validate":
            if original not in palette:
                raise ImageValidationError(f"{label} has a visible color outside palette {policy['name']}: {rgb_to_hex(original)}")
        elif original != mapped:
            data[offset : offset + 3] = bytes(mapped)
            changed += 1
    if maximum_distance > policy["maxSquaredDistance"]:
        raise ImageValidationError(
            f"{label} palette mapping distance {maximum_distance} exceeds reviewed maximum {policy['maxSquaredDistance']}"
        )
    return Image.frombytes("RGBA", image.size, bytes(data)), changed, maximum_distance


def require_crop_keeps_visible_pixels(image: Image.Image, crop: tuple[int, int, int, int], label: str) -> None:
    bbox = alpha_bbox(image)
    if bbox is None:
        raise ImageValidationError(f"{label} is empty after alpha normalization")
    left, top, right, bottom = crop
    if bbox[0] < left or bbox[1] < top or bbox[2] > right or bbox[3] > bottom:
        raise ImageValidationError(f"{label} crop would discard visible pixels")


def validate_final_image(
    image: Image.Image,
    target_size: tuple[int, int],
    palette: list[tuple[int, int, int]],
    target_ground_y: int,
    label: str,
) -> tuple[int, int, int, int]:
    if image.mode != "RGBA" or image.size != target_size:
        raise ImageValidationError(f"{label} must be an RGBA {target_size[0]}x{target_size[1]} image")
    values = alpha_values(image)
    if values != [0, 255]:
        raise ImageValidationError(f"{label} alpha values must be exactly [0, 255], got {values}")
    for red, green, blue, alpha in image.get_flattened_data():
        if alpha == 0 and (red, green, blue) != (0, 0, 0):
            raise ImageValidationError(f"{label} has transparent-edge RGB residue")
        if alpha == 255 and (red, green, blue) not in palette:
            raise ImageValidationError(f"{label} has a visible color outside its approved palette")
    bbox = alpha_bbox(image)
    if bbox is None:
        raise ImageValidationError(f"{label} has no opaque pixels")
    width, height = target_size
    if bbox[0] == 0 or bbox[1] == 0 or bbox[2] == width or bbox[3] == height:
        raise ImageValidationError(f"{label} opaque bounds touch a forbidden canvas edge: {bbox}")
    if bbox[3] != target_ground_y + 1:
        raise ImageValidationError(
            f"{label} opaque bottom {bbox[3] - 1} does not match declared ground line {target_ground_y}"
        )
    return bbox


def render_group(
    group: dict[str, Any], source_frames: list[Image.Image], palettes: dict[str, list[tuple[int, int, int]]]
) -> list[RenderedFrame]:
    name = group["group"]
    target_size = tuple(group["targetSize"])
    palette = palettes[group["palette"]["name"]]
    scale = group["scale"]
    numerator, denominator = scale["numerator"], scale["denominator"]
    placement = group["placement"]
    result: list[RenderedFrame] = []
    for frame in group["frames"]:
        index = frame["index"]
        label = f"{name} frame {index}"
        image = source_frames[index].copy()
        background = group["backgroundRemoval"]
        if background["mode"] == "border_key":
            image, removed = apply_border_key(image, background)
        else:
            removed = 0
        image = normalize_alpha(image, group["alphaThreshold"])
        image, changed, max_distance = apply_palette(image, group["palette"], palette, label)
        crop = require_quad(frame["crop"], f"{label} crop")
        require_crop_keeps_visible_pixels(image, crop, label)
        cropped = image.crop(crop)
        resized_size = (
            rounded_scaled_dimension(cropped.width, numerator, denominator),
            rounded_scaled_dimension(cropped.height, numerator, denominator),
        )
        if 0 in resized_size:
            raise ImageValidationError(f"{label} scale produces an empty image")
        resized = cropped.resize(resized_size, Image.Resampling.NEAREST)
        source_anchor_x = frame["sourceAnchorX"] - crop[0]
        source_ground_y = frame["sourceGroundY"] - crop[1]
        scaled_anchor_x = scaled_pixel_coordinate(source_anchor_x, numerator, denominator)
        scaled_ground_y = scaled_pixel_coordinate(source_ground_y, numerator, denominator)
        if not (0 <= scaled_anchor_x < resized.width and 0 <= scaled_ground_y < resized.height):
            raise ImageValidationError(f"{label} scaled anatomical anchor falls outside the resized crop")
        offset_x, offset_y = frame["targetOffset"]
        origin_x = placement["targetAnchorX"] + offset_x - scaled_anchor_x
        origin_y = placement["targetGroundY"] + offset_y - scaled_ground_y
        if (
            origin_x < 0
            or origin_y < 0
            or origin_x + resized.width > target_size[0]
            or origin_y + resized.height > target_size[1]
        ):
            raise ImageValidationError(f"{label} would clip after reviewed placement")
        canvas = Image.new("RGBA", target_size, (0, 0, 0, 0))
        canvas.paste(resized, (origin_x, origin_y))
        validate_final_image(canvas, target_size, palette, placement["targetGroundY"] + offset_y, label)
        if removed:
            print(f"{label}: border_key removed {removed} pixels")
        print(f"{label}: palette changed {changed} pixels; max squared distance {max_distance}")
        result.append(
            RenderedFrame(
                group=name,
                index=index,
                basename=frame["output"],
                relative_path=f"{name}/{frame['output']}",
                image=canvas,
                palette_name=group["palette"]["name"],
                target_anchor_x=placement["targetAnchorX"] + offset_x,
                target_ground_y=placement["targetGroundY"] + offset_y,
                colors_changed=changed,
                max_mapping_distance=max_distance,
            )
        )
    return result


def save_canonical_png(image: Image.Image, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path, format="PNG", optimize=False, compress_level=PNG_COMPRESSION_LEVEL)


def validate_expected_frame_set(frames: list[RenderedFrame]) -> None:
    if len(frames) != 45:
        raise ImageValidationError(f"rendered frame count must be 45, got {len(frames)}")
    names = [frame.basename for frame in frames]
    if len(set(names)) != len(names):
        raise ImageValidationError("rendered output basenames are not globally unique")
    count_32 = sum(frame.image.size == (32, 32) for frame in frames)
    count_16 = sum(frame.image.size == (16, 16) for frame in frames)
    if (count_32, count_16) != (44, 1):
        raise ImageValidationError(f"rendered dimensions must be 44 32x32 and one 16x16, got {count_32} and {count_16}")
    total_bytes = sum(frame.image.width * frame.image.height * 4 for frame in frames)
    if total_bytes != 181248:
        raise ImageValidationError(f"rendered ARGB8888 estimate must be 181248 bytes, got {total_bytes}")


def owned_tree_state(output_root: Path, owned: list[str], expected_relative_paths: set[str]) -> tuple[list[Path], list[Path]]:
    stale_pngs: list[Path] = []
    unexpected_non_png: list[Path] = []
    for name in owned:
        directory = output_root / name
        if not directory.exists():
            continue
        if not directory.is_dir():
            unexpected_non_png.append(directory)
            continue
        for path in sorted(directory.rglob("*")):
            if path.is_dir():
                continue
            relative = path.relative_to(output_root).as_posix()
            if path.suffix.lower() != ".png":
                unexpected_non_png.append(path)
            elif path.parent != directory or relative not in expected_relative_paths:
                stale_pngs.append(path)
    return stale_pngs, unexpected_non_png


def validate_tamagotchi_directory_set(output_root: Path, owned: list[str]) -> None:
    if not output_root.exists():
        return
    unexpected = sorted(
        path.name
        for path in output_root.iterdir()
        if path.is_dir() and path.name.startswith("tamagotchi_") and path.name not in owned
    )
    if unexpected:
        raise OutputDriftError(
            "unexpected Tamagotchi output directories outside the exact ownership list: " + ", ".join(unexpected)
        )


def validate_global_names(output_root: Path, owned: list[str], frames: list[RenderedFrame]) -> None:
    existing: list[tuple[str, str]] = []
    owned_set = set(owned)
    if output_root.exists():
        for path in sorted(output_root.rglob("*.png")):
            relative = path.relative_to(output_root)
            if relative.parts and relative.parts[0] in owned_set:
                continue
            existing.append((path.stem, relative.as_posix()))
    expected = [(frame.basename.removesuffix(".png"), frame.relative_path) for frame in frames]
    identifiers: dict[str, str] = {}
    basenames: dict[str, str] = {}
    for basename, path in sorted(existing + expected, key=lambda item: item[1]):
        if basename in basenames:
            raise ImageValidationError(f"duplicate raw PNG basename {basename!r}: {basenames[basename]} and {path}")
        basenames[basename] = path
        identifier = basename.replace("-", "_").replace(".", "_")
        if identifier in identifiers:
            raise ImageValidationError(
                f"png2c identifier collision {identifier!r}: {identifiers[identifier]} and {path}"
            )
        identifiers[identifier] = path


def write_staged_tree(frames: list[RenderedFrame], stage_root: Path) -> None:
    for frame in frames:
        save_canonical_png(frame.image, stage_root / frame.relative_path)


def frame_report(
    frame: RenderedFrame, output_path: Path, palette: list[tuple[int, int, int]]
) -> dict[str, Any]:
    with Image.open(output_path) as opened:
        if opened.info:
            raise ImageValidationError(f"{frame.relative_path} PNG metadata must be stripped")
        image = opened.convert("RGBA")
    bbox = validate_final_image(image, image.size, palette, frame.target_ground_y, frame.relative_path)
    colors = visible_colors(image, palette)
    return {
        "alphaValues": alpha_values(image),
        "basename": frame.basename,
        "byteSize": output_path.stat().st_size,
        "dimensions": [image.width, image.height],
        "estimatedArgb8888Bytes": image.width * image.height * 4,
        "fileSha256": sha256_file(output_path),
        "index": frame.index,
        "mode": image.mode,
        "opaqueBoundingBox": {
            "bottom": bbox[3],
            "height": bbox[3] - bbox[1],
            "left": bbox[0],
            "right": bbox[2],
            "top": bbox[1],
            "width": bbox[2] - bbox[0],
        },
        "png2cIdentifier": Path(frame.basename).stem.replace("-", "_").replace(".", "_"),
        "rawRgbaPixelSha256": sha256_bytes(image.tobytes()),
        "runtimePath": f"raw-png/{frame.relative_path}",
        "targetAnchorX": frame.target_anchor_x,
        "targetGroundY": frame.target_ground_y,
        "visibleColorCount": len(colors),
        "visibleColors": [rgb_to_hex(color) for color in colors],
    }


def build_runtime_report(
    manifest: dict[str, Any],
    source_groups: dict[str, dict[str, Any]],
    frames: list[RenderedFrame],
    staged_root: Path,
    palettes: dict[str, list[tuple[int, int, int]]],
    stale_basenames: list[str],
) -> dict[str, Any]:
    by_group: dict[str, list[RenderedFrame]] = {name: [] for name in sorted(FIXED_INVENTORY)}
    for frame in frames:
        by_group[frame.group].append(frame)
    groups_report: list[dict[str, Any]] = []
    for name in sorted(by_group):
        group_frames = sorted(by_group[name], key=lambda item: item.index)
        source = source_groups[name]
        details = [
            frame_report(frame, staged_root / frame.relative_path, palettes[frame.palette_name]) for frame in group_frames
        ]
        groups_report.append(
            {
                "frameCount": len(group_frames),
                "frames": details,
                "group": name,
                "loopMode": source["playback"],
                "runtimePaths": [detail["runtimePath"] for detail in details],
                "sourceStripPath": source["source"],
                "sourceStripSha256": source["sha256"],
                "targetSize": list(FIXED_INVENTORY[name][1]),
            }
        )
    bytes_total = sum(
        frame.image.width * frame.image.height * 4
        for frame in frames
    )
    return {
        "groups": groups_report,
        "pillowVersion": Image.__version__,
        "processingManifestSha256": sha256_file(Path(manifest["_path"])),
        "schemaVersion": TOOL_SCHEMA_VERSION,
        "staleRuntimeBasenamesPruned": sorted(stale_basenames),
        "toolVersion": TOOL_VERSION,
        "totals": {
            "descriptorCount": len(frames),
            "estimatedArgb8888Bytes": bytes_total,
            "fileCount": len(frames),
            "frame16x16Count": sum(frame.image.size == (16, 16) for frame in frames),
            "frame32x32Count": sum(frame.image.size == (32, 32) for frame in frames),
            "groupCount": len(groups_report),
        },
    }


def promote_staged_tree(
    stage_root: Path,
    output_root: Path,
    owned: list[str],
    report_bytes: bytes,
    report_path: Path,
) -> None:
    backup_root = stage_root / "backup"
    backup_root.mkdir()
    promoted: list[str] = []
    moved_existing: list[str] = []
    try:
        for name in owned:
            target = output_root / name
            staged = stage_root / name
            backup = backup_root / name
            if target.exists():
                os.replace(target, backup)
                moved_existing.append(name)
            os.replace(staged, target)
            promoted.append(name)
        report_path.parent.mkdir(parents=True, exist_ok=True)
        staged_report = stage_root / "runtime-sprite-manifest.json"
        staged_report.write_bytes(report_bytes)
        os.replace(staged_report, report_path)
    except OSError as error:
        # Roll back every directory that was promoted before surfacing the error.
        for name in reversed(promoted):
            target = output_root / name
            failed = stage_root / "failed" / name
            failed.parent.mkdir(parents=True, exist_ok=True)
            if target.exists():
                os.replace(target, failed)
        for name in reversed(moved_existing):
            backup = backup_root / name
            target = output_root / name
            if backup.exists():
                os.replace(backup, target)
        raise OutputDriftError(f"unable to promote staged sprite tree transactionally: {error}") from error


def render_expected(
    manifest_path: Path, source_path: Path
) -> tuple[
    dict[str, Any],
    dict[str, dict[str, Any]],
    dict[str, list[tuple[int, int, int]]],
    list[RenderedFrame],
]:
    source_manifest, source_groups, source_frames = load_and_validate_source_manifest(source_path)
    source_manifest["_path"] = source_path
    manifest, palettes = validate_processing_manifest(manifest_path, source_manifest, source_groups)
    manifest["_path"] = manifest_path
    frames: list[RenderedFrame] = []
    processing_by_name = {group["group"]: group for group in manifest["groups"]}
    for name in sorted(FIXED_INVENTORY):
        frames.extend(render_group(processing_by_name[name], source_frames[name], palettes))
    validate_expected_frame_set(frames)
    return manifest, source_groups, palettes, frames


def inspect_command(source_path: Path) -> int:
    source_manifest, source_groups, source_frames = load_and_validate_source_manifest(source_path)
    print(f"Tamagotchi sprite processor schema {TOOL_SCHEMA_VERSION}, tool {TOOL_VERSION}")
    print(f"Pillow {Image.__version__}")
    print(f"Validated phase-3 binding and {len(source_groups)} phase-4 strips.")
    for name in sorted(source_groups):
        group = source_groups[name]
        print(f"{name}: {group['source']} {group['width']}x{group['height']}, {group['frameCount']} frames")
        for index, frame in enumerate(source_frames[name]):
            values = alpha_values(frame)
            bbox = alpha_bbox(frame)
            thresholded_bbox = alpha_bbox(normalize_alpha(frame, 128))
            colors = visible_colors(frame)
            edge_touching = bbox is not None and (
                bbox[0] == 0 or bbox[1] == 0 or bbox[2] == frame.width or bbox[3] == frame.height
            )
            print(
                f"  frame {index:02d}: alpha={values[0]}..{values[-1]} ({len(values)} values), "
                f"bbox={bbox}, threshold128Bbox={thresholded_bbox}, "
                f"visibleColors={len(colors)}, touchesCellEdge={edge_touching}"
            )
        print("  requires reviewed manifest decisions: background removal, alpha threshold, palette, crop, scale, anchors")
    return 0


def build_command(manifest_path: Path, source_path: Path, output_root: Path, report_path: Path, prune_owned: bool) -> int:
    manifest, source_groups, palettes, frames = render_expected(manifest_path, source_path)
    if relative_repository_path(output_root) != manifest["outputRoot"]:
        raise ContractError("--output-root does not match processing manifest outputRoot")
    if relative_repository_path(report_path) != "_ai/art/tamagotchi/runtime-sprite-manifest.json":
        raise ContractError("--report must be _ai/art/tamagotchi/runtime-sprite-manifest.json")
    expected_paths = {frame.relative_path for frame in frames}
    owned = manifest["ownedOutputDirectories"]
    validate_tamagotchi_directory_set(output_root, owned)
    stale_pngs, unexpected_non_png = owned_tree_state(output_root, owned, expected_paths)
    if unexpected_non_png:
        paths = ", ".join(display_path(path) for path in unexpected_non_png)
        raise OutputDriftError(f"unexpected non-PNG file in owned sprite directory: {paths}")
    if stale_pngs and not prune_owned:
        paths = ", ".join(display_path(path) for path in stale_pngs)
        raise OutputDriftError(f"stale owned PNGs require reviewed --prune-owned: {paths}")
    validate_global_names(output_root, owned, frames)
    stale_basenames = [path.name for path in stale_pngs]
    output_root.mkdir(parents=True, exist_ok=True)
    stage_root = Path(tempfile.mkdtemp(prefix=".tamagotchi-stage-", dir=output_root.parent))
    try:
        write_staged_tree(frames, stage_root)
        staged_stale, staged_non_png = owned_tree_state(stage_root, owned, expected_paths)
        if staged_stale or staged_non_png:
            raise ImageValidationError("internal staged tree validation failed")
        report = build_runtime_report(manifest, source_groups, frames, stage_root, palettes, stale_basenames)
        report_bytes = stable_json_bytes(report)
        promote_staged_tree(stage_root, output_root, owned, report_bytes, report_path)
    finally:
        shutil.rmtree(stage_root, ignore_errors=True)
    print(
        f"Built 17 groups, 45 PNGs (44 at 32x32, one at 16x16), "
        "181248 ARGB8888 bytes."
    )
    if stale_basenames:
        print("Pruned stale owned runtime basenames: " + ", ".join(sorted(stale_basenames)))
    return 0


def check_command(manifest_path: Path, source_path: Path, output_root: Path, report_path: Path) -> int:
    manifest, source_groups, palettes, frames = render_expected(manifest_path, source_path)
    if relative_repository_path(output_root) != manifest["outputRoot"]:
        raise ContractError("--output-root does not match processing manifest outputRoot")
    if relative_repository_path(report_path) != "_ai/art/tamagotchi/runtime-sprite-manifest.json":
        raise ContractError("--report must be _ai/art/tamagotchi/runtime-sprite-manifest.json")
    expected_paths = {frame.relative_path for frame in frames}
    owned = manifest["ownedOutputDirectories"]
    validate_tamagotchi_directory_set(output_root, owned)
    stale_pngs, unexpected_non_png = owned_tree_state(output_root, owned, expected_paths)
    if stale_pngs or unexpected_non_png:
        paths = [display_path(path) for path in stale_pngs + unexpected_non_png]
        raise OutputDriftError("owned output tree has stale or unexpected files: " + ", ".join(paths))
    validate_global_names(output_root, owned, frames)
    stage_root = Path(tempfile.mkdtemp(prefix=".tamagotchi-check-", dir=output_root.parent))
    try:
        write_staged_tree(frames, stage_root)
        for frame in frames:
            expected = stage_root / frame.relative_path
            actual = output_root / frame.relative_path
            if not actual.is_file():
                raise OutputDriftError(f"missing expected owned PNG: {display_path(actual)}")
            if expected.read_bytes() != actual.read_bytes():
                raise OutputDriftError(f"PNG bytes differ from deterministic render: {display_path(actual)}")
        try:
            actual_report = load_json(report_path)
            prior_stale = actual_report["staleRuntimeBasenamesPruned"]
        except (ContractError, KeyError, TypeError) as error:
            raise OutputDriftError(f"cannot validate runtime report: {error}") from error
        if not isinstance(prior_stale, list) or any(not isinstance(item, str) for item in prior_stale):
            raise OutputDriftError("runtime report has invalid staleRuntimeBasenamesPruned")
        expected_report = stable_json_bytes(
            build_runtime_report(manifest, source_groups, frames, stage_root, palettes, sorted(prior_stale))
        )
        if not report_path.is_file() or expected_report != report_path.read_bytes():
            raise OutputDriftError(f"runtime report differs from deterministic render: {display_path(report_path)}")
    finally:
        shutil.rmtree(stage_root, ignore_errors=True)
    print("Check passed: 17 groups, 45 PNGs, 44 at 32x32, one at 16x16, 181248 ARGB8888 bytes.")
    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    inspect = subparsers.add_parser("inspect", help="validate the accepted Phase-4 source handoff read-only")
    inspect.add_argument("--source-manifest", required=True, type=Path)
    for command in ("build", "check"):
        child = subparsers.add_parser(command)
        child.add_argument("--manifest", required=True, type=Path)
        child.add_argument("--output-root", required=True, type=Path)
        child.add_argument("--report", required=True, type=Path)
    subparsers.choices["build"].add_argument("--prune-owned", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        if args.command == "inspect":
            return inspect_command(args.source_manifest.resolve())
        source_path: Path | None = None
        if args.command in {"build", "check"}:
            # The processing manifest binds this path, but source validation is
            # intentionally performed before any output mutation.
            tentative = load_json(args.manifest.resolve())
            source_path = repository_path(tentative.get("sourceManifest"), "processing manifest sourceManifest")
        if args.command == "build":
            return build_command(
                args.manifest.resolve(), source_path, args.output_root.resolve(), args.report.resolve(), args.prune_owned
            )
        return check_command(args.manifest.resolve(), source_path, args.output_root.resolve(), args.report.resolve())
    except ContractError as error:
        print(f"CONTRACT ERROR: {error}", file=sys.stderr)
        return 2
    except ImageValidationError as error:
        print(f"IMAGE VALIDATION ERROR: {error}", file=sys.stderr)
        return 3
    except OutputDriftError as error:
        print(f"OUTPUT DRIFT ERROR: {error}", file=sys.stderr)
        return 4


if __name__ == "__main__":
    sys.exit(main())
