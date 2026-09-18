#!/usr/bin/env python3
"""Host regression tests for the PlatformIO font generator entry point."""

from __future__ import annotations

import os
from pathlib import Path
import shutil
import stat
import subprocess
import sys
import tempfile
import textwrap
import unittest


REPOSITORY_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPOSITORY_ROOT / "ttf2c.py"
FONT_NAMES = (
    "font_label",
    "font_value",
    "font_value_large",
    "font_loud_noises",
)


class Ttf2cGeneratorTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.project_dir = Path(self.temp_dir.name) / "project"
        self.project_dir.mkdir()
        typography_dir = self.project_dir / "typography"
        typography_dir.mkdir()
        for font_file in (
            "Inter_18pt-Regular.ttf",
            "Inter_18pt-SemiBold.ttf",
            "LoudNoises.ttf",
        ):
            (typography_dir / font_file).write_bytes(b"fixture font input")

        self.bin_dir = self.project_dir / "fake-bin"
        self.bin_dir.mkdir()
        self._write_executable("npm", "#!/bin/sh\nexit 0\n")
        self._write_executable(
            "npx",
            """#!/usr/bin/env python3
import os
from pathlib import Path
import sys

font_name = sys.argv[sys.argv.index("--lv-font-name") + 1]
if os.environ.get("FAKE_NPX_FAIL") == font_name:
    print(f"forced conversion failure for {font_name}", file=sys.stderr)
    raise SystemExit(7)

font_arg = sys.argv[sys.argv.index("--font") + 1]
output_arg = sys.argv[sys.argv.index("--output") + 1]
output_path = Path(output_arg)
output_path.parent.mkdir(parents=True, exist_ok=True)
output_path.write_text(
    f"/* {' '.join(sys.argv[1:])} */\\n"
    '#include "lvgl/lvgl.h"\\n',
    encoding="utf-8",
)
""",
        )

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def _write_executable(self, name: str, content: str) -> None:
        path = self.bin_dir / name
        path.write_text(content, encoding="utf-8")
        path.chmod(path.stat().st_mode | stat.S_IXUSR)

    def _environment(self, **extra: str) -> dict[str, str]:
        environment = os.environ.copy()
        environment["PATH"] = f"{self.bin_dir}{os.pathsep}{environment['PATH']}"
        environment.update(extra)
        return environment

    def _run_in_platformio_import_context(self) -> subprocess.CompletedProcess[str]:
        runner = textwrap.dedent(
            f"""
            import pathlib

            namespace = {{"__name__": "ttf2c_platformio"}}

            class FakeEnv:
                def subst(self, value):
                    assert value == "$PROJECT_DIR"
                    return {str(self.project_dir)!r}

            def Import(name):
                assert name == "env"
                namespace["env"] = FakeEnv()

            namespace["Import"] = Import
            script_path = pathlib.Path({str(SCRIPT_PATH)!r})
            exec(compile(script_path.read_text(), str(script_path), "exec"), namespace)
            """
        )
        return subprocess.run(
            [sys.executable, "-c", runner],
            cwd=self.project_dir.parent,
            env=self._environment(),
            text=True,
            capture_output=True,
            check=False,
        )

    def _run_directly(self, **extra_environment: str) -> subprocess.CompletedProcess[str]:
        script_copy = self.project_dir / "ttf2c.py"
        shutil.copy2(SCRIPT_PATH, script_copy)
        return subprocess.run(
            [sys.executable, str(script_copy)],
            cwd=self.project_dir.parent,
            env=self._environment(**extra_environment),
            text=True,
            capture_output=True,
            check=False,
        )

    def test_platformio_import_runs_all_font_conversions(self) -> None:
        result = self._run_in_platformio_import_context()

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Successfully processed 4 of 4 fonts", result.stdout)
        self.assertIn("All fonts were successfully converted to LVGL format!", result.stdout)
        expected_options = {
            "font_label": (
                "--font typography/Inter_18pt-Regular.ttf",
                "--range 0x20-0x7F,0xA0-0xFF",
                "--size 15",
                "--output include/fonts/font_label.c",
            ),
            "font_value": (
                "--font typography/Inter_18pt-SemiBold.ttf",
                "--range 0x20-0x7F,0xA0-0xFF",
                "--size 16",
                "--output include/fonts/font_value.c",
            ),
            "font_value_large": (
                "--font typography/Inter_18pt-SemiBold.ttf",
                "--range 0x20-0x7F,0xA0-0xFF",
                "--size 36",
                "--output include/fonts/font_value_large.c",
            ),
            "font_loud_noises": (
                "--font typography/LoudNoises.ttf",
                "--range 0x20-0x7F",
                "--size 20",
                "--output include/fonts/font_loud_noises.c",
            ),
        }
        for font_name in FONT_NAMES:
            self.assertTrue((self.project_dir / "include/fonts" / f"{font_name}.c").exists())
            self.assertTrue((self.project_dir / "include/fonts" / f"{font_name}.h").exists())
            generated_font = (self.project_dir / "include/fonts" / f"{font_name}.c").read_text()
            for option in expected_options[font_name] + ("--format lvgl", "--bpp 4", "--no-compress"):
                self.assertIn(option, generated_font)
        self.assertTrue((self.project_dir / "include/fonts/fonts.h").exists())
        generated_label = (self.project_dir / "include/fonts/font_label.c").read_text()
        self.assertIn('#include "lvgl.h"', generated_label)
        self.assertNotIn(str(self.project_dir), generated_label)

    def test_direct_invocation_uses_script_project_directory(self) -> None:
        result = self._run_directly()

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Successfully processed 4 of 4 fonts", result.stdout)
        self.assertTrue((self.project_dir / "include/fonts/fonts.h").exists())

    def test_missing_font_fails_instead_of_accepting_partial_output(self) -> None:
        (self.project_dir / "typography/LoudNoises.ttf").unlink()

        result = self._run_in_platformio_import_context()

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("LoudNoises.ttf", result.stdout + result.stderr)

    def test_failed_font_conversion_fails_the_generator(self) -> None:
        result = self._run_directly(FAKE_NPX_FAIL="font_value_large")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("font_value_large", result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main(verbosity=2)
