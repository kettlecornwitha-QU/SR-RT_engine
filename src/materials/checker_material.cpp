#include "materials/checker_material.h"

#include <algorithm>
#include <cmath>

#include "core/hit.h"

CheckerMaterial::CheckerMaterial(const Material* a,
                                 const Material* b,
                                 double su,
                                 double sv)
    : material_a(a),
      material_b(b),
      scale_u(std::max(1e-6, su)),
      scale_v(std::max(1e-6, sv)) {}

const Material* CheckerMaterial::pick(const HitRecord& rec) const {
    const double u = std::floor(rec.u * scale_u);
    const double v = std::floor(rec.v * scale_v);
    const int parity = static_cast<int>(u + v);
    return (parity & 1) ? material_b : material_a;
}

Vec3 CheckerMaterial::emitted(const HitRecord& rec) const {
    const Material* mat = pick(rec);
    if (!mat)
        return {0, 0, 0};
    return mat->emitted(rec);
}

Vec3 CheckerMaterial::aov_albedo(const HitRecord& rec) const {
    const Material* mat = pick(rec);
    if (!mat)
        return {1, 1, 1};
    return mat->aov_albedo(rec);
}

bool CheckerMaterial::scatter(const Ray& ray_in,
                              const HitRecord& rec,
                              Vec3& attenuation,
                              Ray& scattered) const
{
    const Material* mat = pick(rec);
    if (!mat)
        return false;
    return mat->scatter(ray_in, rec, attenuation, scattered);
}
