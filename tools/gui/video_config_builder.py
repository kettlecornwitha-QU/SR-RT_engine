#!/usr/bin/env python3
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional

from PySide6.QtWidgets import QCheckBox, QLineEdit


@dataclass
class RenderTaskConfig:
    scene_name: str
    palette: str
    fps: int
    total_frames_expr: str
    definitions: List[str]
    location_rows: List[Dict[str, object]]
    velocity_rows: List[Dict[str, object]]
    orientation_rows: List[Dict[str, object]]
    width: int
    height: int
    spp: int
    max_depth: int
    adaptive: bool
    adaptive_threshold: float
    denoise: bool
    ppm_denoised_only: bool
    lighting_preset: str
    supports_lighting_preset: bool
    atmosphere_enabled: bool
    atmosphere_density: float
    atmosphere_color: str
    scatter_spacing_override: Optional[float]
    scatter_max_radius_override: Optional[float]
    scatter_growth_scale_override: Optional[float]
    big_scatter_spacing_override: Optional[float]
    big_scatter_max_radius_override: Optional[float]
    row_scatter_xmax_override: Optional[float]
    row_scatter_z_front_override: Optional[float]
    row_scatter_z_back_override: Optional[float]
    row_scatter_replacement_rate_override: Optional[float]
    keep_frames: bool
    options_schema_version: int
    scene_registry_schema_version: int
    raytracer_bin: Path
    output_dir: Path


def _safe_float(edit: QLineEdit, default: float) -> float:
    try:
        return float(edit.text().strip())
    except Exception:
        return default


def _override_value(check: QCheckBox, edit: QLineEdit, default: float) -> Optional[float]:
    if not (check.isEnabled() and check.isChecked()):
        return None
    return _safe_float(edit, default)


def build_render_task_config(
    *,
    scene_name: str,
    palette: str,
    fps_text: str,
    total_frames_expr: str,
    definitions: List[str],
    location_rows: List[Dict[str, object]],
    velocity_rows: List[Dict[str, object]],
    orientation_rows: List[Dict[str, object]],
    settings_dialog: object,
    options_schema: object,
    scene_registry: object,
    raytracer_bin: Path,
    output_dir: Path,
) -> RenderTaskConfig:
    fps = int(fps_text)
    expr = total_frames_expr.strip()
    if not expr:
        raise RuntimeError("Total number of frames is required")

    adaptive_threshold = _safe_float(
        settings_dialog.adaptive_threshold,
        options_schema.render.adaptive_threshold,
    )
    atmosphere_density = _safe_float(
        settings_dialog.atmosphere_density,
        options_schema.render.atmosphere_density,
    )
    atmosphere_color = settings_dialog.atmosphere_color.text().strip() or options_schema.render.atmosphere_color

    return RenderTaskConfig(
        scene_name=scene_name,
        palette=palette,
        fps=fps,
        total_frames_expr=expr,
        definitions=definitions,
        location_rows=location_rows,
        velocity_rows=velocity_rows,
        orientation_rows=orientation_rows,
        width=settings_dialog.width.value(),
        height=settings_dialog.height.value(),
        spp=settings_dialog.spp.value(),
        max_depth=settings_dialog.max_depth.value(),
        adaptive=settings_dialog.adaptive.isChecked(),
        adaptive_threshold=adaptive_threshold,
        denoise=settings_dialog.denoise.isChecked(),
        ppm_denoised_only=settings_dialog.ppm_denoised_only.isChecked(),
        lighting_preset=settings_dialog.lighting_preset.currentText(),
        supports_lighting_preset=bool(options_schema.capabilities.supports_lighting_preset),
        atmosphere_enabled=settings_dialog.atmosphere_check.isChecked(),
        atmosphere_density=atmosphere_density,
        atmosphere_color=atmosphere_color,
        scatter_spacing_override=_override_value(
            settings_dialog.scatter_spacing_check,
            settings_dialog.scatter_spacing_edit,
            options_schema.scene_overrides.scatter_spacing,
        ),
        scatter_max_radius_override=_override_value(
            settings_dialog.scatter_max_radius_check,
            settings_dialog.scatter_max_radius_edit,
            options_schema.scene_overrides.scatter_max_radius,
        ),
        scatter_growth_scale_override=_override_value(
            settings_dialog.scatter_growth_scale_check,
            settings_dialog.scatter_growth_scale_edit,
            options_schema.scene_overrides.scatter_growth_scale,
        ),
        big_scatter_spacing_override=_override_value(
            settings_dialog.big_scatter_spacing_check,
            settings_dialog.big_scatter_spacing_edit,
            options_schema.scene_overrides.big_scatter_spacing,
        ),
        big_scatter_max_radius_override=_override_value(
            settings_dialog.big_scatter_max_radius_check,
            settings_dialog.big_scatter_max_radius_edit,
            options_schema.scene_overrides.big_scatter_max_radius,
        ),
        row_scatter_xmax_override=_override_value(
            settings_dialog.row_scatter_xmax_check,
            settings_dialog.row_scatter_xmax_edit,
            options_schema.scene_overrides.row_scatter_xmax,
        ),
        row_scatter_z_front_override=_override_value(
            settings_dialog.row_scatter_z_front_check,
            settings_dialog.row_scatter_z_front_edit,
            options_schema.scene_overrides.row_scatter_z_front,
        ),
        row_scatter_z_back_override=_override_value(
            settings_dialog.row_scatter_z_back_check,
            settings_dialog.row_scatter_z_back_edit,
            options_schema.scene_overrides.row_scatter_z_back,
        ),
        row_scatter_replacement_rate_override=_override_value(
            settings_dialog.row_scatter_replacement_rate_check,
            settings_dialog.row_scatter_replacement_rate_edit,
            options_schema.scene_overrides.row_scatter_replacement_rate,
        ),
        keep_frames=settings_dialog.keep_frames.isChecked(),
        options_schema_version=options_schema.schema_version,
        scene_registry_schema_version=scene_registry.schema_version,
        raytracer_bin=raytracer_bin,
        output_dir=output_dir,
    )
