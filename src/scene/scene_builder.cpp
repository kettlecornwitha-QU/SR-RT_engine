#include "scene/scene_builder.h"

#include <iostream>
#include <memory>
#include <string>
#include "materials/lambertian.h"
#include "scene/model_loader.h"
#include "scene/scene_presets.h"

bool build_scene_by_name(const std::string& name,
                         SceneBuild& s,
                         RenderSettings& settings,
                         const SceneParamOverrides& scene_param_overrides)
{
    if (build_standard_scene_by_name(name, s, settings, scene_param_overrides))
        return true;

    ModelScenePreset model_preset{};
    if (get_model_scene_preset_by_name(name, model_preset)) {
        settings.image_width = model_preset.render_settings.image_width;
        settings.image_height = model_preset.render_settings.image_height;
        settings.samples_per_pixel = model_preset.render_settings.samples_per_pixel;
        settings.max_depth = model_preset.render_settings.max_depth;
        settings.aperture = model_preset.render_settings.aperture;
        settings.focus_distance = model_preset.render_settings.focus_distance;
        settings.exposure = model_preset.render_settings.exposure;
        settings.adaptive_sampling = model_preset.render_settings.adaptive_sampling;
        settings.adaptive_min_spp = model_preset.render_settings.adaptive_min_spp;
        settings.adaptive_threshold = model_preset.render_settings.adaptive_threshold;
        s.camera = model_preset.camera;
        auto fallback = std::make_shared<Lambertian>(Vec3{0.7, 0.7, 0.7});
        const Material* fallback_raw = fallback.get();
        s.materials.push_back(std::move(fallback));
        std::string obj_path = find_existing_scene_path(model_preset.model_candidates);

        if (obj_path.empty()) {
            std::cerr << name << " scene requested, but model not found. Looked for:\n";
            for (const auto& candidate : model_preset.model_candidates)
                std::cerr << "  " << candidate << "\n";
            return false;
        }

        if (!load_model_scene(obj_path, s, fallback_raw, settings.gltf_triangle_step)) {
            std::cerr << "Failed to load " << name << " model: " << obj_path << "\n";
            return false;
        }
        s.point_lights.insert(s.point_lights.end(),
                              model_preset.point_lights.begin(),
                              model_preset.point_lights.end());
        s.area_lights.insert(s.area_lights.end(),
                             model_preset.area_lights.begin(),
                             model_preset.area_lights.end());
        return true;
    }

    return false;
}
