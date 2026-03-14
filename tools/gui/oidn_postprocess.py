#!/usr/bin/env python3
from __future__ import annotations

import os
import struct
import subprocess
import tempfile
from pathlib import Path
from typing import Optional, Tuple


def oidn_denoise_bin() -> Optional[Path]:
    raw = os.environ.get("SR_RT_OIDN_DENOISE_BIN", "").strip()
    if not raw:
        return None
    path = Path(raw).expanduser().resolve()
    if path.exists():
        return path
    return None


def can_run_oidn_postprocess() -> bool:
    return oidn_denoise_bin() is not None


def _read_token(stream) -> bytes:
    token = bytearray()
    while True:
        ch = stream.read(1)
        if not ch:
            return bytes(token)
        if ch == b"#":
            stream.readline()
            continue
        if not ch.isspace():
            token.append(ch[0])
            break
    while True:
        ch = stream.read(1)
        if not ch or ch.isspace():
            break
        token.append(ch[0])
    return bytes(token)


def _read_ppm(path: Path) -> tuple[int, int, list[tuple[float, float, float]]]:
    with path.open("rb") as f:
        magic = _read_token(f)
        if magic not in {b"P3", b"P6"}:
            raise ValueError(f"Unsupported PPM format in {path}")
        width = int(_read_token(f))
        height = int(_read_token(f))
        max_value = int(_read_token(f))
        if max_value <= 0:
            raise ValueError(f"Invalid max value in {path}")
    scale = 1.0 / float(max_value)
    pixels = []
    if magic == b"P6":
        with path.open("rb") as f:
            _ = _read_token(f)
            _ = _read_token(f)
            _ = _read_token(f)
            _ = _read_token(f)
            raw = f.read(width * height * 3)
            if len(raw) != width * height * 3:
                raise ValueError(f"Unexpected PPM size in {path}")
        for i in range(0, len(raw), 3):
            pixels.append((raw[i] * scale, raw[i + 1] * scale, raw[i + 2] * scale))
    else:
        with path.open("rb") as f:
            _ = _read_token(f)
            _ = _read_token(f)
            _ = _read_token(f)
            _ = _read_token(f)
            values = []
            expected = width * height * 3
            while len(values) < expected:
                tok = _read_token(f)
                if not tok:
                    break
                values.append(int(tok))
            if len(values) != expected:
                raise ValueError(f"Unexpected ASCII PPM size in {path}")
        for i in range(0, len(values), 3):
            pixels.append((values[i] * scale, values[i + 1] * scale, values[i + 2] * scale))
    return width, height, pixels


def _write_pfm(path: Path, width: int, height: int, pixels: list[tuple[float, float, float]]) -> None:
    with path.open("wb") as f:
        f.write(f"PF\n{width} {height}\n-1.0\n".encode("ascii"))
        for y in range(height - 1, -1, -1):
            row = pixels[y * width:(y + 1) * width]
            for r, g, b in row:
                f.write(struct.pack("fff", float(r), float(g), float(b)))


def _read_pfm(path: Path) -> tuple[int, int, list[tuple[float, float, float]]]:
    with path.open("rb") as f:
        magic = f.readline().strip()
        if magic != b"PF":
            raise ValueError(f"Unsupported PFM format in {path}")
        dims = f.readline().strip().split()
        if len(dims) != 2:
            raise ValueError(f"Invalid PFM dimensions in {path}")
        width, height = int(dims[0]), int(dims[1])
        scale = float(f.readline().strip())
        little_endian = scale < 0
        data = f.read(width * height * 3 * 4)
        if len(data) != width * height * 3 * 4:
            raise ValueError(f"Unexpected PFM size in {path}")
    fmt = ("<" if little_endian else ">") + ("f" * width * height * 3)
    floats = struct.unpack(fmt, data)
    pixels = []
    for y in range(height - 1, -1, -1):
        row_start = y * width * 3
        for x in range(width):
            i = row_start + x * 3
            pixels.append((floats[i], floats[i + 1], floats[i + 2]))
    return width, height, pixels


