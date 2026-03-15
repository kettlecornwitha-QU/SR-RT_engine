#!/usr/bin/env python3
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional, Tuple

from PySide6.QtWidgets import QCheckBox, QLineEdit


def float_text(v: float) -> str:
    return f"{v:.6f}".rstrip("0").rstrip(".") or "0"


@dataclass
class ImageRenderConfig:
    scene_name: str
    big_scatter_palette: str
    width: int
    height: int
    spp: int
    max_depth: int
    adaptive: bool
    adaptive_threshold: float
    aberration_speed: float
    aberration_yaw_turns: float
    aberration_pitch_turns: float
    cam_yaw_turns: float
    cam_pitch_turns: float
    lighting_preset: str
    supports_lighting_preset: bool
    denoise: bool
    ppm_denoised_only: bool
    atmosphere_enabled: bool
    atmosphere_density: float
    atmosphere_color: str
    cam_x: Optional[float]
    cam_y: Optional[float]
    cam_z: Optional[float]
    scatter_spacing_override: Optional[float]
    scatter_max_radius_override: Optional[float]
    scatter_growth_scale_override: Optional[float]
    big_scatter_spacing_override: Optional[float]
    big_scatter_max_radius_override: Optional[float]
    row_scatter_xmax_override: Optional[float]
    row_scatter_z_front_override: Optional[float]
    row_scatter_z_back_override: Optional[float]
    row_scatter_replacement_rate_override: Optional[float]


def safe_float(edit: QLineEdit, default: float) -> float:
    try:
        return float(edit.text().strip())
    except Exception:
        return default


def safe_optional_float(edit: QLineEdit) -> Optional[float]:
    raw = edit.text().strip()
    if not raw:
        return None
    try:
        return float(raw)
    except Exception:
        return None


def optional_override(check: QCheckBox, edit: QLineEdit, default: float) -> Optional[float]:
    if not (check.isEnabled() and check.isChecked()):
        return None
    return safe_float(edit, default)


def velocity_vector_to_turns(vx: float, vy: float, vz: float) -> Tuple[float, float, float]:
    import math

    speed = math.sqrt(vx * vx + vy * vy + vz * vz)
    if speed <= 1e-10:
        return 0.0, 0.0, 0.0

    d_x = vx / speed
    d_y = vy / speed
    d_z = vz / speed
    pitch_turns = math.asin(max(-1.0, min(1.0, d_y))) / (2.0 * math.pi)
    yaw_turns = math.atan2(d_x, -d_z) / (2.0 * math.pi)
    yaw_turns = yaw_turns - math.floor(yaw_turns)
    return speed, yaw_turns, pitch_turns


def resolve_aberration_turns(
    *,
    turns_mode: bool,
    ab_speed: QLineEdit,
    ab_yaw: QLineEdit,
    ab_pitch: QLineEdit,
    vx: QLineEdit,
    vy: QLineEdit,
    vz: QLineEdit,
) -> Tuple[float, float, float]:
    if turns_mode:
        speed = safe_float(ab_speed, 0.0)
        yaw = safe_float(ab_yaw, 0.0)
        pitch = safe_float(ab_pitch, 0.0)
        return speed, yaw, pitch

    return velocity_vector_to_turns(
        safe_float(vx, 0.0),
        safe_float(vy, 0.0),
        safe_float(vz, 0.0),
    )


