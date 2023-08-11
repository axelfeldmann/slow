#include <sys/mman.h>
#include <cstring>
#include <unistd.h>
#include <cstdio>
#include <cassert>
#include "primes.h"

char* get_jmp_addr(char* jump_base, uint32_t i) {
    return jump_base + (i * 7);
}

char* get_target_addr(char* target_base, uint32_t i) {
    return target_base + (i * 8);
}

int main() {

    printf("Generating permutation...\n");
    auto perm = generate_cyclic_permutation(1 << 26, 100'000);

    // Allocate the instruction and target buffers
    const size_t size = 1ULL << 30;
    const int prot = PROT_READ | PROT_WRITE | PROT_EXEC;
    const int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    const int fd = -1;
    const off_t offset = 0;

    assert(perm.size() < (size / (2 * 8))); // make sure that N fits

    char* addr = (char*)mmap((void*)0x1000, size, prot, flags, fd, offset);
    if (addr == MAP_FAILED) {
        printf("mmap failed\n");
        return 1;
    }
    std::memset(addr, 0, size);

    // Write the instructions and target buffers
    char* jump_instrs = addr;
    char* jump_targets = addr + (size / 2);
    printf("Preparing instrs, jump_instrs = %p, jump_targets = %p...\n", jump_instrs, jump_targets);
    
    for (uint32_t i = 0; i < perm.size(); i++) {
        char* J = get_jmp_addr(jump_instrs, i);
        char* T = get_target_addr(jump_targets, i);

        assert((uintptr_t)J <= (1 << 31));
        uint32_t Ti = (uint32_t)(uintptr_t)T;

        char* nextJ = get_jmp_addr(jump_instrs, perm.at(i));

        assert((uintptr_t)nextJ <= (1 << 31));
        *(uint32_t*)T = (uint32_t)(uintptr_t)nextJ;

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
