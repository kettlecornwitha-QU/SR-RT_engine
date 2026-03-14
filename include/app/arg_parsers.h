#pragma once

#include <string>

bool parse_materials_variant(const std::string& value,
                             int& out_materials_variant);

const char* materials_variant_expected_values();

bool parse_big_scatter_palette(const std::string& value,
                               int& out_big_scatter_palette);

const char* big_scatter_palette_expected_values();

bool parse_lighting_preset(const std::string& value,
                           int& out_lighting_preset);

const char* lighting_preset_expected_values();
