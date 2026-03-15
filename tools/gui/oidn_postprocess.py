#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
import tempfile
from pathlib import Path
from typing import Optional, Tuple

from image_io import read_pfm, read_ppm, write_pfm, write_ppm


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
        color_w, color_h, color_pixels = read_ppm(color)
        albedo_w, albedo_h, albedo_pixels = read_ppm(albedo)
        normal_w, normal_h, normal_pixels = read_ppm(normal)
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
        write_pfm(color_pfm, color_w, color_h, color_pixels)
        write_pfm(albedo_pfm, albedo_w, albedo_h, albedo_pixels)
        write_pfm(normal_pfm, normal_w, normal_h, normal_pixels)
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

        out_w, out_h, out_pixels = read_pfm(denoised_pfm)
        write_ppm(denoised, out_w, out_h, out_pixels)
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
