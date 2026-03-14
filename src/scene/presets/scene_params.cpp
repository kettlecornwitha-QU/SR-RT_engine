#include "scene/presets/scene_params.h"

RowScatterParams make_row_scatter_params() {
    RowScatterParams params;
    return params;
}

ScatterParams make_scatter_params() {
    ScatterParams params;
    return params;
}

BigScatterParams make_big_scatter_params() {
    BigScatterParams params;
    return params;
}

void apply_scene_param_overrides(ScatterParams& params,
                                 const SceneParamOverrides& overrides)
{
    if (overrides.scatter_spacing_override)
        params.spacing = overrides.scatter_spacing;
    if (overrides.scatter_max_radius_override)
        params.max_field_radius = overrides.scatter_max_radius;
    if (overrides.scatter_growth_scale_override)
        params.radius_growth_scale = overrides.scatter_growth_scale;
}

void apply_scene_param_overrides(BigScatterParams& params,
                                 const SceneParamOverrides& overrides)
{
    if (overrides.big_scatter_spacing_override)
        params.spacing = overrides.big_scatter_spacing;
    if (overrides.big_scatter_max_radius_override)
        params.max_field_radius = overrides.big_scatter_max_radius;
}

void apply_scene_param_overrides(RowScatterParams& params,
                                 const SceneParamOverrides& overrides)
{
    if (overrides.row_scatter_xmax_override)
        params.x_max = overrides.row_scatter_xmax;
    if (overrides.row_scatter_z_front_override)
        params.z_front = overrides.row_scatter_z_front;
    if (overrides.row_scatter_z_back_override)
        params.z_back = overrides.row_scatter_z_back;
    if (overrides.row_scatter_replacement_rate_override)
        params.replacement_rate = overrides.row_scatter_replacement_rate;
}
