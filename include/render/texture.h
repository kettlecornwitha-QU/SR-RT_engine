#pragma once

#include <memory>
#include <string>
#include <vector>

#include "math/vec3.h"

class ImageTexture {
public:
    int width = 0;
    int height = 0;
    int channels = 0;
    std::vector<unsigned char> pixels;

    bool valid() const;
    Vec3 sample(double u, double v) const;

    static std::shared_ptr<ImageTexture> load(const std::string& path,
                                              std::string& message);
};
