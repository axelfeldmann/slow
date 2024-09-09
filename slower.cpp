// NOTE: A lot of the KVM setup code is copied directly from
// https://github.com/dpw/kvm-hello-world

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <linux/kvm.h>
#include <string>

#include "primes.h"

/* CR0 bits */
#define CR0_PE 1u
#define CR0_MP (1U << 1)
#define CR0_EM (1U << 2)
#define CR0_TS (1U << 3)
#define CR0_ET (1U << 4)
#define CR0_NE (1U << 5)
#define CR0_WP (1U << 16)
#define CR0_AM (1U << 18)
#define CR0_NW (1U << 29)
#define CR0_CD (1U << 30)
#define CR0_PG (1U << 31)

/* CR4 bits */
#define CR4_VME 1
#define CR4_PVI (1U << 1)
#define CR4_TSD (1U << 2)
#define CR4_DE (1U << 3)
#define CR4_PSE (1U << 4)
#define CR4_PAE (1U << 5)
#define CR4_MCE (1U << 6)
#define CR4_PGE (1U << 7)
#define CR4_PCE (1U << 8)
#define CR4_OSFXSR (1U << 8)
#define CR4_OSXMMEXCPT (1U << 10)
#define CR4_UMIP (1U << 11)
#define CR4_VMXE (1U << 13)
#define CR4_SMXE (1U << 14)
#define CR4_FSGSBASE (1U << 16)
#define CR4_PCIDE (1U << 17)
#define CR4_OSXSAVE (1U << 18)
#define CR4_SMEP (1U << 20)
#define CR4_SMAP (1U << 21)

#define EFER_SCE 1
#define EFER_LME (1U << 8)
#define EFER_LMA (1U << 10)
#define EFER_NXE (1U << 11)

/* 32-bit page directory entry bits */
#define PDE32_PRESENT 1
#define PDE32_RW (1U << 1)
#define PDE32_USER (1U << 2)
#define PDE32_PS (1U << 7)

/* 64-bit page * entry bits */
#define PDE64_PRESENT 1
#define PDE64_RW (1U << 1)
#define PDE64_USER (1U << 2)
#define PDE64_ACCESSED (1U << 5)
#define PDE64_DIRTY (1U << 6)
#define PDE64_PS (1U << 7)
#define PDE64_G (1U << 8)

struct vm {
    int sys_fd;
    int fd;
    char *mem;
};

void vm_init(struct vm *vm, size_t mem_size) {
    int api_ver;
    struct kvm_userspace_memory_region memreg;

    vm->sys_fd = open("/dev/kvm", O_RDWR);
    if (vm->sys_fd < 0) {
        perror("open /dev/kvm");
        exit(1);
    }

    api_ver = ioctl(vm->sys_fd, KVM_GET_API_VERSION, 0);
    if (api_ver < 0) {
        perror("KVM_GET_API_VERSION");
        exit(1);
    }

    if (api_ver != KVM_API_VERSION) {
        fprintf(stderr, "Got KVM api version %d, expected %d\n",
            api_ver, KVM_API_VERSION);
        exit(1);
    }

    vm->fd = ioctl(vm->sys_fd, KVM_CREATE_VM, 0);
    if (vm->fd < 0) {
        perror("KVM_CREATE_VM");
        exit(1);
    }

        if (ioctl(vm->fd, KVM_SET_TSS_ADDR, 0xfffbd000) < 0) {
                perror("KVM_SET_TSS_ADDR");
        exit(1);
    }

    vm->mem = (char*)mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (vm->mem == MAP_FAILED) {
        perror("mmap mem");
        exit(1);
    }

    madvise(vm->mem, mem_size, MADV_MERGEABLE | MADV_NOHUGEPAGE);

    memreg.slot = 0;
    memreg.flags = 0;
    memreg.guest_phys_addr = 0;
    memreg.memory_size = mem_size;
    memreg.userspace_addr = (unsigned long)vm->mem;
        if (ioctl(vm->fd, KVM_SET_USER_MEMORY_REGION, &memreg) < 0) {
        perror("KVM_SET_USER_MEMORY_REGION");
                exit(1);
    }
}

struct vcpu {
    int fd;
    struct kvm_run *kvm_run;
};

void vcpu_init(struct vm *vm, struct vcpu *vcpu) {
    int vcpu_mmap_size;

    vcpu->fd = ioctl(vm->fd, KVM_CREATE_VCPU, 0);
        if (vcpu->fd < 0) {
        perror("KVM_CREATE_VCPU");
                exit(1);
    }

    vcpu_mmap_size = ioctl(vm->sys_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
        if (vcpu_mmap_size <= 0) {
        perror("KVM_GET_VCPU_MMAP_SIZE");
                exit(1);
    }

    vcpu->kvm_run = (struct kvm_run*)mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE,
                 MAP_SHARED, vcpu->fd, 0);
    if (vcpu->kvm_run == MAP_FAILED) {
        perror("mmap kvm_run");
        exit(1);
    }
}

