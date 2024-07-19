/***************************************************
*
* HEPT.c
*
****************************************************/

#include "HPET.h"
#include "printk.h"
#include "memory.h"
#include "task.h"
#include "schedule.h"
#include "SMP.h"

hw_int_controller HPET_int_controller = 
{
	.enable = IOAPIC_enable,
	.disable = IOAPIC_disable,
	.install = IOAPIC_install,
	.uninstall = IOAPIC_uninstall,
	.ack = IOAPIC_edge_ack,
};

//////////////////////////////////////////////////
//					extern vars					//
//////////////////////////////////////////////////
// define in main.c
extern struct time time;
extern unsigned long volatile jiffies;
extern struct timer_list timer_list_head;

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////
void HPET_handler(unsigned long nr, unsigned long parameter, struct pt_regs * regs)
{
	// color_printk(RED,WHITE,"(HPET handle)");
	struct INT_CMD_REG icr_entry;
	jiffies++;

	memset(&icr_entry,0,sizeof(struct INT_CMD_REG));
	icr_entry.vector = 0xc8;
	icr_entry.dest_shorthand = ICR_ALL_EXCLUDE_Self;
	icr_entry.trigger = APIC_ICR_IOAPIC_Edge;
	icr_entry.dest_mode = ICR_IOAPIC_DELV_PHYSICAL;
	icr_entry.deliver_mode = APIC_ICR_IOAPIC_Fixed;
	wrmsr(0x830,*(unsigned long *)&icr_entry);

    // set_softirq_status(TIMER_SIRQ);
	if((container_of(list_next(&timer_list_head.list),struct timer_list,list)->expect_jiffies <= jiffies))
        set_softirq_status(TIMER_SIRQ);

	switch(current->priority)
    {
        case 0:
        case 1:
            task_schedule[SMP_cpu_id()].CPU_exec_task_jiffies--;
            current->vrun_time += 1;
            break;
        case 2:
        default:
            task_schedule[SMP_cpu_id()].CPU_exec_task_jiffies -= 2;
            current->vrun_time += 2;
            break;
    }
    if(task_schedule[SMP_cpu_id()].CPU_exec_task_jiffies <= 0)
        current->flags |= NEED_SCHEDULE;

	#if DEBUG
/*
		struct task_struct *tsk = NULL;
		tsk = get_next_task();
		color_printk(RED,BLACK,"[+]%s #cur 0x%p, list prev->0x%p, next->0x%p, pid: %d, vrun_time:%d; \
		\n\t next task :0x%p, list prev->0x%p, next->0x%p, pid:%d, vrun_time:%d;	\
		\n\t task_schedule: 0x%p, CPU_exec_task_jiffies: %d, task_queue: 0x%p\n", \
		__func__, current, current->list.prev, current->list.next, current->pid, current->vrun_time, \
		tsk, tsk->list.prev, tsk->list.next, tsk->pid, tsk->vrun_time,	\
		&task_schedule, task_schedule.CPU_exec_task_jiffies, task_schedule.task_queue);
		BochsMagicBreakpoint();
*/
	#endif

}

void HPET_init() {
    unsigned int x;
    unsigned int * p;
    unsigned char * HPET_addr = (unsigned char *)Phy_To_Virt(0xfed00000);
    struct IO_APIC_RET_entry entry;

#if Bochs
	// can't  read RCBA
#else
    //get RCBA address
    io_out32(0xcf8,0x8000f8f0);
    x = io_in32(0xcfc);
    x = x & 0xffffc000;

    //get HPTC address
    if(x > 0xfec00000 && x < 0xfee00000)
    {
        p = (unsigned int *)Phy_To_Virt(x + 0x3404UL);
    }

    //enable HPET
    *p = 0x80;
    io_mfence();
#endif

    //init I/O APIC IRQ2 => HPET Timer 0
	entry.vector = 34;
	entry.deliver_mode = APIC_ICR_IOAPIC_Fixed ;
	entry.dest_mode = ICR_IOAPIC_DELV_PHYSICAL;
	entry.deliver_status = APIC_ICR_IOAPIC_Idle;
	entry.polarity = APIC_IOAPIC_POLARITY_HIGH;
	entry.irr = APIC_IOAPIC_IRR_RESET;
	entry.trigger = APIC_ICR_IOAPIC_Edge;
	entry.mask = APIC_ICR_IOAPIC_Masked;
	entry.reserved = 0;

	entry.destination.physical.reserved1 = 0;
	entry.destination.physical.phy_dest = 0;
	entry.destination.physical.reserved2 = 0;

	register_irq(34, &entry , &HPET_handler, NULL, &HPET_int_controller, "HPET");
	
	color_printk(RED,BLACK,"HPET - GCAP_ID:<%#018lx>\n",*(unsigned long *)HPET_addr);

	// 配置 GEN_CONF，使定时器 0 向 IOAPIC IRQ2 引脚发送中断请求 
	*(unsigned long *)(HPET_addr + 0x10) = 3;
	io_mfence();

	//edge triggered & periodic
	*(unsigned long *)(HPET_addr + 0x100) = 0x004c;
	io_mfence();

	// 1s
#if Bochs
	// *(unsigned long *)(HPET_addr + 0x108) = 14318179;

	// 20s
	*(unsigned long *)(HPET_addr + 0x108) = 14318179 * 20;
#else
	// intel 400 芯片组时间精度为 41.666667ns
	*(unsigned long *)(HPET_addr + 0x108) = 23999999;
#endif
	io_mfence();

	// get CMOS time & init MAIN_CNT
	get_cmos_time(&time);
	*(unsigned long *)(HPET_addr + 0xf0) = 0;
	io_mfence();
	
	color_printk(RED,BLACK,"year%#010x,month:%#010x,day:%#010x,hour:%#010x,mintue:%#010x,second:%#010x\n",time.year,time.month,time.day,time.hour,time.minute,time.second);
}