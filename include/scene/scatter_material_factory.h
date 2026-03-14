#pragma once

#include "core/material.h"
#include "scene/scene_builder.h"

enum class ScatterMaterialMode {
    Mixed,
    Lambertian,
    GGXMetal,
    AnisotropicGGXMetal,
    ThinTransmission,
    Sheen,
    Clearcoat,
    Dielectric,
    CoatedPlastic,
    Subsurface,
    ConductorCopper,
    ConductorAluminum,
    IsotropicVolume
};

struct ScatterMaterialContext {
    const Material* glass = nullptr;
    const Material* conductor_copper = nullptr;
    const Material* conductor_aluminum = nullptr;
    bool use_random_walk_sss = false;
};

struct ScatterMaterialChoice {
    const Material* surface_material = nullptr;
    bool use_volume = false;
    Vec3 volume_albedo = {0.0, 0.0, 0.0};
    double volume_density = 0.0;
};

ScatterMaterialChoice choose_scatter_material(SceneBuild& s,
                                              ScatterMaterialMode mode,
                                              const Vec3& base,
                                              const ScatterMaterialContext& ctx,
                                              int gx,
                                              int gz);

