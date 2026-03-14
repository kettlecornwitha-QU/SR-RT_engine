#pragma once

#include "math/vec3.h"

class Material;
struct SceneBuild;

void add_cube(SceneBuild& s,
              const Vec3& center,
              double size,
              const Material* material);

void add_tetrahedron(SceneBuild& s,
                     const Vec3& center,
                     double size,
                     const Material* material);

void add_tetrahedron_flat_base(SceneBuild& s,
                               const Vec3& base_center,
                               double size,
                               const Material* material);
