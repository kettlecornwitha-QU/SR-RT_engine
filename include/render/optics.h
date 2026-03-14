#pragma once

#include "math/vec3.h"

Vec3 reflect(const Vec3& v, const Vec3& n);
Vec3 refract(const Vec3& uv,
             const Vec3& n,
             double etai_over_etat);

double schlick_reflectance(double cosine, double ref_idx);
Vec3 fresnel_schlick(double cos_theta, const Vec3& f0);
Vec3 fresnel_conductor(double cos_theta, const Vec3& eta, const Vec3& k);
double smith_ggx_g1(double n_dot_x, double roughness);
