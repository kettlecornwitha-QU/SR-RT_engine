#include "scene/scene_presets.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <utility>

#include "geometry/plane.h"
#include "geometry/rectangle.h"
#include "geometry/sphere.h"
#include "geometry/constant_medium.h"
#include "materials/anisotropic_ggx_metal.h"
#include "materials/checker_lambertian.h"
#include "materials/clearcoat.h"
#include "materials/coated_plastic.h"
#include "materials/conductor_metal.h"
#include "materials/dielectric.h"
#include "materials/emissive.h"
#include "materials/ggx_metal.h"
#include "materials/isotropic.h"
#include "materials/lambertian.h"
#include "materials/sheen.h"
#include "materials/subsurface.h"
#include "materials/thin_transmission.h"
#include "scene/model_loader.h"
#include "scene/builders/big_scatter_builder.h"
#include "scene/builders/row_scatter_builder.h"
#include "scene/builders/scatter_builder.h"
#include "scene/presets/scene_configs.h"
#include "scene/presets/scene_params.h"
#include "scene/polyhedra_builder.h"
#include "scene/scatter_material_factory.h"

namespace {

constexpr double kPi = 3.14159265358979323846;

enum class StandardSceneId {
    Spheres,
    Metalglass,
    Materials,
    MaterialsCoatedPlastic,
    MaterialsSSS,
    MaterialsMetalLab,
    MaterialsGGXLab,
    GgxRoughnessRamp,
    PolyhedraLab,
    Scatter,
    ScatterLambertian,
    ScatterGGXMetal,
    ScatterAnisotropicGGXMetal,
    ScatterThinTransmission,
    ScatterSheen,
    ScatterClearcoat,
    ScatterDielectric,
    ScatterCoatedPlastic,
    ScatterSubsurface,
    ScatterConductorCopper,
    ScatterConductorAluminum,
    ScatterIsotropicVolume,
    BigScatter,
    BigScatterLambertian,
    BigScatterGGXMetal,
    BigScatterAnisotropicGGXMetal,
    BigScatterThinTransmission,
    BigScatterSheen,
    BigScatterClearcoat,
    BigScatterDielectric,
    BigScatterCoatedPlastic,
    BigScatterSubsurface,
    BigScatterConductorCopper,
    BigScatterConductorAluminum,
    BigScatterIsotropicVolume,
    RowScatter,
    Cornell,
    Benchmark,
    Volumes
};

enum class ModelSceneId {
    Sponza
};

struct StandardSceneEntry {
    const char* name;
    StandardSceneId id;
    bool alias;
};

struct ModelSceneEntry {
    const char* name;
    ModelSceneId id;
};

const std::vector<StandardSceneEntry>& standard_scene_entries();
const std::vector<ModelSceneEntry>& model_scene_entries();

template <typename T, typename... Args>
T* add_material(SceneBuild& s, Args&&... args) {
    auto mat = std::make_shared<T>(std::forward<Args>(args)...);
    T* raw = mat.get();
    s.materials.push_back(mat);
    return raw;
}

void apply_preset_config(const ScenePresetConfig& preset,
                         SceneBuild& s,
                         RenderSettings& settings)
{
    if (preset.apply_render_settings)
        settings = preset.render_settings;
    if (preset.apply_camera)
        s.camera = preset.camera;
    s.point_lights.insert(s.point_lights.end(),
                          preset.point_lights.begin(),
                          preset.point_lights.end());
    s.area_lights.insert(s.area_lights.end(),
                         preset.area_lights.begin(),
                         preset.area_lights.end());
}

ModelScenePreset make_sponza_model_preset() {
    ModelScenePreset p;
    p.render_settings.image_width = 1280;
    p.render_settings.image_height = 720;
    p.render_settings.samples_per_pixel = 8;
    p.render_settings.max_depth = 8;
    p.render_settings.aperture = 0.0;
    p.render_settings.focus_distance = 10.0;
    p.render_settings.exposure = 1.8;
    p.render_settings.adaptive_sampling = true;
    p.render_settings.adaptive_min_spp = 8;
    p.render_settings.adaptive_threshold = 0.01;

    p.camera.origin = {0.0, 2.0, 8.0};
    p.camera.forward = {0.0, -0.05, -1.0};
    p.camera.half_fov_x = kPi / 4.0;

    p.point_lights.push_back({Vec3{0.0, 4.5, -3.0}, Vec3{3200.0, 2900.0, 2600.0}});
    p.point_lights.push_back({Vec3{-4.5, 4.0, -7.5}, Vec3{2600.0, 2400.0, 2200.0}});
    p.point_lights.push_back({Vec3{4.5, 4.0, -7.5}, Vec3{2600.0, 2400.0, 2200.0}});
    p.point_lights.push_back({Vec3{0.0, 4.0, -12.5}, Vec3{2400.0, 2500.0, 2800.0}});

    p.area_lights.push_back({
        Vec3{0.0, 5.2, -7.0},
        Vec3{4.0, 0.0, 0.0},
        Vec3{0.0, 0.0, 3.0},
        Vec3{42.0, 38.0, 34.0}
    });
    p.area_lights.push_back({
        Vec3{0.0, 5.2, -12.0},
        Vec3{4.0, 0.0, 0.0},
        Vec3{0.0, 0.0, 3.0},
        Vec3{36.0, 40.0, 48.0}
    });

    p.model_candidates = {
        "assets/sponza/sponza.obj",
        "assets/Sponza/sponza.obj",
        "Sponza/sponza.obj",
        "sponza.obj",
        "assets/sponza/sponza.gltf",
        "assets/Sponza/sponza.gltf",
        "Sponza/sponza.gltf",
        "sponza.gltf",
        "assets/sponza/NewSponza_Main_glTF_003.gltf",
        "assets/Sponza/NewSponza_Main_glTF_003.gltf",
        "Sponza/NewSponza_Main_glTF_003.gltf",
        "NewSponza_Main_glTF_003.gltf",
        "assets/sponza/main_sponza/NewSponza_Main_glTF_003.gltf",
        "assets/Sponza/main_sponza/NewSponza_Main_glTF_003.gltf"
    };
    return p;
}

const std::unordered_map<std::string, StandardSceneId>& standard_scene_registry() {
    static const std::unordered_map<std::string, StandardSceneId> kRegistry = [] {
        std::unordered_map<std::string, StandardSceneId> r;
        for (const auto& entry : standard_scene_entries())
            r.emplace(entry.name, entry.id);
        return r;
    }();

    return kRegistry;
}

bool scatter_mode_for_scene(StandardSceneId scene_id,
                            ScatterMaterialMode& mode) {
    switch (scene_id) {
        case StandardSceneId::Scatter: mode = ScatterMaterialMode::Mixed; return true;
        case StandardSceneId::ScatterLambertian: mode = ScatterMaterialMode::Lambertian; return true;
        case StandardSceneId::ScatterGGXMetal: mode = ScatterMaterialMode::GGXMetal; return true;
        case StandardSceneId::ScatterAnisotropicGGXMetal: mode = ScatterMaterialMode::AnisotropicGGXMetal; return true;
        case StandardSceneId::ScatterThinTransmission: mode = ScatterMaterialMode::ThinTransmission; return true;
        case StandardSceneId::ScatterSheen: mode = ScatterMaterialMode::Sheen; return true;
        case StandardSceneId::ScatterClearcoat: mode = ScatterMaterialMode::Clearcoat; return true;
        case StandardSceneId::ScatterDielectric: mode = ScatterMaterialMode::Dielectric; return true;
        case StandardSceneId::ScatterCoatedPlastic: mode = ScatterMaterialMode::CoatedPlastic; return true;
        case StandardSceneId::ScatterSubsurface: mode = ScatterMaterialMode::Subsurface; return true;
        case StandardSceneId::ScatterConductorCopper: mode = ScatterMaterialMode::ConductorCopper; return true;
        case StandardSceneId::ScatterConductorAluminum: mode = ScatterMaterialMode::ConductorAluminum; return true;
        case StandardSceneId::ScatterIsotropicVolume: mode = ScatterMaterialMode::IsotropicVolume; return true;
        default: return false;
    }
}

bool big_scatter_mode_for_scene(StandardSceneId scene_id,
                                ScatterMaterialMode& mode) {
    switch (scene_id) {
        case StandardSceneId::BigScatter: mode = ScatterMaterialMode::Mixed; return true;
        case StandardSceneId::BigScatterLambertian: mode = ScatterMaterialMode::Lambertian; return true;
        case StandardSceneId::BigScatterGGXMetal: mode = ScatterMaterialMode::GGXMetal; return true;
        case StandardSceneId::BigScatterAnisotropicGGXMetal: mode = ScatterMaterialMode::AnisotropicGGXMetal; return true;
        case StandardSceneId::BigScatterThinTransmission: mode = ScatterMaterialMode::ThinTransmission; return true;
        case StandardSceneId::BigScatterSheen: mode = ScatterMaterialMode::Sheen; return true;
        case StandardSceneId::BigScatterClearcoat: mode = ScatterMaterialMode::Clearcoat; return true;
        case StandardSceneId::BigScatterDielectric: mode = ScatterMaterialMode::Dielectric; return true;
        case StandardSceneId::BigScatterCoatedPlastic: mode = ScatterMaterialMode::CoatedPlastic; return true;
        case StandardSceneId::BigScatterSubsurface: mode = ScatterMaterialMode::Subsurface; return true;
        case StandardSceneId::BigScatterConductorCopper: mode = ScatterMaterialMode::ConductorCopper; return true;
        case StandardSceneId::BigScatterConductorAluminum: mode = ScatterMaterialMode::ConductorAluminum; return true;
        case StandardSceneId::BigScatterIsotropicVolume: mode = ScatterMaterialMode::IsotropicVolume; return true;
        default: return false;
    }
}

const std::unordered_map<std::string, ModelSceneId>& model_scene_registry() {
    static const std::unordered_map<std::string, ModelSceneId> kRegistry = [] {
        std::unordered_map<std::string, ModelSceneId> r;
        for (const auto& entry : model_scene_entries())
            r.emplace(entry.name, entry.id);
        return r;
    }();

    return kRegistry;
}

const std::vector<StandardSceneEntry>& standard_scene_entries() {
    static const std::vector<StandardSceneEntry> kEntries = {
        {"spheres", StandardSceneId::Spheres, false},
        {"metalglass", StandardSceneId::Metalglass, false},
        {"materials", StandardSceneId::Materials, false},
        {"materials_coated_plastic", StandardSceneId::MaterialsCoatedPlastic, false},
        {"materials_coat", StandardSceneId::MaterialsCoatedPlastic, true},
        {"materials_sss", StandardSceneId::MaterialsSSS, false},
        {"materials_metal_lab", StandardSceneId::MaterialsMetalLab, false},
        {"materials_ggx_lab", StandardSceneId::MaterialsGGXLab, false},
        {"ggx_roughness_ramp", StandardSceneId::GgxRoughnessRamp, false},
        {"roughness_ramp", StandardSceneId::GgxRoughnessRamp, true},
        {"polyhedra_lab", StandardSceneId::PolyhedraLab, false},
        {"scatter", StandardSceneId::Scatter, false},
        {"scatter_mixed", StandardSceneId::Scatter, true},
        {"scatter_lambertian", StandardSceneId::ScatterLambertian, false},
        {"scatter_ggx_metal", StandardSceneId::ScatterGGXMetal, false},
        {"scatter_metal", StandardSceneId::ScatterGGXMetal, true},
        {"scatter_anisotropic_ggx_metal", StandardSceneId::ScatterAnisotropicGGXMetal, false},
        {"scatter_aniso", StandardSceneId::ScatterAnisotropicGGXMetal, true},
        {"scatter_thin_transmission", StandardSceneId::ScatterThinTransmission, false},
        {"scatter_thin", StandardSceneId::ScatterThinTransmission, true},
        {"scatter_sheen", StandardSceneId::ScatterSheen, false},
        {"scatter_clearcoat", StandardSceneId::ScatterClearcoat, false},
        {"scatter_dielectric", StandardSceneId::ScatterDielectric, false},
        {"scatter_glass", StandardSceneId::ScatterDielectric, true},
        {"scatter_coated_plastic", StandardSceneId::ScatterCoatedPlastic, false},
        {"scatter_coated", StandardSceneId::ScatterCoatedPlastic, true},
        {"scatter_subsurface", StandardSceneId::ScatterSubsurface, false},
        {"scatter_sss", StandardSceneId::ScatterSubsurface, true},
        {"scatter_conductor_copper", StandardSceneId::ScatterConductorCopper, false},
        {"scatter_copper", StandardSceneId::ScatterConductorCopper, true},
        {"scatter_conductor_aluminum", StandardSceneId::ScatterConductorAluminum, false},
        {"scatter_aluminum", StandardSceneId::ScatterConductorAluminum, true},
        {"scatter_isotropic_volume", StandardSceneId::ScatterIsotropicVolume, false},
        {"scatter_volume", StandardSceneId::ScatterIsotropicVolume, true},
        {"big_scatter", StandardSceneId::BigScatter, false},
        {"big_scatter_mixed", StandardSceneId::BigScatter, true},
        {"big_scatter_lambertian", StandardSceneId::BigScatterLambertian, false},
        {"big_scatter_ggx_metal", StandardSceneId::BigScatterGGXMetal, false},
        {"big_scatter_metal", StandardSceneId::BigScatterGGXMetal, true},
        {"big_scatter_anisotropic_ggx_metal", StandardSceneId::BigScatterAnisotropicGGXMetal, false},
        {"big_scatter_aniso", StandardSceneId::BigScatterAnisotropicGGXMetal, true},
        {"big_scatter_thin_transmission", StandardSceneId::BigScatterThinTransmission, false},
        {"big_scatter_thin", StandardSceneId::BigScatterThinTransmission, true},
        {"big_scatter_sheen", StandardSceneId::BigScatterSheen, false},
        {"big_scatter_clearcoat", StandardSceneId::BigScatterClearcoat, false},
        {"big_scatter_dielectric", StandardSceneId::BigScatterDielectric, false},
        {"big_scatter_glass", StandardSceneId::BigScatterDielectric, true},
        {"big_scatter_coated_plastic", StandardSceneId::BigScatterCoatedPlastic, false},
        {"big_scatter_coated", StandardSceneId::BigScatterCoatedPlastic, true},
        {"big_scatter_subsurface", StandardSceneId::BigScatterSubsurface, false},
        {"big_scatter_sss", StandardSceneId::BigScatterSubsurface, true},
        {"big_scatter_conductor_copper", StandardSceneId::BigScatterConductorCopper, false},
        {"big_scatter_copper", StandardSceneId::BigScatterConductorCopper, true},
        {"big_scatter_conductor_aluminum", StandardSceneId::BigScatterConductorAluminum, false},
        {"big_scatter_aluminum", StandardSceneId::BigScatterConductorAluminum, true},
        {"big_scatter_isotropic_volume", StandardSceneId::BigScatterIsotropicVolume, false},
        {"big_scatter_volume", StandardSceneId::BigScatterIsotropicVolume, true},
        {"row_scatter", StandardSceneId::RowScatter, false},
        {"cornell", StandardSceneId::Cornell, false},
        {"benchmark", StandardSceneId::Benchmark, false},
        {"volumes", StandardSceneId::Volumes, false},
    };
    return kEntries;
}

const std::vector<ModelSceneEntry>& model_scene_entries() {
    static const std::vector<ModelSceneEntry> kEntries = {
        {"sponza", ModelSceneId::Sponza},
    };
    return kEntries;
}

} // namespace

