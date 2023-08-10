#include <sys/mman.h>
#include <cstring>
#include <unistd.h>
#include <cstdio>
#include <cassert>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <cstdint>


char* get_jmp_addr(char* jump_base, uint32_t i) {
    return jump_base + (i * 7);
}

char* get_target_addr(char* target_base, uint32_t i) {
    return target_base + (i * 8);
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

std::vector<uint32_t> generate_single_cycle_permutation(uint32_t N) {
    std::vector<uint32_t> P(N - 1);
    for (uint32_t i = 0; i < N - 1; i++) {
        P[i] = i + 1;
    }

    std::vector<uint32_t> permutation(N, 0);
    uint32_t Idx = 0;
    std::random_device rd;
    std::mt19937 gen(rd());
    uint32_t size = N - 1;

    while (size > 0) {
        std::uniform_int_distribution<uint32_t> dis(0, size - 1);
        uint32_t random_index = dis(gen);
        uint32_t M = P[random_index];
        permutation[Idx] = M;
        P[random_index] = P[size - 1];
        size--;
        Idx = M;
    }
    permutation[Idx] = 0;
    return permutation;
}

int main() {

    const uint32_t N = 1 << 26;
    const uint32_t P = get_largest_prime_less_than(N); 
    printf("N = %u, P = %u\n", N, P);

    const size_t size = 1ULL << 30; // 1GB
    const int prot = PROT_READ | PROT_WRITE | PROT_EXEC;
    const int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    const int fd = -1;
    const off_t offset = 0;

    assert(P < (size / (2 * 8))); // make sure that N fits

    printf("Generating permutation...\n");
    auto perm = generate_single_cycle_permutation(P);
    uint32_t cycle_length = 0;
    uint32_t current = 0;
    do {
        current = perm[current];
        cycle_length++;
    } while (current != 0);
    assert(cycle_length == P);

    printf("Mmap...\n");
    char* addr = (char*)mmap((void*)0x1000, size, prot, flags, fd, offset);
    if (addr == MAP_FAILED) {
        printf("mmap failed\n");
        return 1;
    }
    std::memset(addr, 0, size);

    char* jump_instrs = addr;
    char* jump_targets = addr + (size / 2);

    printf("Preparing instrs, jump_instrs = %p, jump_targets = %p...\n", jump_instrs, jump_targets);
    for (uint32_t i = 0; i < P; i++) {
        char* J = get_jmp_addr(jump_instrs, i);
        char* T = get_target_addr(jump_targets, i);

        assert((uintptr_t)J <= (1 << 31));
        uint32_t Ti = (uint32_t)T;

        char* nextJ = get_jmp_addr(jump_instrs, perm.at(i));

        assert((uintptr_t)nextJ <= (1 << 31));
        *(uint32_t*)T = (uint32_t)nextJ;

        J[0] = 0xff;
        J[1] = 0x24;
        J[2] = 0x25;
        *(uint32_t*)(J + 3) = Ti;
    }

    printf("Jumping to start. PID = %d\n", (int)getpid());
    asm volatile ("jmp *%0" : : "r"(jump_instrs));

    assert(false); // Will never get here

    return 0;
}
