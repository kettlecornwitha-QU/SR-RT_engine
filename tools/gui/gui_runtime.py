#!/usr/bin/env python3
from __future__ import annotations

import os
import sys
import tempfile
import time
import uuid
from pathlib import Path


def project_root_path(current_file: Path) -> Path:
    env_project_root = os.environ.get("SR_RT_PROJECT_ROOT", "").strip()
    if env_project_root:
        return Path(env_project_root).expanduser().resolve()

    env_app_root = os.environ.get("SR_RT_APP_ROOT", "").strip()
    if env_app_root:
        return Path(env_app_root).expanduser().resolve()

    current = current_file.resolve()
    for parent in [current.parent, *current.parents]:
        if (parent / "CMakeLists.txt").exists() and (parent / "tools" / "gui").exists():
            return parent

    if len(current.parents) >= 3:
        return current.parents[2]
    return current.parent


def packaged_runtime() -> bool:
    return os.environ.get("SR_RT_PACKAGED", "").strip() == "1" or bool(getattr(sys, "frozen", False))


def raytracer_binary_path(project_root: Path, current_file: Path | None = None) -> Path:
    env_bin = os.environ.get("SR_RT_RAYTRACER_BIN", "").strip()
    if env_bin:
        return Path(env_bin).expanduser().resolve()

    candidates: list[Path] = []
    if current_file is not None:
        current = current_file.resolve()
        for parent in [current.parent, *current.parents]:
            candidates.extend(
                [
                    parent / "bin" / "raytracer",
                    parent / "Resources" / "bin" / "raytracer",
                    parent / "MacOS" / "raytracer",
                    parent / "raytracer",
                ]
            )

    candidates.append(project_root / "build" / "raytracer")

    for candidate in candidates:
        if candidate.exists():
            return candidate.resolve()

    return candidates[-1]


def outputs_dir(project_root: Path) -> Path:
    env_output_dir = os.environ.get("SR_RT_OUTPUT_DIR", "").strip()
    if env_output_dir:
        path = Path(env_output_dir).expanduser().resolve()
    else:
        path = project_root / "outputs"
    path.mkdir(parents=True, exist_ok=True)
    return path


def gui_temp_output_dir(name: str = "sr_rt_gui") -> Path:
    path = Path(tempfile.gettempdir()) / name
    path.mkdir(parents=True, exist_ok=True)
    return path


def new_image_output_base(temp_dir: Path, *, prefix: str = "gui_run") -> Path:
    run_id = f"{time.time_ns()}_{uuid.uuid4().hex[:8]}"
    return temp_dir / f"{prefix}_{run_id}"


def pick_output_image(base: Path) -> Path:
    denoised = base.with_name(base.name + "_denoised").with_suffix(".ppm")
    base_ppm = base.with_suffix(".ppm")
    if denoised.exists():
        return denoised
    return base_ppm
