#pragma once

#include <cstddef>

struct RowScatterParams {
    double ground_y = -0.5;
    double x_max = 36.0;
    double z_front = 135.0;
    double z_back = -158.68;
    double row_spacing_scale = 2.42;

    double min_radius = 0.14;
    double radius_noise_amount = 0.12;
    double jitter_x_amount = 0.14;
    double jitter_z_amount = 0.10;

    double overlap_radius_scale = 1.02;
    double overlap_padding = 0.02;
    int overlap_relax_attempts = 8;
    double overlap_jitter_relax = 0.85;
    double overlap_radius_relax = 0.10;
    double overlap_fallback_radius_scale = 0.78;

    double replacement_rate = 0.15;

    double lambertian_weight = 0.10;
    double clearcoat_weight = 0.45;
    double coated_plastic_weight = 0.45;
    double subsurface_weight = 0.0;
};

struct ScatterParams {
    double ground_y = -0.5;
    double spacing = 1.55;
    double max_sat_dz = 16.0;
    double min_field_radius = 2.4;
    double max_field_radius = 24.0;
    double jitter_amount = 0.45;

    double radius_base = 0.16;
    double radius_dist_scale = 0.03;
    double radius_growth_scale = 2.0;
    double radius_growth_power = 1.7;
    double radius_rand_min = 0.85;
    double radius_rand_scale = 0.30;
    double radius_min = 0.16;
    double radius_max = 2.8;

    double overlap_radius_scale = 1.05;
    double overlap_padding = 0.03;
    std::size_t placed_reserve = 1600;
};

struct BigScatterParams {
    double ground_y = -0.5;
    double spacing = 4.6;
    double min_field_radius = 6.0;
    double max_field_radius = 100.0;
    double jitter_amount = 1.6;
    double poly_bound_scale = 0.8660254037844386; // sqrt(3)/2

    double size_base = 0.55;
    double size_rand_scale = 3.15;
    double size_rand_power = 1.35;
    double size_dist_scale = 1.10;

    double shape_sphere_threshold = 0.34;
    double shape_cube_threshold = 0.67;
    double volume_slot_rate = 0.16;

    double overlap_radius_scale = 1.03;
    double overlap_padding = 0.14;
    std::size_t placed_reserve = 2200;
};

struct SceneParamOverrides {
    bool scatter_spacing_override = false;
    double scatter_spacing = 1.55;
    bool scatter_max_radius_override = false;
    double scatter_max_radius = 24.0;
    bool scatter_growth_scale_override = false;
    double scatter_growth_scale = 2.0;

    bool big_scatter_spacing_override = false;
    double big_scatter_spacing = 4.6;
    bool big_scatter_max_radius_override = false;
    double big_scatter_max_radius = 100.0;

    bool row_scatter_xmax_override = false;
    double row_scatter_xmax = 36.0;
    bool row_scatter_z_front_override = false;
    double row_scatter_z_front = 135.0;
    bool row_scatter_z_back_override = false;
    double row_scatter_z_back = -158.68;
    bool row_scatter_replacement_rate_override = false;
    double row_scatter_replacement_rate = 0.15;
};

RowScatterParams make_row_scatter_params();
ScatterParams make_scatter_params();
BigScatterParams make_big_scatter_params();

void apply_scene_param_overrides(ScatterParams& params,
                                 const SceneParamOverrides& overrides);
void apply_scene_param_overrides(BigScatterParams& params,
                                 const SceneParamOverrides& overrides);
void apply_scene_param_overrides(RowScatterParams& params,
                                 const SceneParamOverrides& overrides);
