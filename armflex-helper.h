#include "../qemu/include/exec/cpu-defs.h"

#define ARCH_STATE_SIZE         ((70) * sizeof(uint32_t))

#define ARMFLEX_ROOT_DIR        "/tmp/armflex"

void* armflex_pack_archstate(CPUARMState *cpu);
void armflex_unpack_archstate(uint32_t *buffer, CPUARMState *cpu);
int armflex_write_file(const char *filename, void* buffer, target_ulong size);
void armflex_page_sanity_check(const char* filename, size_t offst,  uint32_t expected);
