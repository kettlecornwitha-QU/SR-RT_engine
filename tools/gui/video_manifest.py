#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, Optional

from video_config_builder import RenderTaskConfig


def now_utc_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def raytracer_binary_identity(path: Path) -> Dict[str, object]:
    info: Dict[str, object] = {"path": str(path)}
    try:
        st = path.stat()
        info["size_bytes"] = int(st.st_size)
        info["mtime_unix"] = float(st.st_mtime)
    except Exception:
        return info

    try:
        h = hashlib.sha256()
        with path.open("rb") as f:
            while True:
                chunk = f.read(1024 * 1024)
                if not chunk:
                    break
                h.update(chunk)
        info["sha256"] = h.hexdigest()
    except Exception:
        pass
    return info


def write_manifest(manifest_path: Path, manifest: Dict[str, Any]) -> None:
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    tmp = manifest_path.with_suffix(manifest_path.suffix + ".tmp")
    with tmp.open("w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)
    tmp.replace(manifest_path)


def build_initial_manifest(
    *,
    cfg: RenderTaskConfig,
    total_frames: int,
    video_path: Path,
    manifest_path: Path,
    work_dir: Path,
    frames_dir: Optional[Path],
    frozen_binary: Path,
    ffmpeg_path: str,
) -> Dict[str, Any]:
    return {
        "manifest_schema_version": 1,
        "status": "started",
        "started_utc": now_utc_iso(),
        "scene": {
            "scene_name": cfg.scene_name,
            "palette": cfg.palette,
            "lighting_preset": cfg.lighting_preset,
        },
        "video": {
            "fps": cfg.fps,
            "total_frames": total_frames,
            "total_frames_expr": cfg.total_frames_expr,
        },
        "formulas": {
            "definitions": cfg.definitions,
            "camera_location_rows": cfg.location_rows,
            "camera_velocity_rows": cfg.velocity_rows,
            "camera_orientation_rows": cfg.orientation_rows,
        },
        "render_settings": {
            "width": cfg.width,
            "height": cfg.height,
            "spp": cfg.spp,
            "max_depth": cfg.max_depth,
            "adaptive": cfg.adaptive,
            "adaptive_threshold": cfg.adaptive_threshold,
            "denoise": cfg.denoise,
            "ppm_denoised_only": cfg.ppm_denoised_only,
            "atmosphere_enabled": cfg.atmosphere_enabled,
            "atmosphere_density": cfg.atmosphere_density,
            "atmosphere_color": cfg.atmosphere_color,
            "scatter_spacing_override": cfg.scatter_spacing_override,
            "scatter_max_radius_override": cfg.scatter_max_radius_override,
            "scatter_growth_scale_override": cfg.scatter_growth_scale_override,
            "big_scatter_spacing_override": cfg.big_scatter_spacing_override,
            "big_scatter_max_radius_override": cfg.big_scatter_max_radius_override,
            "row_scatter_xmax_override": cfg.row_scatter_xmax_override,
            "row_scatter_z_front_override": cfg.row_scatter_z_front_override,
            "row_scatter_z_back_override": cfg.row_scatter_z_back_override,
            "row_scatter_replacement_rate_override": cfg.row_scatter_replacement_rate_override,
            "keep_frames": cfg.keep_frames,
            "options_schema_version": cfg.options_schema_version,
            "scene_registry_schema_version": cfg.scene_registry_schema_version,
        },
        "outputs": {
            "video_path": str(video_path),
            "frames_dir": str(frames_dir) if frames_dir is not None else None,
            "work_dir": str(work_dir),
            "streaming_encode": True,
            "manifest_path": str(manifest_path),
        },
        "raytracer_binary": {
            "source": raytracer_binary_identity(cfg.raytracer_bin),
            "frozen_for_job": raytracer_binary_identity(frozen_binary),
        },
        "ffmpeg": {"path": ffmpeg_path, "mode": "streaming"},
        "progress": {
            "frames_rendered": 0,
            "frames_expected": total_frames,
        },
    }


def update_progress(manifest: Dict[str, Any], *, frames_rendered: int, frames_streamed: Optional[int] = None) -> None:
    progress = manifest.setdefault("progress", {})
    progress["frames_rendered"] = frames_rendered
    if frames_streamed is not None:
        progress["frames_streamed"] = frames_streamed


def mark_completed(
    manifest: Dict[str, Any],
    *,
    run_start: float,
    total_frames: int,
    ffmpeg_path: str,
    ffmpeg_command: list[str],
    video_path: Path,
) -> None:
    manifest["status"] = "completed"
    manifest["ended_utc"] = now_utc_iso()
    manifest["duration_sec"] = time.time() - run_start
    update_progress(manifest, frames_rendered=total_frames)
    manifest["ffmpeg"] = {"path": ffmpeg_path, "command": ffmpeg_command, "mode": "streaming"}
    try:
        manifest.setdefault("outputs", {})["video_size_bytes"] = int(video_path.stat().st_size)
    except Exception:
        pass


def mark_failed(manifest: Dict[str, Any], *, run_start: float, error: str) -> None:
    manifest["status"] = "failed"
    manifest["ended_utc"] = now_utc_iso()
    manifest["duration_sec"] = time.time() - run_start
    manifest["error"] = error


def mark_aborted(
    manifest: Dict[str, Any],
    *,
    run_start: float,
    frames_rendered: int,
    frames_streamed: int,
) -> None:
    manifest["status"] = "aborted"
    manifest["ended_utc"] = now_utc_iso()
    manifest["duration_sec"] = time.time() - run_start
    update_progress(manifest, frames_rendered=frames_rendered, frames_streamed=frames_streamed)
