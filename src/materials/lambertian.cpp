#include "materials/lambertian.h"
#include "core/hit.h"

#include "render/constants.h"
#include "render/sampling.h"

Lambertian::Lambertian(const Vec3& a) : albedo(a) {}

Vec3 Lambertian::aov_albedo(const HitRecord& rec) const {
    (void)rec;
    return albedo;
}

bool Lambertian::scatter(const Ray& ray_in,
                         const HitRecord& rec,
                         Vec3& attenuation,
                         Ray& scattered) const
{
    (void)ray_in;
    Vec3 local_dir = random_cosine_direction();
    Vec3 scatter_direction = local_to_world(local_dir, rec.normal).normalize();

    scattered.origin = rec.point + rec.normal * kRayEpsilon;
    scattered.direction = scatter_direction;
    attenuation = albedo;

    return true;
}
