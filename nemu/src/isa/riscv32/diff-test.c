#include "nemu.h"
#include "monitor/diff-test.h"

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc)
{
  if (cpu.pc != ref_r->pc)
    return false;
  else
  {
    for (int i = 0; i < 32; i++)
    {
      if (cpu.gpr[0]._32 != ref_r->gpr[0]._32)
        return false;
    }
  }
  return true;
}

void isa_difftest_attach(void)
{
}
