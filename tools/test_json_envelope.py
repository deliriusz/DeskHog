#!/usr/bin/env python3
"""Host regression tests for mutable ArduinoJson card-request parsing."""

from __future__ import annotations

from pathlib import Path
import shutil
import subprocess
import tempfile
import textwrap
import unittest


REPOSITORY_ROOT = Path(__file__).resolve().parent.parent

HARNESS_SOURCE = r"""
#include <ArduinoJson.h>
#include <cstdint>
#include <cstring>

#include "src/config/JsonEnvelope.h"

namespace {

template <size_t Size>
bool expectResult(const char (&json)[Size], JsonArrayParseResult expected,
                  size_t expectedEntries = 0) {
    uint8_t mutableJson[Size];
    std::memcpy(mutableJson, json, Size);

    DynamicJsonDocument document(2048);
    const JsonArrayParseResult result = parseMutableJsonArray(
        document, mutableJson, Size - 1);
    if (result != expected) {
        return false;
    }

    if (result == JsonArrayParseResult::Success &&
        document.as<JsonArray>().size() != expectedEntries) {
        return false;
    }

    return true;
}

} // namespace

int main() {
    const char sixCards[] =
        "[{\"type\":\"FLAPPY_HOG\",\"config\":\"\",\"order\":0,\"name\":\"Flappy Hog\"},"
        "{\"type\":\"QUESTION\",\"config\":\"\",\"order\":1,\"name\":\"Question Card\"},"
        "{\"type\":\"PADDLE\",\"config\":\"\",\"order\":2,\"name\":\"Paddle\"},"
        "{\"type\":\"FRIEND\",\"config\":\"\",\"order\":3,\"name\":\"Friend card\"},"
        "{\"type\":\"INSIGHT\",\"config\":\"1\",\"order\":4,\"name\":\"PostHog insight\"},"
        "{\"type\":\"TAMAGOTCHI\",\"config\":\"\",\"name\":\"Tamagotchi\",\"order\":5}]";

    if (!expectResult(sixCards, JsonArrayParseResult::Success, 6)) {
        return 1;
    }
    if (!expectResult("[] trailing", JsonArrayParseResult::InvalidJson)) {
        return 2;
    }
    if (!expectResult("{\"type\":\"FRIEND\"}", JsonArrayParseResult::InvalidRoot)) {
        return 3;
    }
    if (!expectResult("[{]", JsonArrayParseResult::InvalidJson)) {
        return 4;
    }

    return 0;
}
"""


class JsonEnvelopeTests(unittest.TestCase):
    def test_mutable_array_parser_preserves_original_envelope(self) -> None:
        compiler = shutil.which("c++")
        self.assertIsNotNone(compiler, "A host C++ compiler is required")

        dependency_roots = sorted(
            (REPOSITORY_ROOT / ".pio/libdeps").glob("*/ArduinoJson/src")
        )
        self.assertTrue(
            dependency_roots,
            "ArduinoJson is unavailable; run the configured PlatformIO build first",
        )

        with tempfile.TemporaryDirectory() as temporary_directory:
            temporary_path = Path(temporary_directory)
            source_path = temporary_path / "json_envelope_regression.cpp"
            binary_path = temporary_path / "json_envelope_regression"
            source_path.write_text(textwrap.dedent(HARNESS_SOURCE), encoding="utf-8")

            compile_result = subprocess.run(
                [
                    compiler,
                    "-std=c++17",
                    "-I",
                    str(REPOSITORY_ROOT),
                    "-I",
                    str(dependency_roots[0]),
                    str(source_path),
                    "-o",
                    str(binary_path),
                ],
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(
                compile_result.returncode,
                0,
                compile_result.stdout + compile_result.stderr,
            )

            run_result = subprocess.run(
                [str(binary_path)],
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(
                run_result.returncode,
                0,
                run_result.stdout + run_result.stderr,
            )


if __name__ == "__main__":
    unittest.main(verbosity=2)