bool build_standard_scene_by_name(const std::string& name,
                                  SceneBuild& s,
                                  RenderSettings& settings,
                                  const SceneParamOverrides& scene_param_overrides)
{
    const auto it = standard_scene_registry().find(name);
    if (it == standard_scene_registry().end())
        return false;

    const StandardSceneId scene_id = it->second;

    if (scene_id == StandardSceneId::Spheres) {
        auto* ground = add_material<Lambertian>(s, Vec3{0.2, 0.8, 0.2});
        auto* red = add_material<Lambertian>(s, Vec3{0.9, 0.3, 0.3});
        auto* glass = add_material<Dielectric>(s, 1.5);
        auto* metal = add_material<GGXMetal>(s, Vec3{0.95, 0.78, 0.32}, 0.25);
        auto* lamp = add_material<Emissive>(s, Vec3{6.0, 5.5, 4.5});
        auto* area_emissive = add_material<Emissive>(s, Vec3{8.0, 8.0, 8.0});

        s.world.add(std::make_shared<Sphere>(Vec3{0, 0, -3}, 0.5, glass));
        s.world.add(std::make_shared<Sphere>(Vec3{-1, 0, -4}, 0.7, red));
        s.world.add(std::make_shared<Sphere>(Vec3{1.0, -0.05, -4.2}, 0.45, metal));
        s.world.add(std::make_shared<Sphere>(Vec3{-2.0, 1.3, -3.1}, 0.25, lamp));
        s.world.add(std::make_shared<Rectangle>(
            Vec3{0.0, 2.2, -3.4},
            Vec3{1.2, 0.0, 0.0},
            Vec3{0.0, 0.0, 1.2},
            area_emissive));
        s.world.add(std::make_shared<Plane>(Vec3{0, -0.5, 0}, Vec3{0, 1, 0}, ground));

        s.point_lights.push_back({Vec3{2.0, 3.0, -1.5}, Vec3{30.0, 30.0, 30.0}});
        s.area_lights.push_back({
            Vec3{0.0, 2.2, -3.4},
            Vec3{1.2, 0.0, 0.0},
            Vec3{0.0, 0.0, 1.2},
            Vec3{8.0, 8.0, 8.0}
        });

        if (scene_file_exists("model.obj"))
            (void)load_model_scene("model.obj", s, red, settings.gltf_triangle_step);
        return true;
    }

    if (scene_id == StandardSceneId::Metalglass) {
        auto* ground = add_material<Lambertian>(s, Vec3{0.65, 0.65, 0.65});
        auto* glass = add_material<Dielectric>(s, 1.5);
        auto* gold = add_material<GGXMetal>(s, Vec3{0.95, 0.78, 0.32}, 0.18);
        auto* chrome = add_material<GGXMetal>(s, Vec3{0.90, 0.90, 0.92}, 0.06);
        auto* area_emissive = add_material<Emissive>(s, Vec3{10.0, 10.0, 10.0});

        s.world.add(std::make_shared<Plane>(Vec3{0,-0.5,0}, Vec3{0,1,0}, ground));
        s.world.add(std::make_shared<Sphere>(Vec3{-0.8,0.0,-3.2}, 0.5, glass));
        s.world.add(std::make_shared<Sphere>(Vec3{0.7,0.0,-3.5}, 0.5, gold));
        s.world.add(std::make_shared<Sphere>(Vec3{0.0,0.6,-4.3}, 0.35, chrome));
        s.world.add(std::make_shared<Rectangle>(
            Vec3{0.0, 2.0, -3.3},
            Vec3{1.3, 0.0, 0.0},
            Vec3{0.0, 0.0, 1.0},
            area_emissive));

        s.area_lights.push_back({
            Vec3{0.0, 2.0, -3.3},
            Vec3{1.3, 0.0, 0.0},
            Vec3{0.0, 0.0, 1.0},
            Vec3{10.0, 10.0, 10.0}
        });
        return true;
    }

    if (scene_id == StandardSceneId::Materials) {
        apply_preset_config(make_materials_config(), s, settings);

        auto* ground = add_material<Lambertian>(s, Vec3{0.55, 0.55, 0.58});
        auto* diffuse_base = add_material<Lambertian>(s, Vec3{0.82, 0.16, 0.14});
        auto* clearcoat_red = add_material<Clearcoat>(s, diffuse_base, 1.5, 0.03);
        auto* aniso_gold = add_material<AnisotropicGGXMetal>(s, Vec3{0.95, 0.78, 0.32}, 0.08, 0.38, Vec3{1, 0, 0});
        auto* aniso_steel = add_material<AnisotropicGGXMetal>(s, Vec3{0.86, 0.88, 0.90}, 0.30, 0.06, Vec3{0, 0, 1});
        auto* chrome = add_material<GGXMetal>(s, Vec3{0.92, 0.92, 0.93}, 0.05);
        auto* conductor_copper = add_material<ConductorMetal>(
            s,
            Vec3{0.271, 0.675, 1.316},
            Vec3{3.609, 2.624, 2.292},
            0.08);
        auto* conductor_aluminum = add_material<ConductorMetal>(
            s,
            Vec3{1.50, 0.98, 0.62},
            Vec3{7.30, 6.60, 5.40},
            0.12);
        auto* coated_plastic = add_material<CoatedPlastic>(s, Vec3{0.14, 0.45, 0.86}, 1.5, 0.035, 1.0);
        auto* glass = add_material<Dielectric>(s, 1.5);
        auto* thin_blue = add_material<ThinTransmission>(s, Vec3{0.70, 0.85, 1.0}, 1.45, 0.02);
        auto* fabric = add_material<Sheen>(s, Vec3{0.25, 0.20, 0.18}, Vec3{0.95, 0.55, 0.35}, 0.55);
        auto* area_emissive = add_material<Emissive>(s, Vec3{14.0, 13.0, 12.0});

        s.world.add(std::make_shared<Plane>(Vec3{0,-0.5,0}, Vec3{0,1,0}, ground));

        if (settings.materials_variant == RenderSettings::MaterialsAll) {
            s.world.add(std::make_shared<Sphere>(Vec3{-1.8, 0.0, -3.4}, 0.5, aniso_gold));
            s.world.add(std::make_shared<Sphere>(Vec3{-0.6, 0.0, -3.2}, 0.5, aniso_steel));
            s.world.add(std::make_shared<Sphere>(Vec3{0.6, 0.0, -3.2}, 0.5, clearcoat_red));
            s.world.add(std::make_shared<Sphere>(Vec3{1.8, 0.0, -3.4}, 0.5, conductor_aluminum));
            s.world.add(std::make_shared<Sphere>(Vec3{0.0, 0.25, -4.6}, 0.75, glass));
            s.world.add(std::make_shared<Sphere>(Vec3{-2.6, 0.0, -4.8}, 0.45, thin_blue));
            s.world.add(std::make_shared<Sphere>(Vec3{2.6, 0.0, -4.8}, 0.45, fabric));
            s.world.add(std::make_shared<Sphere>(Vec3{0.0, -0.05, -2.35}, 0.35, conductor_copper));
            s.world.add(std::make_shared<Sphere>(Vec3{0.0, 0.05, -2.95}, 0.38, coated_plastic));
        } else {
            const Material* hero = chrome;
            switch (settings.materials_variant) {
                case RenderSettings::MaterialsAnisoGold: hero = aniso_gold; break;
                case RenderSettings::MaterialsAnisoSteel: hero = aniso_steel; break;
                case RenderSettings::MaterialsClearcoat: hero = clearcoat_red; break;
                case RenderSettings::MaterialsChrome: hero = chrome; break;
                case RenderSettings::MaterialsGlass: hero = glass; break;
                case RenderSettings::MaterialsThinTransmission: hero = thin_blue; break;
                case RenderSettings::MaterialsSheen: hero = fabric; break;
                case RenderSettings::MaterialsConductorCopper: hero = conductor_copper; break;
                case RenderSettings::MaterialsConductorAluminum: hero = conductor_aluminum; break;
                case RenderSettings::MaterialsCoatedPlastic: hero = coated_plastic; break;
                default: break;
            }
            s.world.add(std::make_shared<Sphere>(Vec3{0.0, 0.0, -3.5}, 0.9, hero));
            s.world.add(std::make_shared<Sphere>(Vec3{-2.2, -0.15, -4.8}, 0.35, ground));
        }

        s.world.add(std::make_shared<Rectangle>(
            Vec3{0.0, 2.8, -3.6},
            Vec3{2.4, 0.0, 0.0},
            Vec3{0.0, 0.0, 1.2},
            area_emissive));
        return true;
    }

    if (scene_id == StandardSceneId::MaterialsCoatedPlastic) {
        apply_preset_config(make_materials_config(), s, settings);

        settings.image_width = 1920;
        settings.image_height = 1080;
        settings.samples_per_pixel = 128;
        settings.max_depth = 5;
        settings.focus_distance = 4.0;
        settings.exposure = 1.0;
        settings.adaptive_sampling = true;
        settings.adaptive_min_spp = 8;
        settings.adaptive_threshold = 0.01;
        settings.aperture = 0.0;

        s.camera.origin = {0.0, 1.35, 5.5};
        s.camera.forward = {0.0, -0.16, -1.0};
        s.camera.half_fov_x = kPi / 5.2;

        auto* ground = add_material<Lambertian>(s, Vec3{0.54, 0.54, 0.56});
        auto* area_emissive = add_material<Emissive>(s, Vec3{14.0, 13.0, 12.0});

        s.world.add(std::make_shared<Plane>(Vec3{0, -0.5, 0}, Vec3{0, 1, 0}, ground));

        const double ior_values[] = {1.30, 1.45, 1.60, 1.75};
        const double roughness_values[] = {0.005, 0.02, 0.06, 0.12, 0.22};
        const int ior_count = static_cast<int>(sizeof(ior_values) / sizeof(ior_values[0]));
        const int rough_count = static_cast<int>(sizeof(roughness_values) / sizeof(roughness_values[0]));
        const double x_spacing = 1.10;
        const double z_spacing = 1.10;
        const double x_origin = -0.5 * x_spacing * (ior_count - 1);
        const double z_front = -2.4;
        const double radius = 0.38;

        for (int rz = 0; rz < rough_count; ++rz) {
            for (int cx = 0; cx < ior_count; ++cx) {
                const double ior = ior_values[cx];
                const double rough = roughness_values[rz];
                const double t = (rough_count > 1) ? (static_cast<double>(rz) / (rough_count - 1)) : 0.0;
                Vec3 base = {
                    0.10 + 0.05 * t,
                    0.35 + 0.20 * t,
                    0.82 + 0.10 * t
                };
                auto* coated = add_material<CoatedPlastic>(s, base, ior, rough, 1.0);
                Vec3 c = {
                    x_origin + x_spacing * cx,
                    -0.5 + radius,
                    z_front - z_spacing * rz
                };
                s.world.add(std::make_shared<Sphere>(c, radius, coated));
            }
        }

        s.world.add(std::make_shared<Rectangle>(
            Vec3{0.0, 2.9, -4.8},
            Vec3{3.0, 0.0, 0.0},
            Vec3{0.0, 0.0, 2.4},
            area_emissive));
        return true;
    }

    if (scene_id == StandardSceneId::MaterialsSSS) {
        settings.image_width = 1920;
        settings.image_height = 1080;
        settings.samples_per_pixel = 128;
        settings.max_depth = 5;
        settings.focus_distance = 4.0;
        settings.exposure = 1.0;
        settings.adaptive_sampling = true;
        settings.adaptive_min_spp = 8;
        settings.adaptive_threshold = 0.01;
        settings.aperture = 0.0;

        s.camera.origin = {0.0, 1.0, 6.4};
        s.camera.forward = {0.0, -0.08, -1.0};
        s.camera.half_fov_x = kPi / 5.0;

        auto* ground = add_material<Lambertian>(s, Vec3{0.58, 0.58, 0.60});
        const bool use_random_walk_sss = settings.sss_random_walk;
        auto* wax = add_material<Subsurface>(s, Vec3{0.95, 0.60, 0.42}, 0.55, 0.82, 0.72, use_random_walk_sss);
        auto* milk = add_material<Subsurface>(s, Vec3{0.95, 0.96, 1.00}, 0.95, 0.70, 0.55, use_random_walk_sss);
        auto* marble = add_material<Subsurface>(s, Vec3{0.78, 0.86, 0.98}, 0.40, 0.86, 0.78, use_random_walk_sss);
        auto* skin_like = add_material<Subsurface>(s, Vec3{0.92, 0.62, 0.55}, 0.48, 0.80, 0.68, use_random_walk_sss);
        auto* reference_lambert = add_material<Lambertian>(s, Vec3{0.95, 0.60, 0.42});
        auto* area_emissive = add_material<Emissive>(s, Vec3{11.0, 10.5, 10.0});

        s.world.add(std::make_shared<Plane>(Vec3{0, -0.5, 0}, Vec3{0, 1, 0}, ground));
        s.world.add(std::make_shared<Sphere>(Vec3{-2.1, 0.35, -3.0}, 0.85, wax));
        s.world.add(std::make_shared<Sphere>(Vec3{-0.7, 0.35, -3.5}, 0.85, milk));
        s.world.add(std::make_shared<Sphere>(Vec3{0.7, 0.35, -3.5}, 0.85, marble));
        s.world.add(std::make_shared<Sphere>(Vec3{2.1, 0.35, -3.0}, 0.85, skin_like));
        s.world.add(std::make_shared<Sphere>(Vec3{0.0, 0.05, -2.2}, 0.45, reference_lambert));

        s.world.add(std::make_shared<Rectangle>(
            Vec3{0.0, 3.0, -5.8},
            Vec3{3.6, 0.0, 0.0},
            Vec3{0.0, 0.0, 2.8},
            area_emissive));
        s.point_lights.push_back({Vec3{-3.8, 2.0, -2.2}, Vec3{38.0, 34.0, 30.0}});
        s.point_lights.push_back({Vec3{3.8, 2.0, -2.2}, Vec3{38.0, 34.0, 30.0}});
        s.point_lights.push_back({Vec3{0.0, 1.2, -8.4}, Vec3{115.0, 102.0, 95.0}});
        return true;
    }

    if (scene_id == StandardSceneId::MaterialsMetalLab) {
        settings.image_width = 1920;
        settings.image_height = 1080;
        settings.samples_per_pixel = 128;
        settings.max_depth = 5;
        settings.focus_distance = 4.0;
        settings.exposure = 1.0;
        settings.adaptive_sampling = true;
        settings.adaptive_min_spp = 8;
        settings.adaptive_threshold = 0.01;
        settings.aperture = 0.0;

        s.camera.origin = {0.0, 1.2, 7.2};
        s.camera.forward = {0.0, -0.10, -1.0};
        s.camera.half_fov_x = kPi / 5.0;

        auto* ground = add_material<Lambertian>(s, Vec3{0.56, 0.56, 0.58});
        auto* wall = add_material<Lambertian>(s, Vec3{0.22, 0.22, 0.24});
        auto* area_emissive = add_material<Emissive>(s, Vec3{12.0, 12.0, 11.5});
        auto* rim_emissive = add_material<Emissive>(s, Vec3{8.5, 8.5, 9.5});

        // Isotropic GGX row (top).
        auto* iso_rough_low = add_material<GGXMetal>(s, Vec3{0.92, 0.91, 0.90}, 0.06);
        auto* iso_rough_med = add_material<GGXMetal>(s, Vec3{0.92, 0.91, 0.90}, 0.18);
        auto* iso_rough_high = add_material<GGXMetal>(s, Vec3{0.92, 0.91, 0.90}, 0.36);

        // Anisotropic GGX row (bottom), same base metal but directional roughness.
        auto* aniso_x = add_material<AnisotropicGGXMetal>(s, Vec3{0.92, 0.91, 0.90}, 0.08, 0.34, Vec3{1, 0, 0});
        auto* aniso_z = add_material<AnisotropicGGXMetal>(s, Vec3{0.92, 0.91, 0.90}, 0.08, 0.34, Vec3{0, 0, 1});
        auto* aniso_strong = add_material<AnisotropicGGXMetal>(s, Vec3{0.92, 0.91, 0.90}, 0.05, 0.48, Vec3{1, 0, 0});

        s.world.add(std::make_shared<Plane>(Vec3{0, -0.5, 0}, Vec3{0, 1, 0}, ground));
        s.world.add(std::make_shared<Rectangle>(
            Vec3{0.0, 1.3, -7.4},
            Vec3{6.5, 0.0, 0.0},
            Vec3{0.0, 3.0, 0.0},
            wall));

        const double r = 0.58;
        s.world.add(std::make_shared<Sphere>(Vec3{-2.2, 1.15, -3.5}, r, iso_rough_low));
        s.world.add(std::make_shared<Sphere>(Vec3{0.0, 1.15, -3.5}, r, iso_rough_med));
        s.world.add(std::make_shared<Sphere>(Vec3{2.2, 1.15, -3.5}, r, iso_rough_high));

        s.world.add(std::make_shared<Sphere>(Vec3{-2.2, 0.05, -4.8}, r, aniso_x));
        s.world.add(std::make_shared<Sphere>(Vec3{0.0, 0.05, -4.8}, r, aniso_z));
        s.world.add(std::make_shared<Sphere>(Vec3{2.2, 0.05, -4.8}, r, aniso_strong));

        // Soft overhead key.
        s.world.add(std::make_shared<Rectangle>(
            Vec3{0.0, 3.5, -4.2},
            Vec3{2.8, 0.0, 0.0},
            Vec3{0.0, 0.0, 2.1},
            area_emissive));
        // Tall side rim strip to reveal brushed directionality.
        s.world.add(std::make_shared<Rectangle>(
            Vec3{-3.5, 1.7, -4.2},
            Vec3{0.0, 2.0, 0.0},
            Vec3{0.0, 0.0, 1.6},
            rim_emissive));

        s.point_lights.push_back({Vec3{3.6, 2.1, -2.8}, Vec3{26.0, 25.0, 24.0}});
        s.point_lights.push_back({Vec3{-3.6, 2.0, -5.9}, Vec3{14.0, 14.0, 16.0}});
        return true;
    }

    if (scene_id == StandardSceneId::MaterialsGGXLab) {
        settings.image_width = 1920;
        settings.image_height = 1080;
        settings.samples_per_pixel = 128;
        settings.max_depth = 5;
        settings.focus_distance = 4.0;
        settings.exposure = 1.0;
        settings.adaptive_sampling = true;
        settings.adaptive_min_spp = 8;
        settings.adaptive_threshold = 0.01;
        settings.aperture = 0.0;

        s.camera.origin = {0.0, 1.05, 5.6};
        s.camera.forward = {0.0, -0.08, -1.0};
        s.camera.half_fov_x = kPi / 4.8;

        auto* ground = add_material<Lambertian>(s, Vec3{0.60, 0.60, 0.62});
        auto* back = add_material<Lambertian>(s, Vec3{0.24, 0.24, 0.26});
        auto* area_emissive = add_material<Emissive>(s, Vec3{11.0, 11.0, 10.5});
        auto* rim_emissive = add_material<Emissive>(s, Vec3{7.5, 8.0, 9.5});

        auto* ggx_low = add_material<GGXMetal>(s, Vec3{0.92, 0.91, 0.90}, 0.05);
        auto* ggx_mid = add_material<GGXMetal>(s, Vec3{0.92, 0.91, 0.90}, 0.15);
        auto* ggx_high = add_material<GGXMetal>(s, Vec3{0.92, 0.91, 0.90}, 0.32);
        auto* ggx_warm = add_material<GGXMetal>(s, Vec3{0.95, 0.80, 0.55}, 0.18);
        auto* ggx_cool = add_material<GGXMetal>(s, Vec3{0.78, 0.86, 0.95}, 0.18);

        s.world.add(std::make_shared<Plane>(Vec3{0, -0.5, 0}, Vec3{0, 1, 0}, ground));
        s.world.add(std::make_shared<Rectangle>(
            Vec3{0.0, 1.3, -6.9},
            Vec3{7.5, 0.0, 0.0},
            Vec3{0.0, 3.0, 0.0},
            back));

        const double r_main = 0.90;
        s.world.add(std::make_shared<Sphere>(Vec3{-2.2, 0.40, -2.7}, r_main, ggx_low));
        s.world.add(std::make_shared<Sphere>(Vec3{0.0, 0.40, -2.7}, r_main, ggx_mid));
        s.world.add(std::make_shared<Sphere>(Vec3{2.2, 0.40, -2.7}, r_main, ggx_high));

        const double r_small = 0.62;
        s.world.add(std::make_shared<Sphere>(Vec3{-1.2, 0.12, -4.5}, r_small, ggx_warm));
        s.world.add(std::make_shared<Sphere>(Vec3{1.2, 0.12, -4.5}, r_small, ggx_cool));

        s.world.add(std::make_shared<Rectangle>(
            Vec3{0.0, 3.2, -3.5},
            Vec3{2.8, 0.0, 0.0},
            Vec3{0.0, 0.0, 2.3},
            area_emissive));
        s.world.add(std::make_shared<Rectangle>(
            Vec3{-3.8, 1.7, -3.8},
            Vec3{0.0, 1.8, 0.0},
            Vec3{0.0, 0.0, 1.7},
            rim_emissive));

        s.point_lights.push_back({Vec3{3.5, 2.1, -2.1}, Vec3{24.0, 23.0, 22.0}});
        return true;
    }

    if (scene_id == StandardSceneId::GgxRoughnessRamp) {
        settings.image_width = 1920;
        settings.image_height = 1080;
        settings.samples_per_pixel = 128;
        settings.max_depth = 5;
        settings.focus_distance = 4.0;
        settings.exposure = 1.0;
        settings.adaptive_sampling = true;
        settings.adaptive_min_spp = 8;
        settings.adaptive_threshold = 0.01;
        settings.aperture = 0.0;

        s.camera.origin = {0.0, 0.95, 7.8};
        s.camera.forward = {0.0, -0.07, -1.0};
        s.camera.half_fov_x = kPi / 5.0;

        auto* ground = add_material<CheckerLambertian>(
            s,
            Vec3{0.86, 0.86, 0.86},
            Vec3{0.30, 0.30, 0.30},
            1.8, 1.8);
        auto* back = add_material<Lambertian>(s, Vec3{0.22, 0.22, 0.25});
        auto* area_emissive = add_material<Emissive>(s, Vec3{13.0, 12.5, 12.0});
        auto* rim_emissive = add_material<Emissive>(s, Vec3{9.0, 9.0, 10.5});

        s.world.add(std::make_shared<Plane>(Vec3{0, -0.5, 0}, Vec3{0, 1, 0}, ground));
        s.world.add(std::make_shared<Rectangle>(
            Vec3{0.0, 1.2, -8.0},
            Vec3{12.0, 0.0, 0.0},
            Vec3{0.0, 3.0, 0.0},
            back));

        const std::vector<double> roughness = {
            0.02, 0.08, 0.14, 0.20, 0.26, 0.32, 0.40, 0.50
        };
        const double start_x = -5.6;
        const double spacing = 1.6;
        const double radius = 0.55;
        const Vec3 f0 = {0.92, 0.91, 0.90};

        for (int i = 0; i < static_cast<int>(roughness.size()); ++i) {
            auto* mat = add_material<GGXMetal>(s, f0, roughness[static_cast<std::size_t>(i)]);
            const double x = start_x + spacing * i;
            s.world.add(std::make_shared<Sphere>(Vec3{x, 0.05, -3.6}, radius, mat));
        }

        s.world.add(std::make_shared<Rectangle>(
            Vec3{0.0, 3.3, -4.2},
            Vec3{4.0, 0.0, 0.0},
            Vec3{0.0, 0.0, 2.8},
            area_emissive));
        s.world.add(std::make_shared<Rectangle>(
            Vec3{-6.8, 1.7, -4.0},
            Vec3{0.0, 1.9, 0.0},
            Vec3{0.0, 0.0, 2.2},
            rim_emissive));

        s.point_lights.push_back({Vec3{6.5, 2.2, -2.4}, Vec3{28.0, 26.0, 24.0}});

        std::cout << "GGX roughness ramp legend (left -> right):\n";
        for (int i = 0; i < static_cast<int>(roughness.size()); ++i) {
            const double x = start_x + spacing * i;
            std::cout << "  index " << i
                      << " | x=" << x
                      << " | roughness=" << roughness[static_cast<std::size_t>(i)]
                      << "\n";
        }
        return true;
    }

    if (scene_id == StandardSceneId::PolyhedraLab) {
        settings.image_width = 1920;
        settings.image_height = 1080;
        settings.samples_per_pixel = 128;
        settings.max_depth = 5;
        settings.focus_distance = 4.0;
        settings.exposure = 1.0;
        settings.adaptive_sampling = true;
        settings.adaptive_min_spp = 8;
        settings.adaptive_threshold = 0.01;
        settings.aperture = 0.0;

        s.camera.origin = {0.0, 1.0, 6.5};
        s.camera.forward = {0.0, -0.08, -1.0};
        s.camera.half_fov_x = kPi / 5.0;

        auto* ground = add_material<Lambertian>(s, Vec3{0.60, 0.60, 0.62});
        auto* matte = add_material<Lambertian>(s, Vec3{0.84, 0.30, 0.24});
        auto* metal = add_material<GGXMetal>(s, Vec3{0.92, 0.90, 0.86}, 0.16);
        auto* glass = add_material<Dielectric>(s, 1.5);
        auto* aniso = add_material<AnisotropicGGXMetal>(s, Vec3{0.86, 0.88, 0.92}, 0.08, 0.34, Vec3{1, 0, 0});
        auto* area_emissive = add_material<Emissive>(s, Vec3{11.0, 10.5, 10.0});

        s.world.add(std::make_shared<Plane>(Vec3{0, -0.5, 0}, Vec3{0, 1, 0}, ground));

        // Reference spheres.
        s.world.add(std::make_shared<Sphere>(Vec3{-3.0, 0.0, -4.8}, 0.5, matte));
        s.world.add(std::make_shared<Sphere>(Vec3{3.0, 0.0, -4.8}, 0.5, metal));

        // Polyhedra built from triangles.
        add_cube(s, Vec3{-1.5, 0.2, -3.1}, 1.2, metal);
        add_tetrahedron(s, Vec3{0.1, 0.25, -3.6}, 1.5, matte);
        add_cube(s, Vec3{1.8, 0.15, -3.0}, 1.1, aniso);
        add_tetrahedron(s, Vec3{0.0, 0.9, -4.9}, 1.7, glass);

        s.world.add(std::make_shared<Rectangle>(
            Vec3{0.0, 3.2, -4.4},
            Vec3{2.8, 0.0, 0.0},
            Vec3{0.0, 0.0, 2.1},
            area_emissive));
        s.point_lights.push_back({Vec3{-3.2, 2.0, -2.7}, Vec3{26.0, 24.0, 22.0}});
        s.point_lights.push_back({Vec3{3.2, 2.0, -5.6}, Vec3{16.0, 17.0, 20.0}});
        return true;
    }

    ScatterMaterialMode scatter_mode = ScatterMaterialMode::Mixed;
    if (scatter_mode_for_scene(scene_id, scatter_mode)) {
        ScatterParams params = make_scatter_params();
        apply_scene_param_overrides(params, scene_param_overrides);
        build_scatter_scene(s, settings, scatter_mode, params);
        return true;
    }

    scatter_mode = ScatterMaterialMode::Mixed;
    if (big_scatter_mode_for_scene(scene_id, scatter_mode)) {
        BigScatterParams params = make_big_scatter_params();
        apply_scene_param_overrides(params, scene_param_overrides);
        build_big_scatter_scene(s, settings, scatter_mode, params);
        return true;
    }

    if (scene_id == StandardSceneId::RowScatter) {
        RowScatterParams params = make_row_scatter_params();
        apply_scene_param_overrides(params, scene_param_overrides);
        build_row_scatter_scene(s, settings, params);
        return true;
    }

    if (scene_id == StandardSceneId::Cornell) {
        apply_preset_config(make_cornell_config(), s, settings);

        auto* white = add_material<Lambertian>(s, Vec3{0.75, 0.75, 0.75});
        auto* red = add_material<Lambertian>(s, Vec3{0.75, 0.15, 0.15});
        auto* green = add_material<Lambertian>(s, Vec3{0.15, 0.75, 0.15});
        auto* glass = add_material<Dielectric>(s, 1.5);
        auto* metal = add_material<GGXMetal>(s, Vec3{0.9, 0.9, 0.9}, 0.12);
        auto* area_emissive = add_material<Emissive>(s, Vec3{13.0, 13.0, 13.0});

        s.world.add(std::make_shared<Rectangle>(Vec3{0.0, 0.0, 0.0}, Vec3{2.0,0.0,0.0}, Vec3{0.0,0.0,2.0}, white));
        s.world.add(std::make_shared<Rectangle>(Vec3{0.0, 2.0, 0.0}, Vec3{2.0,0.0,0.0}, Vec3{0.0,0.0,2.0}, white));
        s.world.add(std::make_shared<Rectangle>(Vec3{0.0, 1.0, -1.0}, Vec3{2.0,0.0,0.0}, Vec3{0.0,2.0,0.0}, white));
        s.world.add(std::make_shared<Rectangle>(Vec3{-1.0, 1.0, 0.0}, Vec3{0.0,0.0,2.0}, Vec3{0.0,2.0,0.0}, red));
        s.world.add(std::make_shared<Rectangle>(Vec3{1.0, 1.0, 0.0}, Vec3{0.0,0.0,2.0}, Vec3{0.0,2.0,0.0}, green));
        s.world.add(std::make_shared<Rectangle>(Vec3{0.0, 1.99, -0.2}, Vec3{0.6,0.0,0.0}, Vec3{0.0,0.0,0.6}, area_emissive));
        s.world.add(std::make_shared<Sphere>(Vec3{-0.45,0.35,-0.35}, 0.35, glass));
        s.world.add(std::make_shared<Sphere>(Vec3{0.45,0.35,0.25}, 0.35, metal));

        return true;
    }

    if (scene_id == StandardSceneId::Benchmark) {
        apply_preset_config(make_benchmark_config(), s, settings);

        auto* ground = add_material<Lambertian>(s, Vec3{0.5, 0.5, 0.5});
        auto* glass = add_material<Dielectric>(s, 1.5);
        auto* metal = add_material<GGXMetal>(s, Vec3{0.9, 0.9, 0.9}, 0.25);
        auto* diffuse = add_material<Lambertian>(s, Vec3{0.75, 0.4, 0.3});
        auto* area_emissive = add_material<Emissive>(s, Vec3{12.0, 12.0, 12.0});

        s.world.add(std::make_shared<Plane>(Vec3{0,-0.5,0}, Vec3{0,1,0}, ground));
        for (int z = 0; z < 10; ++z) {
            for (int x = -5; x <= 5; ++x) {
                Vec3 c = {0.5 * x, -0.15, -2.0 - 0.75 * z};
                const Material* m = ((x + z) % 3 == 0) ? static_cast<const Material*>(glass)
                                  : ((x + z) % 3 == 1) ? static_cast<const Material*>(metal)
                                                       : static_cast<const Material*>(diffuse);
                s.world.add(std::make_shared<Sphere>(c, 0.35, m));
            }
        }
        s.world.add(std::make_shared<Rectangle>(
            Vec3{0.0, 3.0, -5.0},
            Vec3{3.0, 0.0, 0.0},
            Vec3{0.0, 0.0, 3.0},
            area_emissive));
        return true;
    }

    if (scene_id == StandardSceneId::Volumes) {
        apply_preset_config(make_volumes_config(), s, settings);

        auto* ground = add_material<Lambertian>(s, Vec3{0.50, 0.50, 0.52});
        auto* matte_white = add_material<Lambertian>(s, Vec3{0.75, 0.75, 0.76});
        auto* glass = add_material<Dielectric>(s, 1.5);
        auto* metal = add_material<GGXMetal>(s, Vec3{0.90, 0.88, 0.82}, 0.16);
        auto* area_emissive = add_material<Emissive>(s, Vec3{16.0, 15.0, 14.0});

        auto* blue_fog = add_material<Isotropic>(s, Vec3{0.45, 0.62, 0.95});
        auto* warm_smoke = add_material<Isotropic>(s, Vec3{0.95, 0.76, 0.50});
        auto* haze = add_material<Isotropic>(s, Vec3{0.90, 0.94, 1.00});

        s.world.add(std::make_shared<Plane>(Vec3{0, -0.5, 0}, Vec3{0, 1, 0}, ground));
        s.world.add(std::make_shared<Sphere>(Vec3{-2.7, 0.35, -3.2}, 0.35, matte_white));
        s.world.add(std::make_shared<Sphere>(Vec3{2.7, 0.35, -3.2}, 0.35, matte_white));
        s.world.add(std::make_shared<Sphere>(Vec3{0.0, 0.55, -6.1}, 0.55, metal));

        s.world.add(std::make_shared<Rectangle>(
            Vec3{0.0, 3.0, -4.2},
            Vec3{1.8, 0.0, 0.0},
            Vec3{0.0, 0.0, 1.8},
            area_emissive));

        auto fog_boundary = std::make_shared<Sphere>(Vec3{-1.2, 0.7, -4.3}, 1.35, glass);
        auto smoke_boundary = std::make_shared<Sphere>(Vec3{1.3, 0.65, -4.7}, 1.15, glass);
        auto haze_boundary = std::make_shared<Sphere>(Vec3{0.0, 0.8, -4.5}, 8.5, glass);

        s.world.add(std::make_shared<Sphere>(Vec3{-1.2, 0.7, -4.3}, 1.35, glass));
        s.world.add(std::make_shared<Sphere>(Vec3{1.3, 0.65, -4.7}, 1.15, glass));
        s.world.add(std::make_shared<ConstantMedium>(fog_boundary, 0.75, blue_fog));
        s.world.add(std::make_shared<ConstantMedium>(smoke_boundary, 0.95, warm_smoke));
        s.world.add(std::make_shared<ConstantMedium>(haze_boundary, 0.018, haze));
        return true;
    }

    return false;
}

bool get_model_scene_preset_by_name(const std::string& name,
                                    ModelScenePreset& preset)
{
    const auto it = model_scene_registry().find(name);
    if (it == model_scene_registry().end())
        return false;

    switch (it->second) {
        case ModelSceneId::Sponza:
            preset = make_sponza_model_preset();
            return true;
    }

    return false;
}

std::vector<std::string> list_primary_scene_names()
{
    std::vector<std::string> names;
    names.reserve(standard_scene_entries().size() + model_scene_entries().size());
    for (const auto& entry : standard_scene_entries()) {
        if (!entry.alias)
            names.emplace_back(entry.name);
    }
    for (const auto& entry : model_scene_entries())
        names.emplace_back(entry.name);
    return names;
}

std::vector<std::string> list_scene_alias_names()
{
    std::vector<std::string> names;
    for (const auto& entry : standard_scene_entries()) {
        if (entry.alias)
            names.emplace_back(entry.name);
    }
    return names;
}
