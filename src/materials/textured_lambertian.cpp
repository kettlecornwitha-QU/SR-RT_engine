#include "materials/textured_lambertian.h"

#include <utility>

#include "core/hit.h"
#include "render/constants.h"
#include "render/sampling.h"

TexturedLambertian::TexturedLambertian(std::shared_ptr<ImageTexture> tex,
                                       const Vec3& tint)
    : Lambertian(tint), texture(std::move(tex)) {}

Vec3 TexturedLambertian::aov_albedo(const HitRecord& rec) const {
    if (!texture || !texture->valid())
        return albedo;
    Vec3 texel = texture->sample(rec.u, rec.v);
    return {texel.x * albedo.x, texel.y * albedo.y, texel.z * albedo.z};
}

bool TexturedLambertian::scatter(const Ray& ray_in,
                                 const HitRecord& rec,
                                 Vec3& attenuation,
                                 Ray& scattered) const
{
    (void)ray_in;
    Vec3 local_dir = random_cosine_direction();
    Vec3 scatter_direction = local_to_world(local_dir, rec.normal).normalize();

    scattered.origin = rec.point + rec.normal * kRayEpsilon;
    scattered.direction = scatter_direction;
    attenuation = aov_albedo(rec);

    return true;
}
