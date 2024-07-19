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
//					Functions					//
//////////////////////////////////////////////////

void SMP_init();

void Start_SMP();

#endif
