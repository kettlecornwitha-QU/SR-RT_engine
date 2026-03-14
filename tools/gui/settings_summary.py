#!/usr/bin/env python3
from __future__ import annotations


def count_scene_overrides(settings_dialog: object) -> int:
    return sum(
        1
        for check in [
            settings_dialog.scatter_spacing_check,
            settings_dialog.scatter_max_radius_check,
            settings_dialog.scatter_growth_scale_check,
            settings_dialog.big_scatter_spacing_check,
            settings_dialog.big_scatter_max_radius_check,
            settings_dialog.row_scatter_xmax_check,
            settings_dialog.row_scatter_z_front_check,
            settings_dialog.row_scatter_z_back_check,
            settings_dialog.row_scatter_replacement_rate_check,
        ]
        if check.isChecked()
    )


def build_settings_summary(
    settings_dialog: object,
    *,
    include_keep_frames: bool = False,
    include_lighting: bool = False,
) -> str:
    parts = [
        "Settings:",
        f"{settings_dialog.width.value()}x{settings_dialog.height.value()}",
        f"SPP {settings_dialog.spp.value()}",
        f"depth {settings_dialog.max_depth.value()}",
        f"adaptive {'on' if settings_dialog.adaptive.isChecked() else 'off'}",
        f"denoise {'on' if settings_dialog.denoise.isChecked() else 'off'}",
        f"fog {'on' if settings_dialog.atmosphere_check.isChecked() else 'off'}",
    ]
    if include_keep_frames:
        parts.append(f"keep frames {'on' if settings_dialog.keep_frames.isChecked() else 'off'}")
    parts.append(f"scene overrides {count_scene_overrides(settings_dialog)}")
    if include_lighting:
        parts.append(f"lighting {settings_dialog.lighting_preset.currentText()}")
    return ", ".join(parts)