void run_vm(struct vcpu *vcpu) {
    printf("Starting VM. PID = %d\n", (int)getpid());
    if (ioctl(vcpu->fd, KVM_RUN, 0) < 0) {
        perror("KVM_RUN");
        exit(1);
    }
    fprintf(stderr, "exit_reason = %d\n", vcpu->kvm_run->exit_reason);
    exit(1);
}

static void setup_protected_mode(struct kvm_sregs *sregs) {
    struct kvm_segment seg = {
        .base = 0,
        .limit = 0xffffffff,
        .selector = 1 << 3,
        .type = 11, /* Code: execute, read, accessed */
        .present = 1,
        .dpl = 0,
        .db = 1,
        .s = 1, /* Code/data */
        .l = 0,
        .g = 1, /* 4KB granularity */
    };

    sregs->cr0 |= CR0_PE; /* enter protected mode */

    sregs->cs = seg;

    seg.type = 3; /* Data: read/write, accessed */
    seg.selector = 2 << 3;
    sregs->ds = sregs->es = sregs->fs = sregs->gs = sregs->ss = seg;
}

static void setup_paged_32bit_mode(struct vm *vm, struct kvm_sregs *sregs) {

    uint32_t next_frame = 0xFFFFF000;
    uint32_t pd_addr = next_frame;
    uint32_t* pd = (uint32_t*)(vm->mem + pd_addr);
    for (uint32_t i = 0; i < 1022; i++) {
        next_frame -= 0x1000;
        uint32_t pt_addr = next_frame;
        pd[i] = PDE32_PRESENT | PDE32_RW | PDE32_USER | pt_addr;

        uint32_t* pt = (uint32_t*)(vm->mem + pt_addr);

        for (uint32_t j = 0; j < 1024; j++) {
            uint32_t frame = (i * 1024u + j) * 0x1000u;
            pt[j] = PDE32_PRESENT | PDE32_RW | PDE32_USER | frame;
        }
    }

    sregs->cr3 = pd_addr;
    sregs->cr4 = 0;
    sregs->cr0
        = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM | CR0_PG;
    sregs->cr0 = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_AM | CR0_PG;
    sregs->efer = 0;
}

void run_paged_32bit_mode(struct vm *vm, struct vcpu *vcpu, uint32_t approx_shift)
{
    struct kvm_sregs sregs;
    struct kvm_regs regs;

    if (ioctl(vcpu->fd, KVM_GET_SREGS, &sregs) < 0) {
        perror("KVM_GET_SREGS");
        exit(1);
    }

    setup_protected_mode(&sregs);
    setup_paged_32bit_mode(vm, &sregs);

    if (ioctl(vcpu->fd, KVM_SET_SREGS, &sregs) < 0) {
        perror("KVM_SET_SREGS");
        exit(1);
    }

    memset(&regs, 0, sizeof(regs));

    /* Clear all FLAGS bits, except bit 1 which is always set. */
    regs.rflags = 2;
    regs.rip = 0x1000;

    if (ioctl(vcpu->fd, KVM_SET_REGS, &regs) < 0) {
        perror("KVM_SET_REGS");
        exit(1);
    }

    auto perm = generate_cyclic_permutation(1 << 26, approx_shift);

    constexpr size_t size = 1 << 30;
    uint32_t jump_base = 0x1000;
    uint32_t target_base = jump_base + (size / 2);

    auto get_jmp_addr = [=](uint32_t i) -> uint32_t {
        return (jump_base + (i * 7));
    };

    auto get_target_addr = [=](uint32_t i) -> uint32_t {
        return (target_base + (i * 8));
    };

    for (uint32_t i = 0; i < perm.size(); i++) {
        uint32_t J = get_jmp_addr(i);
        uint32_t T = get_target_addr(i);

        uint32_t Ti = (uint32_t)(uintptr_t)T;
        uint32_t nextJ = get_jmp_addr(perm.at(i));

        *(uint32_t*)(vm->mem + T) = (uint32_t)(uintptr_t)nextJ;

        vm->mem[J+0] = 0xff;
        vm->mem[J+1] = 0x24;
        vm->mem[J+2] = 0x25;
        *(uint32_t*)(vm->mem + J + 3) = Ti;
    }
    run_vm(vcpu); // never returns
}

int main(int argc, char** argv) {

    if (argc != 2) {
        printf("Usage: %s <approx_shift>\n", argv[0]);
        return 1;
    }

    uint32_t approx_shift = std::stoi(argv[1]);

    struct vm vm;
    struct vcpu vcpu;
    vm_init(&vm, 1l << 32);
    vcpu_init(&vm, &vcpu);
    run_paged_32bit_mode(&vm, &vcpu, approx_shift);
    return 0;
}
