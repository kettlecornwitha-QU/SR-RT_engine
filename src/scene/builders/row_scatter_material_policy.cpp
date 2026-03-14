#include "scene/builders/row_scatter_material_policy.h"

#include <algorithm>
#include <memory>

#include "materials/clearcoat.h"
#include "materials/coated_plastic.h"
#include "materials/conductor_metal.h"
#include "materials/dielectric.h"
#include "materials/ggx_metal.h"
#include "materials/lambertian.h"
#include "materials/thin_transmission.h"
#include "scene/random/hash.h"

namespace {

template <typename T, typename... Args>
T* add_material(SceneBuild& s, Args&&... args) {
    auto mat = std::make_shared<T>(std::forward<Args>(args)...);
    T* raw = mat.get();
    s.materials.push_back(mat);
    return raw;
}

} // namespace

RowScatterMaterialPolicy make_row_scatter_material_policy(const RowScatterParams& params) {
    RowScatterMaterialPolicy policy;
    const double w_lambertian = std::max(0.0, params.lambertian_weight);
    const double w_clearcoat = std::max(0.0, params.clearcoat_weight);
    const double w_coated = std::max(0.0, params.coated_plastic_weight);
    const double weight_sum = w_lambertian + w_clearcoat + w_coated;
    if (weight_sum > 0.0) {
        policy.lambertian_threshold = w_lambertian / weight_sum;
        policy.clearcoat_threshold = (w_lambertian + w_clearcoat) / weight_sum;
    }
    return policy;
}

RowScatterRowMaterialType pick_row_scatter_row_material_type(
    int row,
    const RowScatterMaterialPolicy& policy)
{
    const double row_pick =
        scene::random::hash01(row, 0, scene::random::SeedChannel::RowScatterMaterialBandPick);
    if (row_pick < policy.lambertian_threshold) {
        return RowScatterRowMaterialType::Lambertian;
    }
    if (row_pick < policy.clearcoat_threshold) {
        return RowScatterRowMaterialType::Clearcoat;
    }
    return RowScatterRowMaterialType::CoatedPlastic;
}

const Material* make_row_scatter_material_for_slot(
    SceneBuild& s,
    const RowScatterParams& params,
    const RowScatterMaterialPolicy& policy,
    int row,
    int col,
    const Vec3& base_color)
{
    const bool override_material =
        scene::random::hash01(row, col, scene::random::SeedChannel::RowScatterOverrideGate) <
        params.replacement_rate;

    if (override_material) {
        const double noise_pick =
            scene::random::hash01(row, col, scene::random::SeedChannel::RowScatterOverridePick);
        if (noise_pick < 0.20) {
            Vec3 tint = (Vec3{1.0, 1.0, 1.0} * 0.60) + (base_color * 0.40);
            return add_material<ThinTransmission>(
                s,
                tint,
                1.35 + 0.20 * scene::random::hash01(row, col, scene::random::SeedChannel::RowScatterThinIOR),
                0.01 + 0.05 *
                           scene::random::hash01(row, col, scene::random::SeedChannel::RowScatterThinRoughness));
        }
        if (noise_pick < 0.40) {
            return add_material<Dielectric>(
                s,
                1.45 + 0.15 *
                           scene::random::hash01(row, col, scene::random::SeedChannel::RowScatterDielectricIOR));
        }
        if (noise_pick < 0.60) {
            return add_material<ConductorMetal>(
                s,
                Vec3{0.271, 0.675, 1.316},
                Vec3{3.609, 2.624, 2.292},
                0.08 + 0.08 *
                           scene::random::hash01(row, col, scene::random::SeedChannel::RowScatterCopperRoughness));
        }
        if (noise_pick < 0.80) {
            return add_material<ConductorMetal>(
                s,
                Vec3{1.50, 0.98, 0.62},
                Vec3{7.30, 6.60, 5.40},
                0.10 + 0.10 * scene::random::hash01(
                                   row,
                                   col,
                                   scene::random::SeedChannel::RowScatterAluminumRoughness));
        }
        Vec3 f0 = (Vec3{0.58, 0.58, 0.58} * 0.65) + (base_color * 0.35);
        return add_material<GGXMetal>(
            s,
            f0,
            0.26 + 0.06 *
                       scene::random::hash01(row, col, scene::random::SeedChannel::RowScatterGGXRoughness));
    }

    switch (pick_row_scatter_row_material_type(row, policy)) {
        case RowScatterRowMaterialType::Lambertian:
            return add_material<Lambertian>(s, base_color);
        case RowScatterRowMaterialType::Clearcoat: {
            auto* coat_base = add_material<Lambertian>(s, base_color);
            return add_material<Clearcoat>(
                s,
                coat_base,
                1.42 + 0.16 *
                           scene::random::hash01(row, col, scene::random::SeedChannel::RowScatterClearcoatIOR),
                0.01 + 0.06 * scene::random::hash01(
                                  row,
                                  col,
                                  scene::random::SeedChannel::RowScatterClearcoatRoughness));
        }
        case RowScatterRowMaterialType::CoatedPlastic:
            return add_material<CoatedPlastic>(
                s,
                base_color,
                1.42 + 0.14 *
                           scene::random::hash01(row, col, scene::random::SeedChannel::RowScatterCoatedPlasticIOR),
                0.015 + 0.10 * scene::random::hash01(
                                   row,
                                   col,
                                   scene::random::SeedChannel::RowScatterCoatedPlasticRoughness),
                1.0);
    }
    return add_material<Lambertian>(s, base_color);
}
