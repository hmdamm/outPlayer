#!/usr/bin/env python3
"""
Icon generator script for outPlayer.
Renders SVG icon to multi-resolution PNG assets for FreeDesktop hicolor theme.
Resolutions: 16x16, 32x32, 48x48, 64x64, 128x128, 256x256, 512x512.
"""

import os
import sys
import shutil
import subprocess

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))
SVG_PATH = os.path.join(SCRIPT_DIR, "hicolor", "scalable", "apps", "org.outplayer.outPlayer.svg")
HICOLOR_DIR = os.path.join(SCRIPT_DIR, "hicolor")

TARGET_SIZES = [16, 32, 48, 64, 128, 256, 512]


def render_with_convert(svg_file, output_png, size):
    cmd = [
        "convert",
        "-background", "none",
        "-density", str(max(72, int(72 * (size / 48.0)))),
        "-resize", f"{size}x{size}",
        svg_file,
        output_png,
    ]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"convert failed: {res.stderr}")


def render_with_pil_fallback(svg_file, output_png, size):
    # Pure PIL rendering fallback for basic icons if external tool is absent
    from PIL import Image, ImageDraw

    # Draw dark rounded squircle plate with amber ring and play glyph
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    margin = max(1, int(size * 0.05))
    corner_r = max(2, int(size * 0.22))
    draw.rounded_rectangle(
        [margin, margin, size - margin, size - margin],
        radius=corner_r,
        fill=(27, 27, 30, 255),
        outline=(50, 50, 55, 255),
        width=max(1, int(size * 0.02)),
    )

    # Amber ring
    center = size / 2.0
    ring_r = int(size * 0.32)
    ring_bbox = [center - ring_r, center - ring_r, center + ring_r, center + ring_r]
    ring_width = max(1, int(size * 0.025))
    draw.ellipse(ring_bbox, outline=(255, 170, 0, 255), width=ring_width)

    # Inner core
    core_r = int(size * 0.28)
    core_bbox = [center - core_r, center - core_r, center + core_r, center + core_r]
    draw.ellipse(core_bbox, fill=(20, 20, 22, 255))

    # Play glyph (triangle)
    p1 = (int(center - size * 0.08), int(center - size * 0.16))
    p2 = (int(center + size * 0.16), int(center))
    p3 = (int(center - size * 0.08), int(center + size * 0.16))
    draw.polygon([p1, p2, p3], fill=(255, 255, 255, 255))

    img.save(output_png, "PNG")


def main():
    if not os.path.exists(SVG_PATH):
        print(f"Error: Scalable SVG icon not found at {SVG_PATH}", file=sys.stderr)
        sys.exit(1)

    has_convert = shutil.which("convert") is not None
    print(f"Rendering outPlayer icons from: {SVG_PATH}")
    print(f"Render engine: {'ImageMagick convert' if has_convert else 'PIL fallback'}")

    for size in TARGET_SIZES:
        size_dir = os.path.join(HICOLOR_DIR, f"{size}x{size}", "apps")
        os.makedirs(size_dir, exist_ok=True)
        out_path = os.path.join(size_dir, "org.outplayer.outPlayer.png")

        try:
            if has_convert:
                render_with_convert(SVG_PATH, out_path, size)
            else:
                render_with_pil_fallback(SVG_PATH, out_path, size)
            print(f"Generated: {out_path} ({size}x{size})")
        except Exception as e:
            print(f"Warning: convert failed for {size}x{size} ({e}), falling back to PIL...", file=sys.stderr)
            render_with_pil_fallback(SVG_PATH, out_path, size)
            print(f"Generated via PIL: {out_path} ({size}x{size})")

    print("\nAll icon resolutions rendered successfully.")


if __name__ == "__main__":
    main()
