/***************************************************
*
* q2os kernel main
*
****************************************************/

#include "lib.h"
#include "printk.h"
#include "gate.h"
#include "trap.h"
#include "memory.h"
#include "task.h"
#include "interrupt.h"
#include "schedule.h"
#include "SMP.h"
#include "VFS.h"
#if APIC
#include "APIC.h"
#include "keyboard.h"
#include "mouse.h"
#include "disk.h"
#include "HPET.h"
#else
#include "8259A.h"
#endif





//////////////////////////////////////////////////
//					Constants					//
//////////////////////////////////////////////////
#define CMOS_READ(addr) ({ \
    io_out8(0x70,0x80 | addr); \
    io_in8(0x71); \
})



//////////////////////////////////////////////////
//					Global vars					//
//////////////////////////////////////////////////
/*************
    static var 
*/
// record screen info
struct position Pos = {0};

// APs lock
spinlock_T SMP_lock;

// cpus numbs, apic index id
int global_i = 0;

irq_desc_T SMP_IPI_desc[10] = {0};
void (* SMP_interrupt[10])(void);


struct time time = {0};
// 定时功能的队列
struct timer_list timer_list_head = {{0}, 0};

// 全局软中断状态，64 位，某一位为 1，表示存在某一类的软中断未处理
unsigned long softirq_status = 0;

struct softirq softirq_vector[64] = {0};

// 记录定时器产生的中断次数，也就是系统时间计数值
unsigned long volatile jiffies = 0;



struct Global_Memory_Descriptor memory_management_struct = {{0},0};

struct file *g_file_struct[TASK_FILE_MAX] = {0};
//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////
int get_cmos_time(struct time *time)
{
    // cli();
    do
    {   time->year =   CMOS_READ(0x09) + CMOS_READ(0x32) * 0x100;
        time->month =  CMOS_READ(0x08);
        time->day =    CMOS_READ(0x07);
        time->hour =   CMOS_READ(0x04);
        time->minute = CMOS_READ(0x02);
        time->second = CMOS_READ(0x00);
    }while(time->second != CMOS_READ(0x00));
    io_out8(0x70,0x00);
    // sti();
}

// ============== sofirq functions
/**
 * @brief 注册一个软中断
 *
 * @param nr 中断向量号
 * @param action 软中断处理函数
 * @param data 软中断处理函数的参数
 */
void register_softirq(int nr, void (*action)(void *data), void *data) {
  softirq_vector[nr].action = action;
  softirq_vector[nr].data = data;
}

/**
 * @brief 注销一个软中断
 *
 * @param nr 中断向量号
 */
void unregister_softirq(int nr) {
  softirq_vector[nr].action = NULL;
  softirq_vector[nr].data = NULL;
}

void set_softirq_status(unsigned long status) { softirq_status |= status; }
unsigned long get_softirq_status() { return softirq_status; }

// 初始化软中断
void softirq_init() {
  softirq_status = 0;
  memset(softirq_vector, 0, sizeof(struct softirq) * 64);
}

/**
 * @brief 处理系统中当前存在的所有软中断，下半部
 * 逐个检测中断状态变量softirq_status的各个位。
 * 如果有某位处于置位状态，系统则调用相应的处理方法，并在执行结束后，复位此状态位。
 */
void do_softirq() {
  int i;
  // 开中断
  sti();
  // 处理所有软中断
  while (softirq_status) {
    for (i = 0; i < 64; i++) {
      if (softirq_status & (1 << i)) {
        // 调用软中断处理函数
        softirq_vector[i].action(softirq_vector[i].data);
        softirq_status &= ~(1 << i); // 复位
      }
    }
  }

  cli();
}



// sofirq test timer
/*
void do_timer(void * data)
{
    color_printk(RED,WHITE,"(HPET:%ld)",jiffies);
}

void timer_init()
{
    jiffies = 0;
    register_softirq(0,&do_timer,NULL);
}
*/
// ==============

// ============== timer functions
void init_timer(struct timer_list * timer,void (* func)(void * data),void * data,unsigned long expect_jiffies) {
    list_init(&timer->list);
    timer->func = func;
    timer->data = data;
    timer->expect_jiffies = jiffies + expect_jiffies;
}

