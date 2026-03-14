#pragma once

#include <string>

#include "app/cli.h"
#include "render/settings.h"
#include "scene/scene_builder.h"

bool apply_run_configuration(const ParsedArgs& args,
                             RenderSettings& settings,
                             SceneBuild& scene,
                             std::string& output_base,
                             std::string& error_message);
