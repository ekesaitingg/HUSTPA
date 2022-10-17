#include "rtl/rtl.h"
#include "cpu/exec.h"

void raise_intr(uint32_t NO, vaddr_t epc)
{
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */
  cpu.sepc = epc;
  cpu.scause = NO;
  decinfo.jmp_pc = decinfo.isa.stvec;
  decinfo_set_jmp(true);
}

bool isa_query_intr(void)
{
  return false;
}
