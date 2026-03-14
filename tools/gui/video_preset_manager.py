#!/usr/bin/env python3
from __future__ import annotations

import json
import shutil
from pathlib import Path
from typing import Dict


def _float_text(v: float) -> str:
    return f"{v:.10g}"


PRESET_SCHEMA_VERSION = 2


class VideoPresetManager:
    def __init__(self, project_root: Path, options_schema: object) -> None:
        self.project_root = project_root
        self.options_schema = options_schema

    def legacy_preset_dir(self) -> Path:
        return self.project_root / "outputs" / "video_presets"

    def preset_dir(self) -> Path:
        path = self.project_root / "presets" / "video"
        path.mkdir(parents=True, exist_ok=True)
        legacy = self.legacy_preset_dir()
        if legacy.exists() and legacy.is_dir():
            for legacy_file in legacy.glob("*.json"):
                dest = path / legacy_file.name
                if not dest.exists():
                    try:
                        shutil.copy2(legacy_file, dest)
                    except Exception:
                        pass
        return path

    def collect_preset_data(self, window: object) -> Dict[str, object]:
        scene_base = str(window.scene_combo.currentData() or window.scene_combo.currentText())
        return {
            "preset_schema_version": PRESET_SCHEMA_VERSION,
            "options_schema_version": self.options_schema.schema_version,
            "scene_base": scene_base,
            "scene_name": str(window.variant_combo.currentData() or scene_base),
            "palette": window.palette_combo.currentText(),
            "fps": window.fps_combo.currentText(),
            "total_frames": window.total_frames.text(),
            "definitions": [row.edit.text() for row in window.definitions.rows],
            "camera_location": window._serialize_section(window.camera_location),
            "camera_velocity": window._serialize_section(window.camera_velocity),
            "camera_orientation": window._serialize_section(window.camera_orientation),
            "settings": {
                "width": window.settings_dialog.width.value(),
                "height": window.settings_dialog.height.value(),
                "spp": window.settings_dialog.spp.value(),
                "max_depth": window.settings_dialog.max_depth.value(),
                "adaptive": window.settings_dialog.adaptive.isChecked(),
                "adaptive_threshold": window.settings_dialog.adaptive_threshold.text(),
                "denoise": window.settings_dialog.denoise.isChecked(),
                "lighting_preset": window.settings_dialog.lighting_preset.currentText(),
                "ppm_denoised_only": window.settings_dialog.ppm_denoised_only.isChecked(),
                "atmosphere_enabled": window.settings_dialog.atmosphere_check.isChecked(),
                "atmosphere_density": window.settings_dialog.atmosphere_density.text(),
                "atmosphere_color": window.settings_dialog.atmosphere_color.text(),
                "scatter_spacing_enabled": window.settings_dialog.scatter_spacing_check.isChecked(),
                "scatter_spacing": window.settings_dialog.scatter_spacing_edit.text(),
                "scatter_max_radius_enabled": window.settings_dialog.scatter_max_radius_check.isChecked(),
                "scatter_max_radius": window.settings_dialog.scatter_max_radius_edit.text(),
                "scatter_growth_scale_enabled": window.settings_dialog.scatter_growth_scale_check.isChecked(),
                "scatter_growth_scale": window.settings_dialog.scatter_growth_scale_edit.text(),
                "big_scatter_spacing_enabled": window.settings_dialog.big_scatter_spacing_check.isChecked(),
                "big_scatter_spacing": window.settings_dialog.big_scatter_spacing_edit.text(),
                "big_scatter_max_radius_enabled": window.settings_dialog.big_scatter_max_radius_check.isChecked(),
                "big_scatter_max_radius": window.settings_dialog.big_scatter_max_radius_edit.text(),
                "row_scatter_xmax_enabled": window.settings_dialog.row_scatter_xmax_check.isChecked(),
                "row_scatter_xmax": window.settings_dialog.row_scatter_xmax_edit.text(),
                "row_scatter_z_front_enabled": window.settings_dialog.row_scatter_z_front_check.isChecked(),
                "row_scatter_z_front": window.settings_dialog.row_scatter_z_front_edit.text(),
                "row_scatter_z_back_enabled": window.settings_dialog.row_scatter_z_back_check.isChecked(),
                "row_scatter_z_back": window.settings_dialog.row_scatter_z_back_edit.text(),
                "row_scatter_replacement_rate_enabled": window.settings_dialog.row_scatter_replacement_rate_check.isChecked(),
                "row_scatter_replacement_rate": window.settings_dialog.row_scatter_replacement_rate_edit.text(),
                "keep_frames": window.settings_dialog.keep_frames.isChecked(),
            },
        }

    def migrate_preset_data(self, data: Dict[str, object]) -> Dict[str, object]:
        migrated = dict(data)
        version = int(migrated.get("preset_schema_version", 1))

        if version < 2:
            settings = migrated.get("settings")
            if not isinstance(settings, dict):
                settings = {}
            settings = dict(settings)
            settings.setdefault("lighting_preset", self.options_schema.lighting_preset_default)
            settings.setdefault("keep_frames", self.options_schema.video_keep_frames_default)
            settings.setdefault("ppm_denoised_only", self.options_schema.ppm_denoised_only)
            settings.setdefault("atmosphere_enabled", self.options_schema.atmosphere_enabled)
            settings.setdefault("atmosphere_density", _float_text(self.options_schema.atmosphere_density))
            settings.setdefault("atmosphere_color", self.options_schema.atmosphere_color)
            migrated["settings"] = settings

            if "scene_name" not in migrated and "scene_base" in migrated:
                migrated["scene_name"] = migrated.get("scene_base")
            if "fps" in migrated:
                migrated["fps"] = str(migrated.get("fps", ""))

            migrated["preset_schema_version"] = 2
            migrated["options_schema_version"] = self.options_schema.schema_version

        return migrated

    def apply_preset_data(self, window: object, data: Dict[str, object]) -> None:
        scene_base = str(data.get("scene_base", ""))
        if scene_base:
            for i in range(window.scene_combo.count()):
                if str(window.scene_combo.itemData(i)) == scene_base:
                    window.scene_combo.setCurrentIndex(i)
                    break

        scene_name = str(data.get("scene_name", ""))
        if scene_name:
            for i in range(window.variant_combo.count()):
                if str(window.variant_combo.itemData(i)) == scene_name:
                    window.variant_combo.setCurrentIndex(i)
                    break

        palette = str(data.get("palette", ""))
        if palette:
            idx = window.palette_combo.findText(palette)
            if idx >= 0:
                window.palette_combo.setCurrentIndex(idx)

        fps = str(data.get("fps", ""))
        if fps:
            idx = window.fps_combo.findText(fps)
            if idx >= 0:
                window.fps_combo.setCurrentIndex(idx)
        window.total_frames.setText(str(data.get("total_frames", "")))

        defs = data.get("definitions", [])
        if isinstance(defs, list) and defs:
            window._set_definition_row_count(len(defs))
            for i, text in enumerate(defs):
                window.definitions.rows[i].edit.setText(str(text))

        settings = data.get("settings", {})
        if isinstance(settings, dict):
            window.settings_dialog.width.setValue(int(settings.get("width", window.settings_dialog.width.value())))
            window.settings_dialog.height.setValue(int(settings.get("height", window.settings_dialog.height.value())))
            window.settings_dialog.spp.setValue(int(settings.get("spp", window.settings_dialog.spp.value())))
            window.settings_dialog.max_depth.setValue(
                int(settings.get("max_depth", window.settings_dialog.max_depth.value()))
            )
            window.settings_dialog.adaptive.setChecked(
                bool(settings.get("adaptive", window.settings_dialog.adaptive.isChecked()))
            )
            window.settings_dialog.adaptive_threshold.setText(
                str(settings.get("adaptive_threshold", window.settings_dialog.adaptive_threshold.text()))
            )
            window.settings_dialog.denoise.setChecked(
                bool(settings.get("denoise", window.settings_dialog.denoise.isChecked()))
            )
            window.settings_dialog.ppm_denoised_only.setChecked(
                bool(settings.get("ppm_denoised_only", window.settings_dialog.ppm_denoised_only.isChecked()))
            )
            window.settings_dialog.atmosphere_check.setChecked(
                bool(settings.get("atmosphere_enabled", window.settings_dialog.atmosphere_check.isChecked()))
            )
            window.settings_dialog.atmosphere_density.setText(
                str(settings.get("atmosphere_density", window.settings_dialog.atmosphere_density.text()))
            )
            window.settings_dialog.atmosphere_color.setText(
                str(settings.get("atmosphere_color", window.settings_dialog.atmosphere_color.text()))
            )
            lighting_preset = str(settings.get("lighting_preset", ""))
            if lighting_preset:
                idx = window.settings_dialog.lighting_preset.findText(lighting_preset)
                if idx >= 0:
                    window.settings_dialog.lighting_preset.setCurrentIndex(idx)
            window.settings_dialog.scatter_spacing_check.setChecked(bool(settings.get("scatter_spacing_enabled", False)))
            window.settings_dialog.scatter_spacing_edit.setText(
                str(settings.get("scatter_spacing", window.settings_dialog.scatter_spacing_edit.text()))
            )
            window.settings_dialog.scatter_max_radius_check.setChecked(
                bool(settings.get("scatter_max_radius_enabled", False))
            )
            window.settings_dialog.scatter_max_radius_edit.setText(
                str(settings.get("scatter_max_radius", window.settings_dialog.scatter_max_radius_edit.text()))
            )
            window.settings_dialog.scatter_growth_scale_check.setChecked(
                bool(settings.get("scatter_growth_scale_enabled", False))
            )
            window.settings_dialog.scatter_growth_scale_edit.setText(
                str(settings.get("scatter_growth_scale", window.settings_dialog.scatter_growth_scale_edit.text()))
            )
            window.settings_dialog.big_scatter_spacing_check.setChecked(
                bool(settings.get("big_scatter_spacing_enabled", False))
            )
            window.settings_dialog.big_scatter_spacing_edit.setText(
                str(settings.get("big_scatter_spacing", window.settings_dialog.big_scatter_spacing_edit.text()))
            )
            window.settings_dialog.big_scatter_max_radius_check.setChecked(
                bool(settings.get("big_scatter_max_radius_enabled", False))
            )
            window.settings_dialog.big_scatter_max_radius_edit.setText(
                str(settings.get("big_scatter_max_radius", window.settings_dialog.big_scatter_max_radius_edit.text()))
            )
            window.settings_dialog.row_scatter_xmax_check.setChecked(
                bool(settings.get("row_scatter_xmax_enabled", False))
            )
            window.settings_dialog.row_scatter_xmax_edit.setText(
                str(settings.get("row_scatter_xmax", window.settings_dialog.row_scatter_xmax_edit.text()))
            )
            window.settings_dialog.row_scatter_z_front_check.setChecked(
                bool(settings.get("row_scatter_z_front_enabled", False))
            )
            window.settings_dialog.row_scatter_z_front_edit.setText(
                str(settings.get("row_scatter_z_front", window.settings_dialog.row_scatter_z_front_edit.text()))
            )
            window.settings_dialog.row_scatter_z_back_check.setChecked(
                bool(settings.get("row_scatter_z_back_enabled", False))
            )
            window.settings_dialog.row_scatter_z_back_edit.setText(
                str(settings.get("row_scatter_z_back", window.settings_dialog.row_scatter_z_back_edit.text()))
            )
            window.settings_dialog.row_scatter_replacement_rate_check.setChecked(
                bool(settings.get("row_scatter_replacement_rate_enabled", False))
            )
            window.settings_dialog.row_scatter_replacement_rate_edit.setText(
                str(
                    settings.get(
                        "row_scatter_replacement_rate",
                        window.settings_dialog.row_scatter_replacement_rate_edit.text(),
                    )
                )
            )
            window.settings_dialog.keep_frames.setChecked(
                bool(settings.get("keep_frames", window.settings_dialog.keep_frames.isChecked()))
            )

        if isinstance(data.get("camera_location"), dict):
            window._apply_section_data(window.camera_location, data["camera_location"])
        if isinstance(data.get("camera_velocity"), dict):
            window._apply_section_data(window.camera_velocity, data["camera_velocity"])
        if isinstance(data.get("camera_orientation"), dict):
            window._apply_section_data(window.camera_orientation, data["camera_orientation"])

    def write_preset_file(self, path: Path, preset_name: str, window: object) -> None:
        data = self.collect_preset_data(window)
        data["preset_name"] = preset_name
        with path.open("w", encoding="utf-8") as f:
            json.dump(data, f, indent=2)

    def read_preset_file(self, path: Path) -> Dict[str, object]:
        with path.open("r", encoding="utf-8") as f:
            data = json.load(f)
        if not isinstance(data, dict):
            raise RuntimeError("Preset file is not a JSON object.")
        return self.migrate_preset_data(data)
