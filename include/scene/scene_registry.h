#pragma once

#include <string>
#include <vector>

struct SceneVariantMeta {
    std::string label;
    std::string scene_name;
};

struct SceneGroupMeta {
    std::string key;
    std::string label;
    std::string default_variant;
    bool has_variants = false;
    std::vector<SceneVariantMeta> variants;
};

struct SceneRegistryMeta {
    int schema_version = 1;
    std::vector<SceneGroupMeta> groups;
    std::vector<std::string> aliases;
};

const SceneRegistryMeta& get_scene_registry_meta();
std::string make_scene_registry_json();
