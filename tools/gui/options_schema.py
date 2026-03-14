from __future__ import annotations

import json
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import List


@dataclass
class GuiOptionsSchema:
    schema_version: int = 1
    width: int = 1920
    height: int = 1080
    spp: int = 128
    max_depth: int = 5
    adaptive: bool = True
    adaptive_threshold: float = 0.01
    denoise: bool = True
    ppm_denoised_only: bool = True
    atmosphere_enabled: bool = False
    atmosphere_density: float = 0.03
    atmosphere_color: str = "0.74,0.83,0.95"

    scatter_spacing: float = 1.55
    scatter_max_radius: float = 24.0
    scatter_growth_scale: float = 2.0
    big_scatter_spacing: float = 4.6
    big_scatter_max_radius: float = 100.0
    row_scatter_xmax: float = 36.0
    row_scatter_z_front: float = 135.0
    row_scatter_z_back: float = -158.68
    row_scatter_replacement_rate: float = 0.15

    big_scatter_palette_choices: List[str] = None
    lighting_preset_choices: List[str] = None
    video_fps_choices: List[int] = None
    big_scatter_palette_default: str = "balanced"
    lighting_preset_default: str = "scene-default"
    video_keep_frames_default: bool = False

    def __post_init__(self) -> None:
        if self.big_scatter_palette_choices is None:
            self.big_scatter_palette_choices = ["balanced", "vivid", "earthy"]
        if self.lighting_preset_choices is None:
            self.lighting_preset_choices = [
                "scene-default",
                "scatter-default",
                "big-scatter-default",
                "row-scatter-default",
                "materials-studio",
                "cornell-ceiling",
                "cornell-ceiling-plus-key",
                "benchmark-softbox",
                "volumes-backlit",
            ]
        if self.video_fps_choices is None:
            self.video_fps_choices = [24, 30, 60]


def _csv_color_from_list(v: object) -> str:
    if isinstance(v, list) and len(v) == 3:
        try:
            return ",".join(f"{float(x):.10g}" for x in v)
        except Exception:
            return "0.74,0.83,0.95"
    return "0.74,0.83,0.95"


def load_gui_options_schema(raytracer_bin: Path) -> GuiOptionsSchema:
    schema = GuiOptionsSchema()
    try:
        proc = subprocess.run(
            [str(raytracer_bin), "--print-options-schema"],
            check=True,
            capture_output=True,
            text=True,
        )
        data = json.loads(proc.stdout)
    except Exception:
        return schema

    render = data.get("render_defaults", {})
    scene_overrides = data.get("scene_override_defaults", {})
    choices = data.get("choices", {})
    defaults = data.get("defaults", {})

    try:
        schema.schema_version = int(data.get("schema_version", schema.schema_version))
        schema.width = int(render.get("width", schema.width))
        schema.height = int(render.get("height", schema.height))
        schema.spp = int(render.get("spp", schema.spp))
        schema.max_depth = int(render.get("max_depth", schema.max_depth))
        schema.adaptive = bool(render.get("adaptive", schema.adaptive))
        schema.adaptive_threshold = float(render.get("adaptive_threshold", schema.adaptive_threshold))
        schema.denoise = bool(render.get("denoise", schema.denoise))
        schema.ppm_denoised_only = bool(render.get("ppm_denoised_only", schema.ppm_denoised_only))
        schema.atmosphere_enabled = bool(render.get("atmosphere_enabled", schema.atmosphere_enabled))
        schema.atmosphere_density = float(render.get("atmosphere_density", schema.atmosphere_density))
        schema.atmosphere_color = _csv_color_from_list(render.get("atmosphere_color"))

        schema.scatter_spacing = float(scene_overrides.get("scatter_spacing", schema.scatter_spacing))
        schema.scatter_max_radius = float(scene_overrides.get("scatter_max_radius", schema.scatter_max_radius))
        schema.scatter_growth_scale = float(scene_overrides.get("scatter_growth_scale", schema.scatter_growth_scale))
        schema.big_scatter_spacing = float(scene_overrides.get("big_scatter_spacing", schema.big_scatter_spacing))
        schema.big_scatter_max_radius = float(scene_overrides.get("big_scatter_max_radius", schema.big_scatter_max_radius))
        schema.row_scatter_xmax = float(scene_overrides.get("row_scatter_xmax", schema.row_scatter_xmax))
        schema.row_scatter_z_front = float(scene_overrides.get("row_scatter_z_front", schema.row_scatter_z_front))
        schema.row_scatter_z_back = float(scene_overrides.get("row_scatter_z_back", schema.row_scatter_z_back))
        schema.row_scatter_replacement_rate = float(
            scene_overrides.get("row_scatter_replacement_rate", schema.row_scatter_replacement_rate)
        )

        palette_choices = choices.get("big_scatter_palette")
        if isinstance(palette_choices, list) and palette_choices:
            schema.big_scatter_palette_choices = [str(x) for x in palette_choices]
        lighting_choices = choices.get("lighting_preset")
        if isinstance(lighting_choices, list) and lighting_choices:
            schema.lighting_preset_choices = [str(x) for x in lighting_choices]
        fps_choices = choices.get("video_fps")
        if isinstance(fps_choices, list) and fps_choices:
            schema.video_fps_choices = [int(x) for x in fps_choices]

        schema.big_scatter_palette_default = str(
            defaults.get("big_scatter_palette", schema.big_scatter_palette_default)
        )
        schema.lighting_preset_default = str(defaults.get("lighting_preset", schema.lighting_preset_default))
        schema.video_keep_frames_default = bool(
            defaults.get("video_keep_frames", schema.video_keep_frames_default)
        )
    except Exception:
        return schema

    return schema
