#include "scene/polyhedra_builder.h"

#include <cmath>
#include <memory>

#include "geometry/triangle.h"
#include "scene/scene_builder.h"

namespace {

void add_tri(SceneBuild& s,
             const Vec3& a,
             const Vec3& b,
             const Vec3& c,
             const Material* material)
{
    s.world.add(std::make_shared<Triangle>(a, b, c, material));
}

} // namespace

void add_cube(SceneBuild& s,
              const Vec3& center,
              double size,
              const Material* material)
{
    const double h = 0.5 * size;
    const Vec3 p000 = center + Vec3{-h, -h, -h};
    const Vec3 p001 = center + Vec3{-h, -h, +h};
    const Vec3 p010 = center + Vec3{-h, +h, -h};
    const Vec3 p011 = center + Vec3{-h, +h, +h};
    const Vec3 p100 = center + Vec3{+h, -h, -h};
    const Vec3 p101 = center + Vec3{+h, -h, +h};
    const Vec3 p110 = center + Vec3{+h, +h, -h};
    const Vec3 p111 = center + Vec3{+h, +h, +h};

    // -X face
    add_tri(s, p000, p010, p011, material);
    add_tri(s, p000, p011, p001, material);
    // +X face
    add_tri(s, p100, p101, p111, material);
    add_tri(s, p100, p111, p110, material);
    // -Y face
    add_tri(s, p000, p001, p101, material);
    add_tri(s, p000, p101, p100, material);
    // +Y face
    add_tri(s, p010, p110, p111, material);
    add_tri(s, p010, p111, p011, material);
    // -Z face
    add_tri(s, p000, p100, p110, material);
    add_tri(s, p000, p110, p010, material);
    // +Z face
    add_tri(s, p001, p011, p111, material);
    add_tri(s, p001, p111, p101, material);
}

void add_tetrahedron(SceneBuild& s,
                     const Vec3& center,
                     double size,
                     const Material* material)
{
    const double h = 0.5 * size;
    const Vec3 a = center + Vec3{+h, +h, +h};
    const Vec3 b = center + Vec3{-h, -h, +h};
    const Vec3 c = center + Vec3{-h, +h, -h};
    const Vec3 d = center + Vec3{+h, -h, -h};

    add_tri(s, a, c, b, material);
    add_tri(s, a, b, d, material);
    add_tri(s, a, d, c, material);
    add_tri(s, b, c, d, material);
}

void add_tetrahedron_flat_base(SceneBuild& s,
                               const Vec3& base_center,
                               double size,
                               const Material* material)
{
    const double inv_sqrt3 = 0.5773502691896257; // 1 / sqrt(3)
    const double half = 0.5 * size;
    const double base_z = size * inv_sqrt3;
    const double base_back_z = -0.5 * size * inv_sqrt3;
    const double apex_h = std::sqrt(2.0 / 3.0) * size;

    const Vec3 a = base_center + Vec3{0.0, 0.0, base_z};
    const Vec3 b = base_center + Vec3{-half, 0.0, base_back_z};
    const Vec3 c = base_center + Vec3{half, 0.0, base_back_z};
    const Vec3 d = base_center + Vec3{0.0, apex_h, 0.0};

    // Base face (a,b,c) lies flat on the ground plane.
    add_tri(s, a, b, c, material);
    add_tri(s, a, d, b, material);
    add_tri(s, b, d, c, material);
    add_tri(s, c, d, a, material);
}
