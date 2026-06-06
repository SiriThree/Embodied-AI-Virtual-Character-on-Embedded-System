#!/usr/bin/env python3
"""Convert PNG images to RGB565 C arrays for STM32 display"""

from PIL import Image
import os
import sys

def rgb888_to_rgb565(r, g, b):
    """Convert RGB888 to RGB565"""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def png_to_c_array(png_path, array_name):
    """Convert PNG to C array"""
    img = Image.open(png_path).convert('RGB')
    width, height = img.size
    pixels = img.load()

    result = []
    result.append(f"/* {os.path.basename(png_path)} - {width}x{height} */")
    result.append(f"const uint16_t {array_name}[{width * height}] = {{")

    hex_values = []
    for y in range(height):
        for x in range(width):
            r, g, b = pixels[x, y]
            rgb565 = rgb888_to_rgb565(r, g, b)
            hex_values.append(f"0x{rgb565:04X}")

    # Format 12 values per line
    for i in range(0, len(hex_values), 12):
        line = ", ".join(hex_values[i:i+12])
        if i + 12 < len(hex_values):
            result.append(f"    {line},")
        else:
            result.append(f"    {line}")

    result.append("};")
    return "\n".join(result), width, height

def process_folder(folder, prefix):
    """Process all PNG files in a folder"""
    files = sorted([f for f in os.listdir(folder) if f.endswith('.png')])
    results = []
    info = []

    for f in files:
        name = os.path.splitext(f)[0]
        array_name = f"{prefix}_{name}"
        path = os.path.join(folder, f)

        code, w, h = png_to_c_array(path, array_name)
        results.append(code)
        info.append((array_name, w, h, name))

    return results, info

if __name__ == "__main__":
    output_c = []
    output_h = []

    # Header guard
    output_h.append("#ifndef __AI_AVATAR_DATA_H")
    output_h.append("#define __AI_AVATAR_DATA_H")
    output_h.append("")
    output_h.append("#include <stdint.h>")
    output_h.append("")

    # Process body
    print("Processing body...")
    body_code, body_info = process_folder("body", "avatar_body")
    output_c.extend(body_code)
    for name, w, h, _ in body_info:
        output_h.append(f"extern const uint16_t {name}[{w * h}];")
        output_h.append(f"#define {name.upper()}_WIDTH {w}")
        output_h.append(f"#define {name.upper()}_HEIGHT {h}")
    output_h.append("")

    # Process eyes
    print("Processing eyes...")
    eye_code, eye_info = process_folder("eye", "avatar_eye")
    output_c.extend(eye_code)
    for name, w, h, _ in eye_info:
        output_h.append(f"extern const uint16_t {name}[{w * h}];")
    output_h.append(f"#define AVATAR_EYE_WIDTH {eye_info[0][1]}")
    output_h.append(f"#define AVATAR_EYE_HEIGHT {eye_info[0][2]}")
    output_h.append("")

    # Process mouths
    print("Processing mouths...")
    mouth_code, mouth_info = process_folder("mouth", "avatar_mouth")
    output_c.extend(mouth_code)
    for name, w, h, _ in mouth_info:
        output_h.append(f"extern const uint16_t {name}[{w * h}];")
    output_h.append(f"#define AVATAR_MOUTH_WIDTH {mouth_info[0][1]}")
    output_h.append(f"#define AVATAR_MOUTH_HEIGHT {mouth_info[0][2]}")
    output_h.append("")

    output_h.append("#endif /* __AI_AVATAR_DATA_H */")

    # Write files
    with open("ai_avatar_data.c", "w", encoding="utf-8") as f:
        f.write('#include "ai_avatar_data.h"\n\n')
        f.write("\n\n".join(output_c))

    with open("ai_avatar_data.h", "w", encoding="utf-8") as f:
        f.write("\n".join(output_h))

    print(f"Generated ai_avatar_data.c and ai_avatar_data.h")
    print(f"Body images: {len(body_info)}")
    print(f"Eye images: {len(eye_info)}")
    print(f"Mouth images: {len(mouth_info)}")
