#include "../qemu/include/exec/cpu-defs.h"

void* armflex_pack_archstate(CPUARMState *cpu);
void armflex_unpack_archstate(uint32_t *buffer, CPUARMState *cpu);
int armflex_write_page(void* src, target_ulong size);
void armflex_page_sanity_check(size_t offst);
