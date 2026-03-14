#pragma once

#include "scene/presets/scene_params.h"
#include "scene/scene_builder.h"

enum class RowScatterRowMaterialType {
    Lambertian,
    Clearcoat,
    CoatedPlastic
};

struct RowScatterMaterialPolicy {
    double lambertian_threshold = 0.10;
    double clearcoat_threshold = 0.55;
};

RowScatterMaterialPolicy make_row_scatter_material_policy(const RowScatterParams& params);

RowScatterRowMaterialType pick_row_scatter_row_material_type(
    int row,
    const RowScatterMaterialPolicy& policy);

const Material* make_row_scatter_material_for_slot(
    SceneBuild& s,
    const RowScatterParams& params,
    const RowScatterMaterialPolicy& policy,
    int row,
    int col,
    const Vec3& base_color);
