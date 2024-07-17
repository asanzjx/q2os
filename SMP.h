/***************************************************
*
***************************************************/

#ifndef __SMP_H__
#define __SMP_H__


#include "lib.h"
#include "gate.h"
#include "spinlock.h"
#include "printk.h"
#include "APIC.h"
#include "interrupt.h"
#include "task.h"



#define SMP_cpu_id()	(current->cpu_id)


//////////////////////////////////////////////////
//					extern vars					//
//////////////////////////////////////////////////
extern unsigned char _APU_boot_start[];
extern unsigned char _APU_boot_end[];

extern spinlock_T SMP_lock;
extern int global_i;
extern irq_desc_T SMP_IPI_desc[10];
extern void (* SMP_interrupt[10])(void);

// 所有 CPU 的 idle 进程的 PCB，其中 BSP 采用静态创建，AP 的 PCB 在 BSP 中动态创建
extern struct task_struct *init_task[NR_CPUS];



//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

void SMP_init();

void Start_SMP();

#endif
