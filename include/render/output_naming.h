#pragma once

#include <string>

struct OutputNameTags {
    double aberration_speed = 0.0;
    double aberration_pitch = 0.0;
    double aberration_yaw = 0.0;
    double camera_pitch = 0.0;
    double camera_yaw = 0.0;
};

std::string format_output_tag_number(double v);

std::string make_default_output_base(const std::string& scene_name,
                                     const OutputNameTags& tags);