def build_image_render_config(
    *,
    scene_name: str,
    big_scatter_palette: str,
    turns_mode: bool,
    ab_speed: QLineEdit,
    ab_yaw: QLineEdit,
    ab_pitch: QLineEdit,
    vx: QLineEdit,
    vy: QLineEdit,
    vz: QLineEdit,
    cam_yaw: QLineEdit,
    cam_pitch: QLineEdit,
    cam_x: QLineEdit,
    cam_y: QLineEdit,
    cam_z: QLineEdit,
    settings_dialog: object,
    options_schema: object,
) -> ImageRenderConfig:
    speed, yaw, pitch = resolve_aberration_turns(
        turns_mode=turns_mode,
        ab_speed=ab_speed,
        ab_yaw=ab_yaw,
        ab_pitch=ab_pitch,
        vx=vx,
        vy=vy,
        vz=vz,
    )
    return ImageRenderConfig(
        scene_name=scene_name,
        big_scatter_palette=big_scatter_palette,
        width=settings_dialog.width_spin.value(),
        height=settings_dialog.height_spin.value(),
        spp=settings_dialog.spp_spin.value(),
        max_depth=settings_dialog.max_depth_spin.value(),
        adaptive=settings_dialog.adaptive_check.isChecked(),
        adaptive_threshold=safe_float(settings_dialog.adaptive_threshold, options_schema.render.adaptive_threshold),
        aberration_speed=speed,
        aberration_yaw_turns=yaw,
        aberration_pitch_turns=pitch,
        cam_yaw_turns=safe_float(cam_yaw, 0.0),
        cam_pitch_turns=safe_float(cam_pitch, 0.0),
        lighting_preset=settings_dialog.lighting_preset_combo.currentText(),
        supports_lighting_preset=bool(options_schema.capabilities.supports_lighting_preset),
        denoise=settings_dialog.denoise_check.isChecked(),
        ppm_denoised_only=settings_dialog.ppm_denoised_only.isChecked(),
        atmosphere_enabled=settings_dialog.atmosphere_check.isChecked(),
        atmosphere_density=safe_float(settings_dialog.atmosphere_density, options_schema.render.atmosphere_density),
        atmosphere_color=settings_dialog.atmosphere_color.text().strip(),
        cam_x=safe_optional_float(cam_x),
        cam_y=safe_optional_float(cam_y),
        cam_z=safe_optional_float(cam_z),
        scatter_spacing_override=optional_override(
            settings_dialog.scatter_spacing_check,
            settings_dialog.scatter_spacing_edit,
            options_schema.scene_overrides.scatter_spacing,
        ),
        scatter_max_radius_override=optional_override(
            settings_dialog.scatter_max_radius_check,
            settings_dialog.scatter_max_radius_edit,
            options_schema.scene_overrides.scatter_max_radius,
        ),
        scatter_growth_scale_override=optional_override(
            settings_dialog.scatter_growth_scale_check,
            settings_dialog.scatter_growth_scale_edit,
            options_schema.scene_overrides.scatter_growth_scale,
        ),
        big_scatter_spacing_override=optional_override(
            settings_dialog.big_scatter_spacing_check,
            settings_dialog.big_scatter_spacing_edit,
            options_schema.scene_overrides.big_scatter_spacing,
        ),
        big_scatter_max_radius_override=optional_override(
            settings_dialog.big_scatter_max_radius_check,
            settings_dialog.big_scatter_max_radius_edit,
            options_schema.scene_overrides.big_scatter_max_radius,
        ),
        row_scatter_xmax_override=optional_override(
            settings_dialog.row_scatter_xmax_check,
            settings_dialog.row_scatter_xmax_edit,
            options_schema.scene_overrides.row_scatter_xmax,
        ),
        row_scatter_z_front_override=optional_override(
            settings_dialog.row_scatter_z_front_check,
            settings_dialog.row_scatter_z_front_edit,
            options_schema.scene_overrides.row_scatter_z_front,
        ),
        row_scatter_z_back_override=optional_override(
            settings_dialog.row_scatter_z_back_check,
            settings_dialog.row_scatter_z_back_edit,
            options_schema.scene_overrides.row_scatter_z_back,
        ),
        row_scatter_replacement_rate_override=optional_override(
            settings_dialog.row_scatter_replacement_rate_check,
            settings_dialog.row_scatter_replacement_rate_edit,
            options_schema.scene_overrides.row_scatter_replacement_rate,
        ),
    )


def build_image_cli_args(config: ImageRenderConfig, output_base: Path) -> List[str]:
    args = [
        "--scene",
        config.scene_name,
        "--big-scatter-palette",
        config.big_scatter_palette,
        "--output-base",
        str(output_base),
        "--width",
        str(config.width),
        "--height",
        str(config.height),
        "--spp",
        str(config.spp),
        "--max-depth",
        str(config.max_depth),
        "--adaptive",
        "1" if config.adaptive else "0",
        "--adaptive-threshold",
        float_text(config.adaptive_threshold),
        "--aberration-speed",
        float_text(config.aberration_speed),
        "--aberration-yaw-turns",
        float_text(config.aberration_yaw_turns),
        "--aberration-pitch-turns",
        float_text(config.aberration_pitch_turns),
        "--cam-yaw-turns",
        float_text(config.cam_yaw_turns),
        "--cam-pitch-turns",
        float_text(config.cam_pitch_turns),
        "--denoise",
        "1" if config.denoise else "0",
        "--ppm-denoised-only",
        "1" if config.ppm_denoised_only else "0",
        "--atmosphere",
        "1" if config.atmosphere_enabled else "0",
        "--atmosphere-density",
        float_text(config.atmosphere_density),
        "--atmosphere-color",
        config.atmosphere_color,
    ]

    if config.supports_lighting_preset:
        args += ["--lighting-preset", config.lighting_preset]

    if config.cam_x is not None:
        args += ["--cam-x", float_text(config.cam_x)]
    if config.cam_y is not None:
        args += ["--cam-y", float_text(config.cam_y)]
    if config.cam_z is not None:
        args += ["--cam-z", float_text(config.cam_z)]

    if config.scatter_spacing_override is not None:
        args += ["--scatter-spacing", float_text(config.scatter_spacing_override)]
    if config.scatter_max_radius_override is not None:
        args += ["--scatter-max-radius", float_text(config.scatter_max_radius_override)]
    if config.scatter_growth_scale_override is not None:
        args += ["--scatter-growth-scale", float_text(config.scatter_growth_scale_override)]
    if config.big_scatter_spacing_override is not None:
        args += ["--big-scatter-spacing", float_text(config.big_scatter_spacing_override)]
    if config.big_scatter_max_radius_override is not None:
        args += ["--big-scatter-max-radius", float_text(config.big_scatter_max_radius_override)]
    if config.row_scatter_xmax_override is not None:
        args += ["--row-scatter-xmax", float_text(config.row_scatter_xmax_override)]
    if config.row_scatter_z_front_override is not None:
        args += ["--row-scatter-z-front", float_text(config.row_scatter_z_front_override)]
    if config.row_scatter_z_back_override is not None:
        args += ["--row-scatter-z-back", float_text(config.row_scatter_z_back_override)]
    if config.row_scatter_replacement_rate_override is not None:
        args += ["--row-scatter-replacement-rate", float_text(config.row_scatter_replacement_rate_override)]

    return args
