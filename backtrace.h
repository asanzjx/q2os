/***************************************************
*
*
***************************************************/

#ifndef __BACKTRACE_H__
#define __BACKTRACE_H__

#include "lib.h"

void backtrace(struct pt_regs * regs);
void display_regs(struct pt_regs * regs);
#endif

