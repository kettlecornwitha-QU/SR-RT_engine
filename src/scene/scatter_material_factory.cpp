#include "scene/scatter_material_factory.h"

#include <algorithm>
#include <cstdint>
#include <memory>

#include "materials/anisotropic_ggx_metal.h"
#include "materials/clearcoat.h"
#include "materials/coated_plastic.h"
#include "materials/ggx_metal.h"
#include "materials/lambertian.h"
#include "materials/sheen.h"
#include "materials/subsurface.h"
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

double clamp01(double x) {
    return std::max(0.0, std::min(1.0, x));
}

Vec3 scatter_mixv(const Vec3& a, const Vec3& b, double t) {
    return a * (1.0 - t) + b * t;
}

const Material* choose_random_surface_material(SceneBuild& s,
                                               const Vec3& base,
                                               const ScatterMaterialContext& ctx,
                                               int gx,
                                               int gz)
{
    double pick = scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialPick);
    if (pick < 0.38)
        return add_material<Lambertian>(s, base);

    if (pick < 0.52) {
        Vec3 tinted = scatter_mixv(Vec3{0.80, 0.80, 0.80}, base, 0.35);
        Vec3 f0 = {
            0.60 + 0.35 * clamp01(tinted.x),
            0.60 + 0.35 * clamp01(tinted.y),
            0.60 + 0.35 * clamp01(tinted.z)
        };
        return add_material<GGXMetal>(
            s,
            f0,
            0.06 + 0.22 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialGGXRoughness));
    }

    if (pick < 0.63) {
        Vec3 tinted = scatter_mixv(Vec3{0.78, 0.78, 0.78}, base, 0.30);
        Vec3 f0 = {
            0.58 + 0.37 * clamp01(tinted.x),
            0.58 + 0.37 * clamp01(tinted.y),
            0.58 + 0.37 * clamp01(tinted.z)
        };
        const double base_rough = 0.05 + 0.12 * scene::random::hash01(
                                                     gx,
                                                     gz,
                                                     scene::random::SeedChannel::MaterialAnisoBaseRoughness);
        const double anisotropy =
            (scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialAnisoSign) < 0.5 ? -1.0 : 1.0) *
            (0.55 + 0.35 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialAnisoAmount));
        const double rx = std::max(0.02, std::min(0.60, base_rough * (1.0 + anisotropy)));
        const double ry = std::max(0.02, std::min(0.60, base_rough * (1.0 - anisotropy)));
        const double tangent_angle = 2.0 * 3.14159265358979323846 * scene::random::hash01(
                                                                       gx,
                                                                       gz,
                                                                       scene::random::SeedChannel::MaterialAnisoTangentAngle);
        Vec3 tangent = {std::cos(tangent_angle), 0.0, std::sin(tangent_angle)};
        return add_material<AnisotropicGGXMetal>(
            s,
            f0,
            rx,
            ry,
            tangent);
    }

    if (pick < 0.72) {
        Vec3 tint = scatter_mixv(Vec3{1.0, 1.0, 1.0}, base, 0.45);
        return add_material<ThinTransmission>(
            s,
            tint,
            1.35 + 0.25 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialAnisoAmount),
            0.01 + 0.06 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialAnisoTangentAngle));
    }

    if (pick < 0.80) {
        return add_material<Sheen>(
            s,
            base * 0.35,
            base,
            0.45 + 0.40 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialSheenBlend));
    }

    if (pick < 0.88) {
        auto* coat_base = add_material<Lambertian>(s, base);
        return add_material<Clearcoat>(
            s,
            coat_base,
            1.40 + 0.18 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialClearcoatIOR),
            0.01 + 0.05 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialClearcoatRoughness));
    }

    if (pick < 0.94) {
        return add_material<CoatedPlastic>(
            s,
            base,
            1.42 + 0.18 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialCoatedPlasticIOR),
            0.015 + 0.11 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialCoatedPlasticRoughness),
            1.0);
    }

    if (pick < 0.98) {
        return add_material<Subsurface>(
            s,
            base,
            0.30 + 0.80 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialSubsurfaceMeanFreePath),
            0.65 + 0.25 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialSubsurfaceAnisotropy),
            0.50 + 0.35 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialSubsurfaceForwardness),
            ctx.use_random_walk_sss);
    }

    return ctx.glass;
}

ScatterMaterialChoice make_volume_choice(const Vec3& base, int gx, int gz) {
    ScatterMaterialChoice choice;
    choice.use_volume = true;
    choice.volume_albedo = scatter_mixv(Vec3{0.90, 0.92, 0.96}, base, 0.70);
    choice.volume_density =
        0.28 + 0.82 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialVolumeDensity);
    return choice;
}

} // namespace

