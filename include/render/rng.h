#pragma once

#include <cstdint>

uint64_t sample_seed(unsigned int base_seed,
                     int pixel_x,
                     int pixel_y,
                     int sample_idx);

void seed_rng(uint64_t seed);
double random_double();
