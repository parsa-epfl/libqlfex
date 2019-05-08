#include "qemu/osdep.h"            // Added for flexus because in the new version of QEMU qmp-commands.h uses type error that it doesnt call itself . osdep calls it for qmp-commands.h
#include "cpu.h"
#include "qom/cpu.h"
#include "qemu/config-file.h"
#include "sysemu/cpus.h"
#include "qmp-commands.h"
#include "include/exec/exec-all.h"
#include "armflex-helper.h"

#define ARCH_NUM_REGS           (32)
#define ARCH_XREGS_SIZE         (ARCH_NUM_REGS * sizeof(uint64_t))
#define ARCH_XREGS_OFFST        (0)
#define ARCH_PC_OFFST           (32 * 2)
#define ARCH_PSTATE_FLAG_OFFST  (ARCH_PC_OFFST + 2)
#define ARCH_PSTATE_CF_OFFST    (ARCH_PSTATE_FLAG_OFFST + 0)
#define ARCH_PSTATE_VF_OFFST    (ARCH_PSTATE_FLAG_OFFST + 1)
#define ARCH_PSTATE_NF_OFFST    (ARCH_PSTATE_FLAG_OFFST + 2)
#define ARCH_PSTATE_ZF_OFFST    (ARCH_PSTATE_FLAG_OFFST + 3)

void* armflex_pack_archstate(CPUARMState *cpu) {
    uint32_t *buffer = malloc((ARCH_PSTATE_ZF_OFFST + 1) * sizeof(uint32_t));

    memcpy(&buffer[ARCH_XREGS_OFFST],     &cpu->xregs, ARCH_XREGS_SIZE);
    memcpy(&buffer[ARCH_PC_OFFST],        &cpu->pc, sizeof(uint64_t));
    memcpy(&buffer[ARCH_PSTATE_CF_OFFST], &cpu->CF, sizeof(uint32_t));
    memcpy(&buffer[ARCH_PSTATE_VF_OFFST], &cpu->VF, sizeof(uint32_t));
    memcpy(&buffer[ARCH_PSTATE_NF_OFFST], &cpu->NF, sizeof(uint32_t));
    memcpy(&buffer[ARCH_PSTATE_ZF_OFFST], &cpu->ZF, sizeof(uint32_t));

    return buffer;
}

void armflex_unpack_archstate(uint32_t *buffer, CPUARMState *cpu) {
    memcpy(&cpu->xregs, &buffer[0], ARCH_XREGS_SIZE);
    cpu->pc = ((uint64_t) buffer[ARCH_PC_OFFST + 1] << 32) | buffer[ARCH_PC_OFFST];
    cpu->CF = buffer[ARCH_PSTATE_CF_OFFST];
    cpu->VF = buffer[ARCH_PSTATE_VF_OFFST];
    cpu->NF = buffer[ARCH_PSTATE_NF_OFFST];
    cpu->ZF = buffer[ARCH_PSTATE_ZF_OFFST];
}

int armflex_write_page(void* src, target_ulong size) {

    int fd = -1;
    void *region;
    if (mkdir("/tmp/armflex", 0777) && errno != EEXIST) {
        printf("mkdir /tmp/armflex");
        return 1;
    }
    if((fd = open("/tmp/armflex/program_page", O_RDWR | O_CREAT, 0666)) == -1) {
        printf( "Program Page dest file: open failed");
        return 1;
    }
    if (lseek(fd, size-1, SEEK_SET) == -1) {
        close(fd);
        perror("Error calling lseek() to 'stretch' the file");
        return 1;
    }
    if (write(fd, "", 1) != 1) {
        close(fd);
        perror("Error writing last byte of the file");
        return 1;
    }
    region = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(region == MAP_FAILED) {
        printf("Program Page dest file: mmap failed");
        return 1;
    }

    memcpy(region, src, size);
    munmap(region, size);
    close(fd);
    return 0;
}

void armflex_page_sanity_check(size_t offst) {
    int fd = -1;
    if((fd = open("/tmp/armflex/program_page", O_RDWR, 0666)) == -1) {
        printf( "Program Page dest file: open failed");
    }
    if (lseek(fd, offst, SEEK_SET) == -1) {
        close(fd);
        perror("Error calling lseek() to 'stretch' the file");
    }
    uint32_t insns;
    int bytes = read(fd, &insns, 4);
    qemu_log_mask(ARMFLEX_LOG,
                  "BYTES:%d|PHYS_INS_FROM_PAGE:0x%x\n\n", bytes, insns);
    close(fd);
}