ScatterMaterialChoice choose_scatter_material(SceneBuild& s,
                                              ScatterMaterialMode mode,
                                              const Vec3& base,
                                              const ScatterMaterialContext& ctx,
                                              int gx,
                                              int gz)
{
    if (mode == ScatterMaterialMode::IsotropicVolume)
        return make_volume_choice(base, gx, gz);

    if (mode == ScatterMaterialMode::Mixed) {
        ScatterMaterialChoice choice;
        choice.surface_material = choose_random_surface_material(s, base, ctx, gx, gz);
        double volume_pick = scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialMixedVolumeGate);
        if (volume_pick < 0.16)
            return make_volume_choice(base, gx, gz);
        return choice;
    }

    ScatterMaterialChoice choice;
    switch (mode) {
        case ScatterMaterialMode::Lambertian:
            choice.surface_material = add_material<Lambertian>(s, base);
            break;
        case ScatterMaterialMode::GGXMetal: {
            Vec3 tinted = scatter_mixv(Vec3{0.80, 0.80, 0.80}, base, 0.35);
            Vec3 f0 = {
                0.60 + 0.35 * clamp01(tinted.x),
                0.60 + 0.35 * clamp01(tinted.y),
                0.60 + 0.35 * clamp01(tinted.z)
            };
            choice.surface_material = add_material<GGXMetal>(
                s,
                f0,
                0.06 + 0.22 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialGGXRoughness));
            break;
        }
        case ScatterMaterialMode::AnisotropicGGXMetal: {
            Vec3 tinted = scatter_mixv(Vec3{0.78, 0.78, 0.78}, base, 0.30);
            Vec3 f0 = {
                0.58 + 0.37 * clamp01(tinted.x),
                0.58 + 0.37 * clamp01(tinted.y),
                0.58 + 0.37 * clamp01(tinted.z)
            };
            const double base_rough = 0.05 + 0.12 * scene::random::hash01(
                                                         gx,
                                                         gz,
                                                         scene::random::SeedChannel::MaterialAnisoBaseRoughness);
            const double anisotropy =
                (scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialAnisoSign) < 0.5 ? -1.0 : 1.0) *
                (0.55 + 0.35 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialAnisoAmount));
            const double rx = std::max(0.02, std::min(0.60, base_rough * (1.0 + anisotropy)));
            const double ry = std::max(0.02, std::min(0.60, base_rough * (1.0 - anisotropy)));
            const double tangent_angle = 2.0 * 3.14159265358979323846 * scene::random::hash01(
                                                                           gx,
                                                                           gz,
                                                                           scene::random::SeedChannel::MaterialAnisoTangentAngle);
            Vec3 tangent = {std::cos(tangent_angle), 0.0, std::sin(tangent_angle)};
            choice.surface_material = add_material<AnisotropicGGXMetal>(
                s,
                f0,
                rx,
                ry,
                tangent);
            break;
        }
        case ScatterMaterialMode::ThinTransmission: {
            Vec3 tint = scatter_mixv(Vec3{1.0, 1.0, 1.0}, base, 0.45);
            choice.surface_material = add_material<ThinTransmission>(
                s,
                tint,
                1.35 + 0.25 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialAnisoAmount),
                0.01 + 0.06 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialAnisoTangentAngle));
            break;
        }
        case ScatterMaterialMode::Sheen:
            choice.surface_material = add_material<Sheen>(
                s,
                base * 0.35,
                base,
                0.45 + 0.40 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialSheenBlend));
            break;
        case ScatterMaterialMode::Clearcoat: {
            auto* coat_base = add_material<Lambertian>(s, base);
            choice.surface_material = add_material<Clearcoat>(
                s,
                coat_base,
                1.40 + 0.18 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialClearcoatIOR),
                0.01 + 0.05 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialClearcoatRoughness));
            break;
        }
        case ScatterMaterialMode::Dielectric:
            choice.surface_material = ctx.glass;
            break;
        case ScatterMaterialMode::CoatedPlastic:
            choice.surface_material = add_material<CoatedPlastic>(
                s,
                base,
                1.42 + 0.18 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialCoatedPlasticIOR),
                0.015 + 0.11 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialCoatedPlasticRoughness),
                1.0);
            break;
        case ScatterMaterialMode::Subsurface:
            choice.surface_material = add_material<Subsurface>(
                s,
                base,
                0.30 + 0.80 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialSubsurfaceMeanFreePath),
                0.65 + 0.25 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialSubsurfaceAnisotropy),
                0.50 + 0.35 * scene::random::hash01(gx, gz, scene::random::SeedChannel::MaterialSubsurfaceForwardness),
                ctx.use_random_walk_sss);
            break;
        case ScatterMaterialMode::ConductorCopper:
            choice.surface_material = ctx.conductor_copper;
            break;
        case ScatterMaterialMode::ConductorAluminum:
            choice.surface_material = ctx.conductor_aluminum;
            break;
        case ScatterMaterialMode::Mixed:
        case ScatterMaterialMode::IsotropicVolume:
            break;
    }
    return choice;
}