void add_timer(struct timer_list * timer)
{
    struct timer_list * tmp = container_of(list_next(&timer_list_head.list),struct timer_list,list);

    if(list_is_empty(&timer_list_head.list)){
    }
    else{
        while(tmp->expect_jiffies < timer->expect_jiffies)
            tmp = container_of(list_next(&tmp->list),struct timer_list,list);
    }
    list_add_to_behind(&tmp->list,&timer->list);
}

void del_timer(struct timer_list * timer)
{
    list_del(&timer->list);
}

void do_timer(void * data)
{
    struct timer_list * tmp = container_of(list_next(&timer_list_head.list),struct timer_list,list);
    while((!list_is_empty(&timer_list_head.list)) && (tmp->expect_jiffies <= jiffies))
    {
        del_timer(tmp);
        tmp->func(tmp->data);
        tmp = container_of(list_next(&timer_list_head.list),struct timer_list,list);
    }

    color_printk(RED,WHITE,"(HPET:%ld)", jiffies);
}

void test_timer(void * data)
{
    color_printk(BLUE,WHITE,"test_timer");
}

void timer_init()
{
    struct timer_list *tmp = NULL;
    jiffies = 0;
    init_timer(&timer_list_head,NULL,NULL,-1UL);
    register_softirq(0,&do_timer,NULL);

    tmp = (struct timer_list *)kmalloc(sizeof(struct timer_list),0);

    init_timer(tmp,&test_timer,NULL,5);
    add_timer(tmp);
}



// ==============
/**********
* global function
*/
void init_cpu(void)
{
	int i,j;
	unsigned int CpuFacName[4] = {0,0,0,0};
	char	FactoryName[17] = {0};

	color_printk(YELLOW, BLACK, "\n[+] init cpu, get cpu info...\n");
	//vendor_string
	get_cpuid(0,0,&CpuFacName[0],&CpuFacName[1],&CpuFacName[2],&CpuFacName[3]);

	*(unsigned int*)&FactoryName[0] = CpuFacName[1];

	*(unsigned int*)&FactoryName[4] = CpuFacName[3];

	*(unsigned int*)&FactoryName[8] = CpuFacName[2];	

	FactoryName[12] = '\0';
	color_printk(YELLOW,BLACK,"%s\t%#010x\t%#010x\t%#010x\n",FactoryName,CpuFacName[1],CpuFacName[3],CpuFacName[2]);
	
	//brand_string
	for(i = 0x80000002;i < 0x80000005;i++)
	{
		get_cpuid(i,0,&CpuFacName[0],&CpuFacName[1],&CpuFacName[2],&CpuFacName[3]);

		*(unsigned int*)&FactoryName[0] = CpuFacName[0];

		*(unsigned int*)&FactoryName[4] = CpuFacName[1];

		*(unsigned int*)&FactoryName[8] = CpuFacName[2];

		*(unsigned int*)&FactoryName[12] = CpuFacName[3];

		FactoryName[16] = '\0';
		color_printk(YELLOW,BLACK,"%s",FactoryName);
	}
	color_printk(YELLOW,BLACK,"\n");

	//Version Informatin Type,Family,Model,and Stepping ID
	get_cpuid(1,0,&CpuFacName[0],&CpuFacName[1],&CpuFacName[2],&CpuFacName[3]);
	color_printk(YELLOW,BLACK,"[+] Family Code:%#010x,Extended Family:%#010x,Model Number:%#010x,Extended Model:%#010x,Processor Type:%#010x,Stepping ID:%#010x\n",(CpuFacName[0] >> 8 & 0xf),(CpuFacName[0] >> 20 & 0xff),(CpuFacName[0] >> 4 & 0xf),(CpuFacName[0] >> 16 & 0xf),(CpuFacName[0] >> 12 & 0x3),(CpuFacName[0] & 0xf));

	//get Linear/Physical Address size
	get_cpuid(0x80000008,0,&CpuFacName[0],&CpuFacName[1],&CpuFacName[2],&CpuFacName[3]);
	color_printk(YELLOW,BLACK,"[+] Physical Address size:%08d,Linear Address size:%08d\n",(CpuFacName[0] & 0xff),(CpuFacName[0] >> 8 & 0xff));

	//max cpuid operation code
	get_cpuid(0,0,&CpuFacName[0],&CpuFacName[1],&CpuFacName[2],&CpuFacName[3]);
	color_printk(WHITE,BLACK,"[+] MAX Basic Operation Code :%#010x\t",CpuFacName[0]);

	get_cpuid(0x80000000,0,&CpuFacName[0],&CpuFacName[1],&CpuFacName[2],&CpuFacName[3]);
	color_printk(WHITE,BLACK,"[+] MAX Extended Operation Code :%#010x\n",CpuFacName[0]);


}

