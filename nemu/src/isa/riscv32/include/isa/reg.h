#ifndef __RISCV32_REG_H__
#define __RISCV32_REG_H__

#include "common.h"

#define PC_START (0x80000000u + IMAGE_START)

typedef struct
{
  struct
  {
    rtlreg_t _32; // typedef uint32_t rtlreg_t
  } gpr[32];
  rtlreg_t sepc;
  rtlreg_t sstatus;
  rtlreg_t scause;
  rtlreg_t stvec;
  vaddr_t pc; // typedef uint32_t vaddr_t

} CPU_state;

static inline int check_reg_index(int index)
{
  assert(index >= 0 && index < 32);
  return index;
}

#define reg_l(index) (cpu.gpr[check_reg_index(index)]._32)

static inline const char *reg_name(int index, int width)
{
  extern const char *regsl[];
  assert(index >= 0 && index < 32);
  return regsl[index];
}

#endif
