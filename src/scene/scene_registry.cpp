#include "scene/scene_registry.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>
#include <unordered_set>

#include "scene/scene_presets.h"

namespace {

std::string humanize_name(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    bool new_word = true;
    for (char c : s) {
        if (c == '_' || c == '-') {
            out.push_back(' ');
            new_word = true;
            continue;
        }
        if (new_word && std::isalpha(static_cast<unsigned char>(c))) {
            out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        } else {
            out.push_back(c);
        }
        new_word = false;
    }
    return out;
}

std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

SceneGroupMeta make_family_group(const std::string& key,
                                 const std::string& label,
                                 const std::string& default_variant,
                                 const std::vector<SceneVariantMeta>& candidate_variants,
                                 const std::unordered_set<std::string>& primary_set,
                                 std::unordered_set<std::string>& claimed_primary)
{
    SceneGroupMeta group;
    group.key = key;
    group.label = label;
    group.default_variant = default_variant;
    group.has_variants = true;

    for (const auto& v : candidate_variants) {
        if (primary_set.find(v.scene_name) == primary_set.end())
            continue;
        group.variants.push_back(v);
        claimed_primary.insert(v.scene_name);
    }

    if (group.variants.empty()) {
        group.variants.push_back({ "default", default_variant });
    }
    return group;
}

} // namespace

const SceneRegistryMeta& get_scene_registry_meta() {
    static const SceneRegistryMeta kRegistry = []() {
        SceneRegistryMeta registry;
        registry.schema_version = 1;
        registry.aliases = list_scene_alias_names();

        const std::vector<std::string> primary = list_primary_scene_names();
        const std::unordered_set<std::string> primary_set(primary.begin(), primary.end());
        std::unordered_set<std::string> claimed;

        // Explicit umbrella families with stable labels and defaults.
        registry.groups.push_back(make_family_group(
            "scatter",
            "Scatter",
            "scatter",
            {
                {"mixed", "scatter"},
                {"lambertian", "scatter_lambertian"},
                {"ggx_metal", "scatter_ggx_metal"},
                {"anisotropic_ggx_metal", "scatter_anisotropic_ggx_metal"},
                {"thin_transmission", "scatter_thin_transmission"},
                {"sheen", "scatter_sheen"},
                {"clearcoat", "scatter_clearcoat"},
                {"dielectric", "scatter_dielectric"},
                {"coated_plastic", "scatter_coated_plastic"},
                {"subsurface", "scatter_subsurface"},
                {"conductor_copper", "scatter_conductor_copper"},
                {"conductor_aluminum", "scatter_conductor_aluminum"},
                {"isotropic_volume", "scatter_isotropic_volume"},
            },
            primary_set,
            claimed));

        registry.groups.push_back(make_family_group(
            "big_scatter",
            "Big Scatter",
            "big_scatter",
            {
                {"mixed", "big_scatter"},
                {"lambertian", "big_scatter_lambertian"},
                {"ggx_metal", "big_scatter_ggx_metal"},
                {"anisotropic_ggx_metal", "big_scatter_anisotropic_ggx_metal"},
                {"thin_transmission", "big_scatter_thin_transmission"},
                {"sheen", "big_scatter_sheen"},
                {"clearcoat", "big_scatter_clearcoat"},
                {"dielectric", "big_scatter_dielectric"},
                {"coated_plastic", "big_scatter_coated_plastic"},
                {"subsurface", "big_scatter_subsurface"},
                {"conductor_copper", "big_scatter_conductor_copper"},
                {"conductor_aluminum", "big_scatter_conductor_aluminum"},
                {"isotropic_volume", "big_scatter_isotropic_volume"},
            },
            primary_set,
            claimed));

        // Remaining primary scenes are standalone groups.
        std::vector<std::string> remaining;
        remaining.reserve(primary.size());
        for (const auto& s : primary) {
            if (claimed.find(s) == claimed.end())
                remaining.push_back(s);
        }

        const std::vector<std::string> preferred = {
            "row_scatter",
            "materials",
            "spheres",
            "metalglass",
            "cornell",
            "benchmark",
            "volumes",
            "sponza"
        };

        std::vector<std::string> ordered;
        ordered.reserve(remaining.size());
        for (const auto& p : preferred) {
            if (std::find(remaining.begin(), remaining.end(), p) != remaining.end())
                ordered.push_back(p);
        }
        for (const auto& r : remaining) {
            if (std::find(ordered.begin(), ordered.end(), r) == ordered.end())
                ordered.push_back(r);
        }

        for (const auto& s : ordered) {
            SceneGroupMeta g;
            g.key = s;
            g.label = humanize_name(s);
            g.default_variant = s;
            g.has_variants = false;
            g.variants.push_back({"default", s});
            registry.groups.push_back(std::move(g));
        }

        return registry;
    }();

    return kRegistry;
}

std::string make_scene_registry_json() {
    const auto& reg = get_scene_registry_meta();
    std::ostringstream os;
    os << "{";
    os << "\"schema_version\":" << reg.schema_version << ",";

    os << "\"groups\":[";
    for (std::size_t i = 0; i < reg.groups.size(); ++i) {
        const auto& g = reg.groups[i];
        if (i > 0)
            os << ",";
        os << "{";
        os << "\"key\":\"" << json_escape(g.key) << "\",";
        os << "\"label\":\"" << json_escape(g.label) << "\",";
        os << "\"default_variant\":\"" << json_escape(g.default_variant) << "\",";
        os << "\"has_variants\":" << (g.has_variants ? "true" : "false") << ",";
        os << "\"variants\":[";
        for (std::size_t j = 0; j < g.variants.size(); ++j) {
            const auto& v = g.variants[j];
            if (j > 0)
                os << ",";
            os << "{";
            os << "\"label\":\"" << json_escape(v.label) << "\",";
            os << "\"scene_name\":\"" << json_escape(v.scene_name) << "\"";
            os << "}";
        }
        os << "]";
        os << "}";
    }
    os << "],";

    os << "\"aliases\":[";
    for (std::size_t i = 0; i < reg.aliases.size(); ++i) {
        if (i > 0)
            os << ",";
        os << "\"" << json_escape(reg.aliases[i]) << "\"";
    }
    os << "]";
    os << "}";
    return os.str();
}
