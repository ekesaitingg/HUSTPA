#include "common.h"
#include "syscall.h"
#include "fs.h"

_Context *do_syscall(_Context *c)
{
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;
  switch (a[0])
  {
  case SYS_exit: // syscall0
    _halt(a[1]);
    break;
  case SYS_yield: // syscall1
    _yield();
    break;
  case SYS_open: // syscall2
    c->GPRx = fs_open((char *)a[1], a[2], a[3]);
    break;
  case SYS_read: // syscall3
    c->GPRx = fs_read(a[1], (void *)a[2], a[3]);
    break;
  case SYS_write: // syscall4
    c->GPRx = fs_write(a[1], (void *)a[2], a[3]);
    break;
  case SYS_close: // syscall7
    c->GPRx = fs_close(a[1]);
    break;
  case SYS_lseek: // syscall8
    c->GPRx = fs_lseek(a[1], a[2], a[3]);
    break;
  case SYS_brk: // syscall9
    break;
  default:
    panic("Unhandled syscall ID = %d", a[0]);
  }

  return NULL;
}
