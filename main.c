;/***************************************************
;
; q2os kernel main
;
; gcc -mcmodel=large -fno-builtin -m64 -c main.c
;***************************************************/

#include "lib.h"
#include "printk.h"
#include "gate.h"
#include "trap.h"
#include "memory.h"
#include "interrupt.h"
#include "task.h"

/*************
    static var 
*/


struct Global_Memory_Descriptor memory_management_struct = {{0},0};


/**********
* global function
*/


// ===========

/*
帧缓存区被映射的线性地址 ，此处是Oxffff800000a0000， 由于页表映射的关系(模式切换时的同一性地址映射)，帧缓存区地址空间也被映射到线性地址 0xa00000 处

*/
void show_color(void)
{
    int *addr = (int *)0xffff800000a00000;
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

	Pos.FB_addr = (int *)0xffff800000a00000;
	// Pos.FB_length = (Pos.XResolution * Pos.YResolution * 4 + PAGE_4K_SIZE - 1) & PAGE_4K_MASK;
    Pos.FB_length = (Pos.XResolution * Pos.YResolution * 4);


    color_printk (YELLOW, BLACK, "Hello\t\t World! \n");
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
    // show_color();
    print_color_hello();

    // =================================================

    // 1. upgrade TSS
    // load_TR(8);
	load_TR(10);

	// set_tss64(_stack_start, _stack_start, _stack_start, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);
    set_tss64(0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

    // 2. init exception / interrupt vector
    sys_vector_init();

    // test except
/*
    int i;
    i = i / 0;
*/

    // 3. init memory manage (upgrade memory paging)
    memory_management_struct.start_code = (unsigned long)& _text;
	memory_management_struct.end_code   = (unsigned long)& _etext;
	memory_management_struct.end_data   = (unsigned long)& _edata;
	memory_management_struct.end_brk    = (unsigned long)& _end;

    color_printk(RED,BLACK,"[+] memory init \n");
	init_memory();

    // test memory
    // test_memory();

    // 4. implment IRQ
    color_printk(RED,BLACK,"[+] interrupt init \n");
    init_interrupt();

    // 5. multi task
    color_printk(RED,BLACK,"[+] task_init \n");
	task_init();

	while(1)
		;
}
