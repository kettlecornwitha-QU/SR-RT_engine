#include "render/rng.h"

namespace {

uint64_t splitmix64(uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

uint64_t xorshift64star(uint64_t& state) {
    // xorshift64*: fast, deterministic, and good enough for path sampling.
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    return state * 2685821657736338717ULL;
}

thread_local uint64_t g_rng_state = 0x9e3779b97f4a7c15ULL;

} // namespace

uint64_t sample_seed(unsigned int base_seed,
                     int pixel_x,
                     int pixel_y,
                     int sample_idx)
{
    uint64_t x = static_cast<uint64_t>(base_seed);
    x ^= splitmix64(static_cast<uint64_t>(pixel_x) + 0x100000001b3ULL);
    x ^= splitmix64(static_cast<uint64_t>(pixel_y) + 0x9e3779b97f4a7c15ULL);
    x ^= splitmix64(static_cast<uint64_t>(sample_idx) + 0xbf58476d1ce4e5b9ULL);
    return splitmix64(x);
}

void seed_rng(uint64_t seed) {
    g_rng_state = splitmix64(seed);
    if (g_rng_state == 0)
        g_rng_state = 0x9e3779b97f4a7c15ULL;
}

double random_double() {
    // Use top 53 bits for uniform doubles in [0, 1).
    uint64_t x = xorshift64star(g_rng_state);
    return static_cast<double>(x >> 11) * (1.0 / 9007199254740992.0); // 2^-53
}
