#include "primes.h"
#include <cstdio>
#include <cmath>
#include <cassert>

bool is_prime(uint32_t n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;

    for (uint32_t i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
    }
    return true;
}

uint32_t get_largest_prime_less_than(uint32_t n) {
    for (uint32_t i = n - 1; i > 1; --i) {
        if (is_prime(i)) return i;
    }
    return 2;
}

uint32_t cycle_length(const std::vector<uint32_t>& perm) {
    uint32_t cur = 0;
    uint32_t length = 0;
    do {
        cur = perm[cur];
        length++;
    } while (cur != 0);
    return length;
}

std::vector<uint32_t> generate_cyclic_permutation(uint32_t approx_P, uint32_t shift) {
    uint32_t P = get_largest_prime_less_than(approx_P);
    printf("\tP = %u\n", P);
    printf("\tshift = %u\n", shift);

    std::vector<uint32_t> perm;
    uint32_t cur = shift;
    do {
        perm.push_back(cur);
        cur = (cur + shift) % P;
    } while (cur != shift);
    
    assert(perm.size() == P);
    assert(cycle_length(perm) == P - 1);

    printf("\tcycle length = %u\n", cycle_length(perm));

    return perm;
}