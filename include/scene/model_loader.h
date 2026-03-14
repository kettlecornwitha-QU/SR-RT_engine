#pragma once

#include <string>
#include <vector>

#include "core/material.h"
#include "scene/scene_builder.h"

bool scene_file_exists(const std::string& path);
std::string find_existing_scene_path(const std::vector<std::string>& candidates);

bool load_model_scene(const std::string& path,
                      SceneBuild& s,
                      const Material* fallback,
                      int gltf_triangle_step);

