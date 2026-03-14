#pragma once

#include <memory>

#include "materials/lambertian.h"
#include "render/texture.h"

class TexturedLambertian : public Lambertian {
public:
    std::shared_ptr<ImageTexture> texture;

    TexturedLambertian(std::shared_ptr<ImageTexture> tex,
                       const Vec3& tint = {1, 1, 1});

    Vec3 aov_albedo(const HitRecord& rec) const override;

    bool scatter(const Ray& ray_in,
                 const HitRecord& rec,
                 Vec3& attenuation,
                 Ray& scattered) const override;
};
