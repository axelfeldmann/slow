#pragma once

#include <cstdint>
#include <vector>

uint32_t cycle_length(const std::vector<uint32_t>& perm);
std::vector<uint32_t> generate_cyclic_permutation(uint32_t approx_P, uint32_t approx_shift);