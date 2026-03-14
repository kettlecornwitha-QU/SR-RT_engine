#include "render/output_naming.h"

#include <cmath>
#include <iomanip>
#include <sstream>

std::string format_output_tag_number(double v) {
    const double rounded_tenth = std::round(v * 10.0) / 10.0;
    const double nearest_int = std::round(rounded_tenth);
    if (std::fabs(rounded_tenth - nearest_int) < 1e-9) {
        std::ostringstream ioss;
        ioss << static_cast<long long>(nearest_int);
        return ioss.str();
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << rounded_tenth;
    return oss.str();
}

std::string make_default_output_base(const std::string& scene_name,
                                     const OutputNameTags& tags)
{
    return "outputs/" + scene_name +
           "_Ab_" + format_output_tag_number(tags.aberration_speed) +
           "_" + format_output_tag_number(tags.aberration_pitch) +
           "_" + format_output_tag_number(tags.aberration_yaw) +
           "_C_" + format_output_tag_number(tags.camera_pitch) +
           "_" + format_output_tag_number(tags.camera_yaw);
}

