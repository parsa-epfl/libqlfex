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
#define ARCH_PC_OFFST           (32 * 2)                        // 64
#define ARCH_PSTATE_FLAG_OFFST  (ARCH_PC_OFFST + 2)             // 66
#define ARCH_PSTATE_CF_OFFST    (ARCH_PSTATE_FLAG_OFFST + 0)    // 66
#define ARCH_PSTATE_VF_OFFST    (ARCH_PSTATE_FLAG_OFFST + 1)    // 67
#define ARCH_PSTATE_NF_OFFST    (ARCH_PSTATE_FLAG_OFFST + 2)    // 68
#define ARCH_PSTATE_ZF_OFFST    (ARCH_PSTATE_FLAG_OFFST + 3)    // 69


void* armflex_pack_archstate(CPUARMState *cpu) {
    uint32_t *buffer = malloc(ARCH_STATE_SIZE);

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

int armflex_write_file(const char *filename, void* buffer, target_ulong size) {

    char filepath[PATH_MAX];
    int fd = -1;
    void *region;

    if (mkdir(ARMFLEX_ROOT_DIR, 0777) && errno != EEXIST) {
        printf("'mkdir "ARMFLEX_ROOT_DIR"' failed\n");
        return 1;
    }
    sprintf(filepath, ARMFLEX_ROOT_DIR"/%s", filename);
    if((fd = open(filepath, O_RDWR | O_CREAT, 0666)) == -1) {
        printf("Program Page dest file: open failed\nfilepath:%s\n", filepath);
        return 1;
    }
    if (lseek(fd, size-1, SEEK_SET) == -1) {
        close(fd);
        printf("Error calling lseek() to 'stretch' the file\n");
        return 1;
    }
    if (write(fd, "", 1) != 1) {
        close(fd);
        printf("Error writing last byte of the file\n");
        return 1;
    }

    region = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(region == MAP_FAILED) {
        close(fd);
        printf("Error dest file: mmap failed");
        return 1;
    }

    memcpy(region, buffer, size);
    msync(region, size, MS_SYNC);
    munmap(region, size);

    close(fd);
    return 0;
}

void armflex_page_sanity_check(const char* filename, size_t offst,  uint32_t expected) {
    char filepath[PATH_MAX];
    int fd = -1;
    uint32_t res;
    int bytes;

    sprintf(filepath, ARMFLEX_ROOT_DIR"/%s", filename);
    if((fd = open(filepath, O_RDWR, 0666)) == -1) {
        printf("Program Page dest file: open failed.\nfilepath:%s\n", filepath);
    }
    if (lseek(fd, offst, SEEK_SET) == -1) {
        close(fd);
        printf("lseek failed on: %s", filepath);
    }
    bytes = read(fd, &res, 4);
    assert(res == expected);
    close(fd);
}