// ===========

/*
帧缓存区被映射的线性地址 ，此处是Oxffff800000a0000， 由于页表映射的关系(模式切换时的同一性地址映射)，帧缓存区地址空间也被映射到线性地址 0xa00000 处

// 后修改到 0x300000 处
*/
void show_color(void)
{
    // int *addr = (int *)0xffff800000a00000;
	int *addr = (int *)0xffff800003000000;
	int i;

    // 800*600
    int x = 800;
    int y = 600;

    x = 1440;
    y = 900;

	for(i = 0 ;i<x*20;i++)
	{
		*((char *)addr+0)=(char)0x00;
		*((char *)addr+1)=(char)0x00;
		*((char *)addr+2)=(char)0xff;
		*((char *)addr+3)=(char)0x00;	
		addr +=1;	
	}
	for(i = 0 ;i<x*20;i++)
	{
		*((char *)addr+0)=(char)0x00;
		*((char *)addr+1)=(char)0xff;
		*((char *)addr+2)=(char)0x00;
		*((char *)addr+3)=(char)0x00;	
		addr +=1;	
	}
	for(i = 0 ;i<x*20;i++)
	{
		*((char *)addr+0)=(char)0xff;
		*((char *)addr+1)=(char)0x00;
		*((char *)addr+2)=(char)0x00;
		*((char *)addr+3)=(char)0x00;	
		addr +=1;	
	}
	for(i = 0 ;i<x*20;i++)
	{
		*((char *)addr+0)=(char)0xff;
		*((char *)addr+1)=(char)0xff;
		*((char *)addr+2)=(char)0xff;
		*((char *)addr+3)=(char)0x00;	
		addr +=1;	
	}
}

void print_color_hello()
{
    int screen_x = 800;
    int screen_y = 600;

    screen_x = 1440;
    screen_y = 900;

    Pos.XResolution = screen_x;
	Pos.YResolution = screen_y;

	Pos.XPosition = 0;
	Pos.YPosition = 0;

	Pos.XCharSize = 8;
	Pos.YCharSize = 16;

	// Pos.FB_addr = (int *)0xffff800000a00000;
	Pos.FB_addr = (int *)0xffff800003000000;
	// Pos.FB_length = (Pos.XResolution * Pos.YResolution * 4 + PAGE_4K_SIZE - 1) & PAGE_4K_MASK;
    Pos.FB_length = (Pos.XResolution * Pos.YResolution * 4);


    color_printk (YELLOW, BLACK, "Hello\t\t World!\t\t \n \t\tFrom kernel start print color hello\n");
}

void test_memory()
{
    struct Page * page = NULL;
    int i;

    color_printk(RED,BLACK,"[+]memory_management_struct.bits_map:%#018lx\n",*memory_management_struct.bits_map);
	color_printk(RED,BLACK,"[+]memory_management_struct.bits_map:%#018lx\n",*(memory_management_struct.bits_map + 1));

	page = alloc_pages(ZONE_NORMAL,64,PG_PTable_Maped | PG_Active | PG_Kernel);

	for(i = 0;i <= 64;i++)
	{
		color_printk(INDIGO,BLACK,"[+]page%d\tattribute:%#018lx\taddress:%#018lx\t",i,(page + i)->attribute,(page + i)->PHY_address);
		i++;
		color_printk(INDIGO,BLACK,"[+]page%d\tattribute:%#018lx\taddress:%#018lx\n",i,(page + i)->attribute,(page + i)->PHY_address);
	}

	color_printk(RED,BLACK,"[+]memory_management_struct.bits_map:%#018lx\n",*memory_management_struct.bits_map);
	color_printk(RED,BLACK,"[+]memory_management_struct.bits_map:%#018lx\n",*(memory_management_struct.bits_map + 1));
    color_printk(RED,BLACK,"[+] memory test end\n");
}

