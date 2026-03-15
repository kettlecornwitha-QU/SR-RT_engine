from __future__ import annotations

import json
import subprocess
from dataclasses import dataclass, field
from pathlib import Path
from typing import List


@dataclass
class GuiRenderDefaults:
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


@dataclass
class GuiSceneOverrideDefaults:
    scatter_spacing: float = 1.55
    scatter_max_radius: float = 24.0
    scatter_growth_scale: float = 2.0
    big_scatter_spacing: float = 4.6
    big_scatter_max_radius: float = 100.0
    row_scatter_xmax: float = 36.0
    row_scatter_z_front: float = 135.0
    row_scatter_z_back: float = -158.68
    row_scatter_replacement_rate: float = 0.15


@dataclass
class GuiOptionChoices:
    big_scatter_palette: List[str] = field(default_factory=lambda: ["balanced", "vivid", "earthy"])
    lighting_preset: List[str] = field(
        default_factory=lambda: [
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
    )
    video_fps: List[int] = field(default_factory=lambda: [24, 30, 60])


@dataclass
class GuiOptionDefaults:
    big_scatter_palette: str = "balanced"
    lighting_preset: str = "scene-default"
    video_keep_frames: bool = False


@dataclass
class GuiBinaryCapabilities:
    supports_lighting_preset: bool = False


@dataclass
class GuiOptionsSchema:
    schema_version: int = 1
    render: GuiRenderDefaults = field(default_factory=GuiRenderDefaults)
    scene_overrides: GuiSceneOverrideDefaults = field(default_factory=GuiSceneOverrideDefaults)
    choices: GuiOptionChoices = field(default_factory=GuiOptionChoices)
    defaults: GuiOptionDefaults = field(default_factory=GuiOptionDefaults)
    capabilities: GuiBinaryCapabilities = field(default_factory=GuiBinaryCapabilities)

    # Compatibility passthroughs while the rest of the GUI migrates.
    @property
    def width(self) -> int:
        return self.render.width

    @property
    def height(self) -> int:
        return self.render.height

    @property
    def spp(self) -> int:
        return self.render.spp

    @property
    def max_depth(self) -> int:
        return self.render.max_depth

    @property
    def adaptive(self) -> bool:
        return self.render.adaptive

    @property
    def adaptive_threshold(self) -> float:
        return self.render.adaptive_threshold

    @property
    def denoise(self) -> bool:
        return self.render.denoise

    @property
    def ppm_denoised_only(self) -> bool:
        return self.render.ppm_denoised_only

    @property
    def atmosphere_enabled(self) -> bool:
        return self.render.atmosphere_enabled

    @property
    def atmosphere_density(self) -> float:
        return self.render.atmosphere_density

    @property
    def atmosphere_color(self) -> str:
        return self.render.atmosphere_color

    @property
    def scatter_spacing(self) -> float:
        return self.scene_overrides.scatter_spacing

    @property
    def scatter_max_radius(self) -> float:
        return self.scene_overrides.scatter_max_radius

    @property
    def scatter_growth_scale(self) -> float:
        return self.scene_overrides.scatter_growth_scale

    @property
    def big_scatter_spacing(self) -> float:
        return self.scene_overrides.big_scatter_spacing

    @property
    def big_scatter_max_radius(self) -> float:
        return self.scene_overrides.big_scatter_max_radius

    @property
    def row_scatter_xmax(self) -> float:
        return self.scene_overrides.row_scatter_xmax

    @property
    def row_scatter_z_front(self) -> float:
        return self.scene_overrides.row_scatter_z_front

    @property
    def row_scatter_z_back(self) -> float:
        return self.scene_overrides.row_scatter_z_back

    @property
    def row_scatter_replacement_rate(self) -> float:
        return self.scene_overrides.row_scatter_replacement_rate

    @property
    def big_scatter_palette_choices(self) -> List[str]:
        return self.choices.big_scatter_palette

    @property
    def lighting_preset_choices(self) -> List[str]:
        return self.choices.lighting_preset

    @property
    def video_fps_choices(self) -> List[int]:
        return self.choices.video_fps

    @property
    def big_scatter_palette_default(self) -> str:
        return self.defaults.big_scatter_palette

    @property
    def lighting_preset_default(self) -> str:
        return self.defaults.lighting_preset

    @property
    def video_keep_frames_default(self) -> bool:
        return self.defaults.video_keep_frames

    @property
    def supports_lighting_preset(self) -> bool:
        return self.capabilities.supports_lighting_preset


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
        schema.render.width = int(render.get("width", schema.render.width))
        schema.render.height = int(render.get("height", schema.render.height))
        schema.render.spp = int(render.get("spp", schema.render.spp))
        schema.render.max_depth = int(render.get("max_depth", schema.render.max_depth))
        schema.render.adaptive = bool(render.get("adaptive", schema.render.adaptive))
        schema.render.adaptive_threshold = float(render.get("adaptive_threshold", schema.render.adaptive_threshold))
        schema.render.denoise = bool(render.get("denoise", schema.render.denoise))
        schema.render.ppm_denoised_only = bool(render.get("ppm_denoised_only", schema.render.ppm_denoised_only))
        schema.render.atmosphere_enabled = bool(render.get("atmosphere_enabled", schema.render.atmosphere_enabled))
        schema.render.atmosphere_density = float(render.get("atmosphere_density", schema.render.atmosphere_density))
        schema.render.atmosphere_color = _csv_color_from_list(render.get("atmosphere_color"))

        schema.scene_overrides.scatter_spacing = float(
            scene_overrides.get("scatter_spacing", schema.scene_overrides.scatter_spacing)
        )
        schema.scene_overrides.scatter_max_radius = float(
            scene_overrides.get("scatter_max_radius", schema.scene_overrides.scatter_max_radius)
        )
        schema.scene_overrides.scatter_growth_scale = float(
            scene_overrides.get("scatter_growth_scale", schema.scene_overrides.scatter_growth_scale)
        )
        schema.scene_overrides.big_scatter_spacing = float(
            scene_overrides.get("big_scatter_spacing", schema.scene_overrides.big_scatter_spacing)
        )
        schema.scene_overrides.big_scatter_max_radius = float(
            scene_overrides.get("big_scatter_max_radius", schema.scene_overrides.big_scatter_max_radius)
        )
        schema.scene_overrides.row_scatter_xmax = float(
            scene_overrides.get("row_scatter_xmax", schema.scene_overrides.row_scatter_xmax)
        )
        schema.scene_overrides.row_scatter_z_front = float(
            scene_overrides.get("row_scatter_z_front", schema.scene_overrides.row_scatter_z_front)
        )
        schema.scene_overrides.row_scatter_z_back = float(
            scene_overrides.get("row_scatter_z_back", schema.scene_overrides.row_scatter_z_back)
        )
        schema.scene_overrides.row_scatter_replacement_rate = float(
            scene_overrides.get(
                "row_scatter_replacement_rate",
                schema.scene_overrides.row_scatter_replacement_rate,
            )
        )

        palette_choices = choices.get("big_scatter_palette")
        if isinstance(palette_choices, list) and palette_choices:
            schema.choices.big_scatter_palette = [str(x) for x in palette_choices]
        lighting_choices = choices.get("lighting_preset")
        if isinstance(lighting_choices, list) and lighting_choices:
            schema.choices.lighting_preset = [str(x) for x in lighting_choices]
            schema.capabilities.supports_lighting_preset = True
        fps_choices = choices.get("video_fps")
        if isinstance(fps_choices, list) and fps_choices:
            schema.choices.video_fps = [int(x) for x in fps_choices]

        schema.defaults.big_scatter_palette = str(
            defaults.get("big_scatter_palette", schema.defaults.big_scatter_palette)
        )
        schema.defaults.lighting_preset = str(
            defaults.get("lighting_preset", schema.defaults.lighting_preset)
        )
        schema.defaults.video_keep_frames = bool(
            defaults.get("video_keep_frames", schema.defaults.video_keep_frames)
        )
    except Exception:
        return schema

    return schema
