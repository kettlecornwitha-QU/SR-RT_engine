#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable, Optional

from gui_runtime import packaged_runtime
from oidn_postprocess import can_run_oidn_postprocess
from video_encoder import find_ffmpeg


@dataclass
class RuntimeDiagnostics:
    critical: list[str] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)


def _git_short_revision(project_root: Path) -> Optional[str]:
    try:
        proc = subprocess.run(
            ["git", "-C", str(project_root), "rev-parse", "--short", "HEAD"],
            capture_output=True,
            text=True,
            timeout=3,
            check=True,
        )
        text = proc.stdout.strip()
        return text or None
    except Exception:
        return None


def app_version_label(project_root: Path) -> str:
    env_version = os.environ.get("SR_RT_APP_VERSION", "").strip()
    if env_version:
        return env_version
    rev = _git_short_revision(project_root)
    if rev:
        return f"dev ({rev})"
    return "dev"


def build_about_text(
    *,
    app_name: str,
    project_root: Path,
    raytracer_bin: Path,
    options_schema_version: Optional[int] = None,
    scene_registry_version: Optional[int] = None,
    extra_lines: Iterable[str] = (),
) -> str:
    lines = [
        f"{app_name}",
        f"Version: {app_version_label(project_root)}",
        f"Runtime: {'packaged' if packaged_runtime() else 'development'}",
        f"Raytracer: {raytracer_bin}",
    ]
    if options_schema_version is not None:
        lines.append(f"Options schema: {options_schema_version}")
    if scene_registry_version is not None:
        lines.append(f"Scene registry: {scene_registry_version}")
    lines.extend(str(x) for x in extra_lines if str(x).strip())
    return "\n".join(lines)


def check_runtime(
    raytracer_bin: Path,
    *,
    require_ffmpeg: bool = False,
    prefer_oidn: bool = False,
) -> RuntimeDiagnostics:
    out = RuntimeDiagnostics()

    if not raytracer_bin.exists():
        out.critical.append(f"Raytracer binary not found: {raytracer_bin}")
        return out

    try:
        proc = subprocess.run(
            [str(raytracer_bin), "--print-options-schema"],
            capture_output=True,
            text=True,
            timeout=10,
            check=False,
        )
        if proc.returncode != 0:
            details = (proc.stderr or proc.stdout).strip()
            msg = f"Raytracer failed startup self-check: {raytracer_bin}"
            if details:
                msg += f"\n{details[-600:]}"
            out.critical.append(msg)
    except Exception as exc:
        out.critical.append(f"Raytracer failed startup self-check: {exc}")

    if require_ffmpeg:
        try:
            find_ffmpeg()
        except Exception as exc:
            out.critical.append(str(exc))

    if prefer_oidn and packaged_runtime() and not can_run_oidn_postprocess():
        out.warnings.append(
            "Bundled oidnDenoise executable not found. Denoise-dependent packaged renders may be unavailable."
        )

    return out
