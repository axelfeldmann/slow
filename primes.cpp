#include "primes.h"
#include <cstdio>
#include <cmath>
#include <thread>
#include <algorithm>
#include <cassert>

uint64_t power(uint64_t base, uint64_t exp, uint64_t mod) {
    uint64_t result = 1;
    base = base % mod;
    
    while (exp > 0) {
        if (exp % 2 == 1) {
            result = (result * base) % mod;
        }
        exp = exp >> 1;
        base = (base * base) % mod;
    }
    return result;
}

std::vector<uint64_t> prime_factors(uint64_t n) {
    std::vector<uint64_t> factors;
    while (n % 2 == 0) {
        if (factors.empty() || factors.back() != 2) {
            factors.push_back(2);
        }
        n /= 2;
    }
    for (uint64_t i = 3; i <= std::sqrt(n); i += 2) {
        while (n % i == 0) {
            if (factors.empty() || factors.back() != i) {
                factors.push_back(i);
            }
            n /= i;
        }
    }
    if (n > 2) {
        factors.push_back(n);
    }
    return factors;
}

bool is_primitive_root(uint64_t p, uint64_t g) {
    std::vector<uint64_t> factors = prime_factors(p - 1);

    for (auto factor : factors) {
        if (power(g, (p - 1) / factor, p) == 1) {
            return false;
        }
    }
    return true;
}

std::vector<uint64_t> get_primitive_roots(uint64_t p) {
    unsigned int num_threads = std::thread::hardware_concurrency();
    assert(num_threads > 0);

    std::vector<std::vector<uint64_t>> thread_roots(num_threads);

    auto worker = [&thread_roots, p](int thread_id, uint64_t start, uint64_t end) {
        for (uint64_t g = start; g < end; g++) {
            if (is_primitive_root(p, g)) {
                thread_roots[thread_id].push_back(g);
            }
        }
    };

    std::vector<std::thread> threads;
    uint64_t step = (p - 2) / num_threads;
    uint64_t start = 2;

    for (unsigned int i = 0; i < num_threads; i++) {
        uint64_t end = start + step;
        if (i == num_threads - 1) {
            end = p; // Make sure the last thread computes any remaining work.
        }

        threads.push_back(std::thread(worker, i, start, end));
        start = end;
    }

    for (auto &t : threads) {
        t.join();
    }

    std::vector<uint64_t> primitive_roots;
    for (const auto& roots : thread_roots) {
        primitive_roots.insert(primitive_roots.end(), roots.begin(), roots.end());
    }
    return primitive_roots;
}

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

std::vector<uint32_t> generate_cyclic_permutation(uint32_t approx_P, uint32_t approx_shift) {
    uint32_t P = get_largest_prime_less_than(approx_P);
    printf("\tP = %u\n", P);

    auto primitive_roots = get_primitive_roots(P);

    auto it = std::lower_bound(primitive_roots.begin(), primitive_roots.end(), approx_shift);
    assert(it != primitive_roots.end());

    uint32_t shift = *it;
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