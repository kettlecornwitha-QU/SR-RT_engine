from __future__ import annotations

import json
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple


@dataclass
class SceneVariant:
    label: str
    scene_name: str


@dataclass
class SceneGroup:
    key: str
    label: str
    default_variant: str
    has_variants: bool
    variants: List[SceneVariant]


@dataclass
class SceneRegistry:
    schema_version: int
    groups: List[SceneGroup]
    aliases: List[str]


def _fallback_registry() -> SceneRegistry:
    return SceneRegistry(
        schema_version=1,
        groups=[
            SceneGroup(
                key="scatter",
                label="Scatter",
                default_variant="scatter",
                has_variants=True,
                variants=[
                    SceneVariant("mixed", "scatter"),
                    SceneVariant("lambertian", "scatter_lambertian"),
                    SceneVariant("ggx_metal", "scatter_ggx_metal"),
                    SceneVariant("anisotropic_ggx_metal", "scatter_anisotropic_ggx_metal"),
                ],
            ),
            SceneGroup(
                key="big_scatter",
                label="Big Scatter",
                default_variant="big_scatter",
                has_variants=True,
                variants=[
                    SceneVariant("mixed", "big_scatter"),
                    SceneVariant("lambertian", "big_scatter_lambertian"),
                    SceneVariant("ggx_metal", "big_scatter_ggx_metal"),
                    SceneVariant("anisotropic_ggx_metal", "big_scatter_anisotropic_ggx_metal"),
                ],
            ),
            SceneGroup(
                key="row_scatter",
                label="Row Scatter",
                default_variant="row_scatter",
                has_variants=False,
                variants=[SceneVariant("default", "row_scatter")],
            ),
            SceneGroup(
                key="materials",
                label="Materials",
                default_variant="materials",
                has_variants=False,
                variants=[SceneVariant("default", "materials")],
            ),
            SceneGroup(
                key="spheres",
                label="Spheres",
                default_variant="spheres",
                has_variants=False,
                variants=[SceneVariant("default", "spheres")],
            ),
        ],
        aliases=[],
    )


def load_scene_registry(raytracer_bin: Path) -> SceneRegistry:
    try:
        proc = subprocess.run(
            [str(raytracer_bin), "--print-scene-registry"],
            check=True,
            capture_output=True,
            text=True,
        )
        data = json.loads(proc.stdout)
    except Exception:
        return _fallback_registry()

    try:
        groups: List[SceneGroup] = []
        for raw_group in data.get("groups", []):
            variants = [
                SceneVariant(
                    label=str(v.get("label", "default")),
                    scene_name=str(v.get("scene_name", "")),
                )
                for v in raw_group.get("variants", [])
                if str(v.get("scene_name", "")).strip()
            ]
            if not variants:
                continue
            groups.append(
                SceneGroup(
                    key=str(raw_group.get("key", variants[0].scene_name)),
                    label=str(raw_group.get("label", raw_group.get("key", variants[0].scene_name))),
                    default_variant=str(raw_group.get("default_variant", variants[0].scene_name)),
                    has_variants=bool(raw_group.get("has_variants", len(variants) > 1)),
                    variants=variants,
                )
            )

        if not groups:
            return _fallback_registry()

        return SceneRegistry(
            schema_version=int(data.get("schema_version", 1)),
            groups=groups,
            aliases=[str(a) for a in data.get("aliases", [])],
        )
    except Exception:
        return _fallback_registry()


def to_scene_variant_map(registry: SceneRegistry) -> Dict[str, List[Tuple[str, str]]]:
    m: Dict[str, List[Tuple[str, str]]] = {}
    for group in registry.groups:
        m[group.key] = [(v.label, v.scene_name) for v in group.variants]
    return m
