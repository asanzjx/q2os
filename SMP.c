/***************************************************
*
***************************************************/

#include "SMP.h"


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
void SMP_init()
{
	int i;
	unsigned int a,b,c,d;

	// 0x0 get local APIC ID
	// 值得注意的是，在2.6.8版本的Bochs虚拟机中，CPUID.0Bh的枚举结果可能有误
	for(i = 0;;i++)
	{
		get_cpuid(0xb,i,&a,&b,&c,&d);
		if((c >> 8 & 0xff) == 0)
			break;
		color_printk(WHITE,BLACK,"local APIC ID Package_../Core_2/SMT_1,type(%x) Width:%#010x,num of logical processor(%x)\n",c >> 8 & 0xff,a & 0x1f,b & 0xff);
	}
	
	color_printk(WHITE,BLACK,"x2APIC ID level:%#010x\tx2APIC ID the current logical processor:%#010x\n",c & 0xff,d);
	
	color_printk(WHITE,BLACK,"[+]SMP copy byte:%#010x\n",(unsigned long)&_APU_boot_end - (unsigned long)&_APU_boot_start);
	memcpy(_APU_boot_start,(unsigned char *)0xffff800000020000,(unsigned long)&_APU_boot_end - (unsigned long)&_APU_boot_start);

	// 0x1 spin lock
	spin_init(&SMP_lock);
	// 0x2 init AP interrupt
// /*
	for(i = 200;i < 210;i++)
	{
		// set_intr_gate(i , 2 , SMP_interrupt[i - 200]);
		set_intr_gate(i , 0, SMP_interrupt[i - 200]);
	}
	memset(SMP_IPI_desc,0,sizeof(irq_desc_T) * 10);
// */

	// 0x3 init IPI
	// =============== BSP send IPI message
	struct INT_CMD_REG icr_entry;
	icr_entry.vector = 0x00;
	icr_entry.deliver_mode =  APIC_ICR_IOAPIC_INIT;
	icr_entry.dest_mode = ICR_IOAPIC_DELV_PHYSICAL;
	icr_entry.deliver_status = APIC_ICR_IOAPIC_Idle;
	icr_entry.res_1 = 0;
	icr_entry.level = ICR_LEVEL_DE_ASSERT;
	icr_entry.trigger = APIC_ICR_IOAPIC_Edge;
	icr_entry.res_2 = 0;
	icr_entry.dest_shorthand = ICR_ALL_EXCLUDE_Self;
	icr_entry.res_3 = 0;
	icr_entry.destination.x2apic_destination = 0x00;
	
	wrmsr(0x830,*(unsigned long *)&icr_entry);	//INIT IPI

#if Bochs
    color_printk(RED,YELLOW,"\n[+]APU start init intr...\n");
    // BochsMagicBreakpoint();
#endif

	// 0x4 Startup IPI
	// 1 BSP, 3 APs
	for(global_i = 1; global_i < 4; global_i++) {
		spin_lock(&SMP_lock);
		_stack_start = (unsigned long)kmalloc(STACK_SIZE, 0) + STACK_SIZE;
		// ((struct task_struct *)(_stack_start - STACK_SIZE))->cpu_id = global_i;

		unsigned int *tss = (unsigned int *)kmalloc(128,0);

		set_tss_descriptor(10 + global_i * 2,tss);
		set_tss64(tss,_stack_start,_stack_start,_stack_start,_stack_start,_stack_start,_stack_start,_stack_start,_stack_start,_stack_start,_stack_start);
		
		icr_entry.vector = 0x20;
		icr_entry.deliver_mode = ICR_Start_up;
		icr_entry.dest_shorthand = ICR_No_Shorthand;
		icr_entry.destination.x2apic_destination = global_i;
		
		wrmsr(0x830,*(unsigned long *)&icr_entry);	//Start-up IPI
		wrmsr(0x830,*(unsigned long *)&icr_entry);	//Start-up IPI
	}

	// 0x5 test IPI message, send IPI to cpu1
	icr_entry.vector = 0xc8;
	icr_entry.destination.x2apic_destination = 1;
	icr_entry.deliver_mode = APIC_ICR_IOAPIC_Fixed;
	wrmsr(0x830, *(unsigned long *)&icr_entry);
	icr_entry.vector = 0xc9;
	wrmsr(0x830, *(unsigned long *)&icr_entry);
    // =============== 


//	register_IPI(200,NULL,&IPI_0x200,NULL,NULL,"IPI 0x200");
	
	color_printk(RED,YELLOW,"\n[+]APU end init...\n");
#if Bochs
    // BochsMagicBreakpoint();
#endif
}

void Start_SMP()
{
	unsigned int x,y;
#if Bochs
	// BochsMagicBreakpoint();
#endif
	color_printk(RED,YELLOW,"\n\tAP %#010x starting......\n", global_i);

	//enable xAPIC & x2APIC
	__asm__ __volatile__(	"movq 	$0x1b,	%%rcx	\n\t"
				"rdmsr	\n\t"
				"bts	$10,	%%rax	\n\t"
				"bts	$11,	%%rax	\n\t"
				"wrmsr	\n\t"
				"movq 	$0x1b,	%%rcx	\n\t"
				"rdmsr	\n\t"
				:"=a"(x),"=d"(y)
				:
				:"memory");


    int ap_xapic = 0;
	if(x&0xc00)
        ap_xapic = 1;
		color_printk(RED,YELLOW,"\tAP xAPIC & x2APIC enabled\n");

    //enable SVR[8] SVR[12]
#if Bochs
	__asm__ __volatile__(	"movq 	$0x80f,	%%rcx	\n\t"
				"rdmsr	\n\t"
				"bts	$8,	%%rax	\n\t"
				"wrmsr	\n\t"
				"movq 	$0x80f,	%%rcx	\n\t"
				"rdmsr	\n\t"
				:"=a"(x),"=d"(y));
#else
	__asm__ __volatile__(	"movq 	$0x80f,	%%rcx	\n\t"
				"rdmsr	\n\t"
				"bts	$8,	%%rax	\n\t"
				"bts	$12,	%%rax\n\t"
				"wrmsr	\n\t"
				"movq 	$0x80f,	%%rcx	\n\t"
				"rdmsr	\n\t"
				:"=a"(x),"=d"(y)
				:
				:"memory");
#endif
    int ap_svr8 = 0;
    int ap_svr12 = 0;
	if(x&0x100)
        ap_svr8 = 1;
		// color_printk(RED,YELLOW,"\tSVR[8] enabled\n");
	if(x&0x1000)
        ap_svr12 = 1;
		// color_printk(RED,YELLOW,"\tSVR[12] enabled\n");

	//get local APIC ID
	__asm__ __volatile__(	"movq $0x802,	%%rcx	\n\t"
				"rdmsr	\n\t"
				:"=a"(x),"=d"(y)
				:
				:"memory");
	
	color_printk(RED,YELLOW,"\tx2APIC ID:%#010x\t",x);

	memset(current,0,sizeof(struct task_struct));
	// load_TR(12);
	load_TR(10 + (global_i -1) * 2);
	spin_unlock(&SMP_lock);
	color_printk(RED,YELLOW,"\tAPU %#010x init end, starting hlt...\n", global_i);

#if Bochs
	// BochsMagicBreakpoint();
#endif

	sti();

	// ==== multi core exception test
	// int i = 1/0;
	// ====

	while(1){
        hlt();
    }
}
