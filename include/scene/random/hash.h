#pragma once

#include <cstdint>

namespace scene::random {

enum class SeedChannel : int {
    ScatterJitterX = 1,
    ScatterJitterZ = 2,
    ScatterColorValue = 3,
    ScatterColorHueA = 4,
    ScatterColorHueB = 5,
    ScatterColorHueC = 6,
    ScatterRadiusJitter = 7,

    MaterialPick = 8,
    MaterialGGXRoughness = 9,
    MaterialAnisoBaseRoughness = 10,
    MaterialAnisoSign = 11,
    MaterialAnisoAmount = 12,
    MaterialAnisoTangentAngle = 13,
    MaterialSheenBlend = 14,
    MaterialClearcoatIOR = 15,
    MaterialClearcoatRoughness = 16,
    MaterialMixedVolumeGate = 18,
    MaterialVolumeDensity = 19,
    MaterialCoatedPlasticIOR = 20,
    MaterialCoatedPlasticRoughness = 21,
    MaterialSubsurfaceMeanFreePath = 22,
    MaterialSubsurfaceAnisotropy = 23,
    MaterialSubsurfaceForwardness = 24,

    BigScatterJitterX = 101,
    BigScatterJitterZ = 102,
    BigScatterSizePick = 103,
    BigScatterShapePick = 104,
    BigScatterPaletteIndex = 105,
    BigScatterPaletteBlend = 106,
    BigScatterNeutralPull = 107,
    BigScatterGainJitter = 108,

    RowScatterColorValue = 301,
    RowScatterColorHueA = 302,
    RowScatterColorHueB = 303,
    RowScatterColorHueC = 304,

    RowScatterMaterialBandPick = 310,
    RowScatterRadiusNoise = 311,
    RowScatterJitterX = 312,
    RowScatterJitterZ = 313,
    RowScatterOverrideGate = 314,
    RowScatterOverridePick = 315,
    RowScatterThinIOR = 316,
    RowScatterThinRoughness = 317,
    RowScatterDielectricIOR = 318,
    RowScatterCopperRoughness = 319,
    RowScatterAluminumRoughness = 320,
    RowScatterGGXRoughness = 321,
    RowScatterClearcoatIOR = 322,
    RowScatterClearcoatRoughness = 323,
    RowScatterCoatedPlasticIOR = 324,
    RowScatterCoatedPlasticRoughness = 325,
    RowScatterSubsurfaceMeanFreePath = 326,
    RowScatterSubsurfaceAnisotropy = 327,
    RowScatterSubsurfaceForwardness = 328
};

constexpr int seed_value(SeedChannel seed) {
    return static_cast<int>(seed);
}

inline double hash01(int x, int z, int seed) {
    uint32_t h = static_cast<uint32_t>(x * 73856093 ^ z * 19349663 ^ seed * 83492791);
    h ^= h >> 13;
    h *= 0x85ebca6bU;
    h ^= h >> 16;
    return (h & 0x00ffffffU) / static_cast<double>(0x01000000U);
}

inline double hash01(int x, int z, SeedChannel seed) {
    return hash01(x, z, seed_value(seed));
}

} // namespace scene::random