def _write_ppm(path: Path, width: int, height: int, pixels: list[tuple[float, float, float]]) -> None:
    with path.open("wb") as f:
        f.write(f"P6\n{width} {height}\n255\n".encode("ascii"))
        buf = bytearray()
        for r, g, b in pixels:
            buf.extend(
                [
                    max(0, min(255, int(round(r * 255.0)))),
                    max(0, min(255, int(round(g * 255.0)))),
                    max(0, min(255, int(round(b * 255.0)))),
                ]
            )
        f.write(buf)


def run_oidn_postprocess(output_base: Path) -> Tuple[bool, str, Optional[Path]]:
    oidn_bin = oidn_denoise_bin()
    if oidn_bin is None:
        return False, "Bundled oidnDenoise executable not found.", None

    color = output_base.with_suffix(".ppm")
    albedo = output_base.with_name(output_base.name + "_albedo").with_suffix(".ppm")
    normal = output_base.with_name(output_base.name + "_normal").with_suffix(".ppm")
    denoised = output_base.with_name(output_base.name + "_denoised").with_suffix(".ppm")

    missing = [str(p) for p in [color, albedo, normal] if not p.exists()]
    if missing:
        return False, "Missing denoise inputs: " + ", ".join(missing), None

    try:
        color_w, color_h, color_pixels = _read_ppm(color)
        albedo_w, albedo_h, albedo_pixels = _read_ppm(albedo)
        normal_w, normal_h, normal_pixels = _read_ppm(normal)
    except Exception as exc:
        return False, f"Failed to load PPM denoise inputs: {exc}", None

    if (color_w, color_h) != (albedo_w, albedo_h) or (color_w, color_h) != (normal_w, normal_h):
        return False, "Denoise inputs have mismatched image sizes.", None

    temp_dir = Path(tempfile.mkdtemp(prefix="sr_rt_oidn_gui_"))
    color_pfm = temp_dir / "color.pfm"
    albedo_pfm = temp_dir / "albedo.pfm"
    normal_pfm = temp_dir / "normal.pfm"
    denoised_pfm = temp_dir / "denoised.pfm"

    try:
        _write_pfm(color_pfm, color_w, color_h, color_pixels)
        _write_pfm(albedo_pfm, albedo_w, albedo_h, albedo_pixels)
        _write_pfm(normal_pfm, normal_w, normal_h, normal_pixels)
    except Exception as exc:
        return False, f"Failed to prepare temporary PFM files: {exc}", None

    env = os.environ.copy()
    env["DYLD_LIBRARY_PATH"] = str(oidn_bin.parent)

    cmd = [
        str(oidn_bin),
        "--hdr",
        str(color_pfm),
        "--alb",
        str(albedo_pfm),
        "--nrm",
        str(normal_pfm),
        "-o",
        str(denoised_pfm),
        "--clean_aux",
        "--verbose",
        "2",
    ]
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, env=env)
        if proc.returncode != 0:
            details = (proc.stdout + "\n" + proc.stderr).strip()
            if not details:
                details = "oidnDenoise exited with no output"
            return False, details, None

        if not denoised_pfm.exists():
            return False, "oidnDenoise reported success but produced no output file.", None

        out_w, out_h, out_pixels = _read_pfm(denoised_pfm)
        _write_ppm(denoised, out_w, out_h, out_pixels)
        return True, "OIDN denoise complete", denoised
    except Exception as exc:
        return False, f"Failed to convert denoised output: {exc}", None
    finally:
        for path in [color_pfm, albedo_pfm, normal_pfm, denoised_pfm]:
            try:
                path.unlink()
            except FileNotFoundError:
                pass
            except Exception:
                pass
        try:
            temp_dir.rmdir()
        except Exception:
            pass
