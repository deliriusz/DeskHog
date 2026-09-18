#!/usr/bin/env python3
import os
import re
import subprocess
from pathlib import Path

def check_npm_installed():
    """Check if npm is installed"""
    try:
        subprocess.check_call(["npm", "--version"], 
                             stdout=subprocess.DEVNULL, 
                             stderr=subprocess.DEVNULL)
        return True
    except (subprocess.SubprocessError, FileNotFoundError):
        return False

def convert_font(ttf_file, output_dir, size, font_name_override=None, working_dir=None):
    """Convert TTF file to LVGL compatible C font"""
    font_name = font_name_override if font_name_override else safe_font_name(ttf_file)
    output_dir = Path(output_dir)
    output_c_file = output_dir / f"{font_name}.c"
    command_ttf_file = os.path.relpath(ttf_file, working_dir) if working_dir else os.fspath(ttf_file)
    command_output_file = os.path.relpath(output_c_file, working_dir) if working_dir else os.fspath(output_c_file)
    
    # Define character range (0-255, basic Latin)
    # Use a simplified range for LoudNoises font which may have limited glyphs
    if "LoudNoises" in str(ttf_file) or font_name == "font_loud_noises":
        range_arg = "0x20-0x7F"  # Basic Latin only
    else:
        range_arg = "0x20-0x7F,0xA0-0xFF"  # Full Latin
    
    # Build the lv_font_conv command with appropriate parameters
    cmd = [
        "npx", "lv_font_conv",
        "--font", command_ttf_file,
        "--range", range_arg,
        "--size", str(size),  # Use the provided size
        "--format", "lvgl",
        "--bpp", "4",  # 4 bits per pixel for grayscale
        "--no-compress",
        "--output", command_output_file,
        "--lv-font-name", font_name  # Correct param for LVGL font name
    ]
    
    try:
        print(f"Converting {ttf_file} at {size}pt as {font_name}...")
        subprocess.check_call(cmd, cwd=working_dir)

        if output_c_file.exists():
            # Fix the include path in the generated file
            with output_c_file.open('r', encoding='utf-8') as f:
                content = f.read()
            
            # Replace the include path
            content = content.replace('#include "lvgl/lvgl.h"', '#include "lvgl.h"')
            
            with output_c_file.open('w', encoding='utf-8') as f:
                f.write(content)
            
            # Also create a header file for inclusion
            header_file = output_dir / f"{font_name}.h"
            with header_file.open('w', encoding='utf-8') as f:
                header_content = f"""/**
 * @file {font_name}.h
 * @brief LVGL font generated from {os.path.basename(ttf_file)} at {size}pt
 */

#pragma once

#ifdef __cplusplus
extern "C" {{
#endif

#include "lvgl.h"

extern const lv_font_t {font_name};

#ifdef __cplusplus
}}
#endif
"""
                f.write(header_content)
            print(f"Generated {output_c_file} and {header_file}")
            return True
        else:
            print(f"Failed to generate {output_c_file}")
            return False
    except subprocess.SubprocessError as e:
        print(f"Error converting font: {e}")
        return False

def safe_font_name(filename):
    """Convert filename to safe C variable name"""
    # Remove file extension and path
    name = os.path.splitext(os.path.basename(filename))[0]
    # Replace non-alphanumeric with underscore
    name = re.sub(r'[^a-zA-Z0-9]', '_', name)
    return name

def main(project_dir=None):
    print("TTF to LVGL Font Converter")
    print("==========================")

    project_dir = Path(project_dir) if project_dir else Path(__file__).resolve().parent
    project_dir = project_dir.resolve()
    
    # Check if npm is installed
    if not check_npm_installed():
        raise RuntimeError("npm is required but not installed. Please install Node.js and npm first.")
    
    # Install lv_font_conv if not already installed
    print("Installing lv_font_conv (if not already installed)...")
    try:
        subprocess.check_call(
            ["npm", "install", "--no-save", "lv_font_conv"],
            cwd=project_dir,
        )
    except subprocess.SubprocessError as e:
        raise RuntimeError(
            "Failed to install lv_font_conv. Please install manually with: "
            "npm install -g lv_font_conv"
        ) from e
    
    # Create output directory if it doesn't exist
    output_dir = project_dir / "include/fonts"
    os.makedirs(output_dir, exist_ok=True)
    
    # Define the fonts to create
    font_configs = [
        {"name": "font_label", "file": "typography/Inter_18pt-Regular.ttf", "size": 15},
        {"name": "font_value", "file": "typography/Inter_18pt-SemiBold.ttf", "size": 16},
        {"name": "font_value_large", "file": "typography/Inter_18pt-SemiBold.ttf", "size": 36},
        {"name": "font_loud_noises", "file": "typography/LoudNoises.ttf", "size": 20}
    ]

    missing_files = [
        project_dir / config["file"]
        for config in font_configs
        if not (project_dir / config["file"]).is_file()
    ]
    if missing_files:
        for missing_file in missing_files:
            print(f"Font file not found: {missing_file}")
        raise RuntimeError(f"Missing {len(missing_files)} required font file(s)")

    success_count = 0
    converted_fonts = []

    for config in font_configs:
        font_file = project_dir / config["file"]
        if convert_font(
            font_file,
            output_dir,
            config["size"],
            config["name"],
            working_dir=project_dir,
        ):
            success_count += 1
            converted_fonts.append(config["name"])

    print(f"Successfully processed {success_count} of {len(font_configs)} fonts")
    if success_count != len(font_configs):
        raise RuntimeError(
            f"Font conversion failed: successfully processed {success_count} of "
            f"{len(font_configs)} fonts"
        )
    
    # Generate fonts.h file that includes all font headers
    fonts_h_content = """/**
 * @file fonts.h
 * @brief Includes all LVGL font headers
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Include all font headers
"""
    for font_name in converted_fonts:
        fonts_h_content += f'#include "{font_name}.h"\n'
    
    fonts_h_content += """
#ifdef __cplusplus
}
#endif
"""
    
    with (output_dir / "fonts.h").open('w', encoding='utf-8') as f:
        f.write(fonts_h_content)

    print(f"Generated {output_dir}/fonts.h")
    print("All fonts were successfully converted to LVGL format!")
    
    print("\nNext steps:")
    print("1. Include 'fonts.h' in your code")
    print("2. Use the fonts with LVGL like this:")
    print("   - For labels: lv_style_set_text_font(&style, &font_label);")
    print("   - For values: lv_style_set_text_font(&style, &font_value);")
    print("   - For large values: lv_style_set_text_font(&style, &font_value_large);")

# When run as a PlatformIO script, Import("env") is available and the project
# directory must come from PlatformIO rather than the process working directory.
try:
    Import("env")  # noqa: F821
except NameError:
    if __name__ == "__main__":
        main()
else:
    main(Path(env.subst("$PROJECT_DIR")))  # noqa: F821
