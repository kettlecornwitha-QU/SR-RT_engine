#!/usr/bin/env python3
from __future__ import annotations

import os
import sys
from pathlib import Path


def _configure_packaged_runtime() -> None:
    if not getattr(sys, "frozen", False):
        return

    app_root = Path(getattr(sys, "_MEIPASS", Path(sys.executable).resolve().parent)).resolve()
    os.environ.setdefault("SR_RT_PACKAGED", "1")
    os.environ.setdefault("SR_RT_APP_ROOT", str(app_root))

    bundled_raytracer = app_root / "bin" / "raytracer"
    if bundled_raytracer.exists():
        os.environ.setdefault("SR_RT_RAYTRACER_BIN", str(bundled_raytracer))

    bundled_oidn = app_root / "bin" / "oidnDenoise"
    if bundled_oidn.exists():
        os.environ.setdefault("SR_RT_OIDN_DENOISE_BIN", str(bundled_oidn))

    bundled_ffmpeg = app_root / "bin" / "ffmpeg"
    if bundled_ffmpeg.exists():
        os.environ.setdefault("SR_RT_FFMPEG_BIN", str(bundled_ffmpeg))

    outputs = Path.home() / "Pictures" / "SR-RT Engine" / "outputs"
    outputs.mkdir(parents=True, exist_ok=True)
    os.environ.setdefault("SR_RT_OUTPUT_DIR", str(outputs))


_configure_packaged_runtime()

from raytracer_animation_gui import main


if __name__ == "__main__":
    raise SystemExit(main())
