/***************************************************
*
*
***************************************************/

#include "backtrace.h"
#include "printk.h"

extern unsigned long kallsyms_addresses[] __attribute__((weak));;
extern long kallsyms_syms_num __attribute__((weak));;
extern long kallsyms_index[] __attribute__((weak));;
extern char* kallsyms_names __attribute__((weak));;

int lookup_kallsyms(unsigned long address)
{
	int index = 0;
	char * string =(char *) &kallsyms_names;
	for(index = 0;index<kallsyms_syms_num;index++)
		if(address > kallsyms_addresses[index] && address <= kallsyms_addresses[index+1])
			break;
	if(index < kallsyms_syms_num)
	{
		color_printk(WHITE,BLUE,"backtrace address:%#018lx (+) %04d\tbacktrace function:%s(%#018lx)\n",address,address - kallsyms_addresses[index],&string[kallsyms_index[index]],kallsyms_addresses[index]);
		return 0;
	}
	else
		return 1;
}

void backtrace(struct pt_regs * regs)
{
	unsigned long *rbp = (unsigned long *)regs->rbp;
	unsigned long ret_address = *(rbp+1);
	int i = 0;

	color_printk(RED,BLACK,"====================== Kernel Stack Backtrace ======================\n");

	lookup_kallsyms(regs->rip);
	for(i = 0;i<10;i++)
	{
		if(lookup_kallsyms(ret_address))
			break;
		rbp = (unsigned long *)*rbp;
		ret_address = *(rbp+1);
	}
}

void display_regs(struct pt_regs * regs)
{
	color_printk(RED,BLACK,"====================== Kernel Regs ======================\n");
	color_printk(RED,BLACK,"CS:%#010x,SS:%#010x\nDS:%#010x,ES:%#010x\nRFLAGS:%#018lx\n",regs->cs,regs->ss,regs->ds,regs->es,regs->rflags);
	color_printk(RED,BLACK,"RAX:%#018lx,RBX:%#018lx,RCX:%#018lx,RDX:%#018lx\nRSP:%#018lx,RBP:%#018lx,RIP:%#018lx\nRSI:%#018lx,RDI:%#018lx\n",regs->rax,regs->rbx,regs->rcx,regs->rdx,regs->rsp,regs->rbp,regs->rip,regs->rsi,regs->rdi);
	color_printk(RED,BLACK,"R8 :%#018lx,R9 :%#018lx\nR10:%#018lx,R11:%#018lx\nR12:%#018lx,R13:%#018lx\nR14:%#018lx,R15:%#018lx\n",regs->r8,regs->r9,regs->r10,regs->r11,regs->r12,regs->r13,regs->r14,regs->r15);
	backtrace(regs);
}