void Start_Kernel(void)
{
	spin_init(&Pos.printk_lock);
    // show_color();
    print_color_hello();

    // =================================================

    // 1. upgrade TSS
    // load_TR(8);
	load_TR(10);

	set_tss64((unsigned int *)&init_tss[0],_stack_start, _stack_start, _stack_start, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

    // 2. init exception / interrupt vector
    sys_vector_init();

    // test except
/*
    int i;
    i = i / 0;
*/
	// init cpu
	init_cpu();

    // 3. init memory manage (upgrade memory paging)
    memory_management_struct.start_code = (unsigned long)& _text;
	memory_management_struct.end_code   = (unsigned long)& _etext;
	memory_management_struct.end_data   = (unsigned long)& _edata;
	memory_management_struct.end_brk    = (unsigned long)& _end;

    color_printk(RED,BLACK,"[+] memory init \n");
	init_memory();

    // test memory
    // test_memory();
	color_printk(RED,BLACK,"[+] slab init \n");
	slab_init();

	unsigned char *ptr = (unsigned char *)kmalloc(STACK_SIZE,0) + STACK_SIZE;
	((struct task_struct *)(ptr - STACK_SIZE))->cpu_id = 0;
		
	init_tss[0].ist1 = (unsigned long)ptr;
	init_tss[0].ist2 = (unsigned long)ptr;
	init_tss[0].ist3 = (unsigned long)ptr;
	init_tss[0].ist4 = (unsigned long)ptr;
	init_tss[0].ist5 = (unsigned long)ptr;
	init_tss[0].ist6 = (unsigned long)ptr;
	init_tss[0].ist7 = (unsigned long)ptr;

	color_printk(RED,BLACK,"[+] frame buffer init \n");
	frame_buffer_init();
	color_printk(WHITE,BLACK,"[+] frame_buffer_init() is OK \n");

	color_printk(RED,BLACK,"[+] pagetable init \n");	
	pagetable_init();

    // 4. implment IRQ
    color_printk(RED,BLACK,"[+] interrupt init \n");
    // init_interrupt();
#if APIC
	APIC_IOAPIC_init();
#else
	init_8259A();
#endif

	schedule_init();
	softirq_init();
	// 4.1 keyboard diver
	color_printk(RED,BLACK,"keyboard init \n");
	keyboard_init();

	// 4.2 mouse driver
	color_printk(RED,BLACK,"mouse init \n");
	mouse_init();

	// 4.3 disk driver
	color_printk(RED,BLACK,"disk init \n");
	disk_init();

	// 4.4 driver test
/*
	color_printk(PURPLE,BLACK,"[+]disk test:...................................\n");
	// =============== 4.4.1 disk write test based on block device driver model
	color_printk(PURPLE,BLACK,"disk write:\n");
	memset(buf,0x44,512);
	// IDE_device_operation.transfer(ATA_WRITE_CMD,0x12345678,1,(unsigned char *)buf);
	IDE_device_operation.transfer(ATA_WRITE_CMD, 0x12, 1, (unsigned char *)buf);

	color_printk(PURPLE,BLACK,"disk write end\n");

	// =============== 4.4.2 disk write test based on block device driver model
	color_printk(PURPLE,BLACK,"disk read:\n");
	memset(buf,0x00,512);
	// IDE_device_operation.transfer(ATA_READ_CMD,0x12345678,1,(unsigned char *)buf);
	IDE_device_operation.transfer(ATA_READ_CMD, 0x12, 1, (unsigned char *)buf);
	
	int i = 0;
	for(i;i < 512 ; i++)
		color_printk(BLACK,WHITE,"%02x",buf[i]);
	color_printk(PURPLE,BLACK,"\ndisk read end\n");
*/
	// 4.5 SMP implement based on Local APIC
	color_printk(RED,YELLOW,"\n[+]BSP smp init\n");
	SMP_init();
#if DEBUG
/*
	// for SMP #DV exception
	while(1)
		hlt();
*/
#endif
	color_printk(RED,BLACK,"Timer & Clock init \n");
	timer_init();
	// get_cmos_time(&time);
	// color_printk(BLACK, WHITE, "year:%#010x,month:%#010x,day:%#010x,hour:%#010x,mintue:%#010x,second:%#010x\n",time.year,time.month,time.day,time.hour,time.minute, time.second);
	color_printk(RED, YELLOW, "\n%s ===========[+]Get COMS time...HPET init...........\n", __func__);
	HPET_init();
	sti();
    // 5. multi task
    color_printk(RED,BLACK,"[+] task_init \n");
	task_init();

	while(1){
#if APIC
		if(p_kb->count)
			analysis_keycode();
		if(p_mouse->count)
			analysis_mousecode();
#endif
		
	}
}
