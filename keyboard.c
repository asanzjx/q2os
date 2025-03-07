/***************************************************
*
*
***************************************************/

#include "keyboard.h"
#include "lib.h"
#include "interrupt.h"
#include "APIC.h"
#include "memory.h"
#include "printk.h"
#include "semaphore.h"
#include "VFS.h"

/*

*/

struct keyboard_inputbuffer * p_kb = NULL;

wait_queue_T keyboard_wait_queue;
// static int shift_l,shift_r,ctrl_l,ctrl_r,alt_l,alt_r;

void keyboard_handler(unsigned long nr, unsigned long parameter, struct pt_regs * regs)
{
	unsigned char x;
	x = io_in8(0x60);
//	color_printk(WHITE,BLACK,"(K:%02x)",x);

	if(p_kb->p_head == p_kb->buf + KB_BUF_SIZE)
		p_kb->p_head = p_kb->buf;

	*p_kb->p_head = x;
	p_kb->count++;
	p_kb->p_head ++;

	wakeup(&keyboard_wait_queue,TASK_UNINTERRUPTIBLE);		
}

/*

*/
/*
unsigned char get_scancode()
{
	unsigned char ret  = 0;

	if(p_kb->count == 0)
		while(!p_kb->count)
			nop();
	
	if(p_kb->p_tail == p_kb->buf + KB_BUF_SIZE)	
		p_kb->p_tail = p_kb->buf;

	ret = *p_kb->p_tail;
	p_kb->count--;
	p_kb->p_tail++;

	return ret;
}

void analysis_keycode()
{
	unsigned char x = 0;
	int i;	
	int key = 0;	
	int make = 0;

	x = get_scancode();
	
	if(x == 0xE1)	//pause break;
	{
		key = PAUSEBREAK;
		for(i = 1;i<6;i++)
			if(get_scancode() != pausebreak_scode[i])
			{
				key = 0;
				break;
			}
	}	
	else if(x == 0xE0) //print screen
	{
		x = get_scancode();

		switch(x)
		{
			case 0x2A: // press printscreen
		
				if(get_scancode() == 0xE0)
					if(get_scancode() == 0x37)
					{
						key = PRINTSCREEN;
						make = 1;
					}
				break;

			case 0xB7: // UNpress printscreen
		
				if(get_scancode() == 0xE0)
					if(get_scancode() == 0xAA)
					{
						key = PRINTSCREEN;
						make = 0;
					}
				break;

			case 0x1d: // press right ctrl
		
				ctrl_r = 1;
				key = OTHERKEY;
				break;

			case 0x9d: // UNpress right ctrl
		
				ctrl_r = 0;
				key = OTHERKEY;
				break;
			
			case 0x38: // press right alt
		
				alt_r = 1;
				key = OTHERKEY;
				break;

			case 0xb8: // UNpress right alt
		
				alt_r = 0;
				key = OTHERKEY;
				break;		

			default:
				key = OTHERKEY;
				break;
		}
		
	}
	
	if(key == 0)
	{
		unsigned int * keyrow = NULL;
		int column = 0;

		make = (x & FLAG_BREAK ? 0:1);

		keyrow = &keycode_map_normal[(x & 0x7F) * MAP_COLS];

		if(shift_l || shift_r)
			column = 1;

		key = keyrow[column];
		
		switch(x & 0x7F)
		{
			case 0x2a:	//SHIFT_L:
				shift_l = make;
				key = 0;

				// xxx test for open keyboard scan
				// io_out8(0x60, 0xFF);
				break;

			case 0x36:	//SHIFT_R:
				shift_r = make;
				key = 0;

				// test for close keyboard scan
				// io_out8(0x60, 0xF5);
				break;

			case 0x1d:	//CTRL_L:
				ctrl_l = make;
				key = 0;

				// test for restart system by intel 8042 control io 0x64
				// io_out8(0x64, 0xFE);
				break;

			case 0x38:	//ALT_L:
				alt_l = make;
				key = 0;
				break;

			default:
				if(!make)
					key = 0;
				break;
		}			

		if(key)
			color_printk(RED,YELLOW,"(K:%c)\t",key);

	}
}
*/

hw_int_controller keyboard_int_controller = 
{
	.enable = IOAPIC_enable,
	.disable = IOAPIC_disable,
	.install = IOAPIC_install,
	.uninstall = IOAPIC_uninstall,
	.ack = IOAPIC_edge_ack,
};

/*

*/

long keyboard_open(struct index_node * inode,struct file * filp)
{
	filp->private_data = p_kb;

	p_kb->p_head = p_kb->buf;
	p_kb->p_tail = p_kb->buf;
	p_kb->count  = 0;
	memset(p_kb->buf,0,KB_BUF_SIZE);

	return 1;
}

long keyboard_close(struct index_node * inode,struct file * filp)
{
	filp->private_data = NULL;

	p_kb->p_head = p_kb->buf;
	p_kb->p_tail = p_kb->buf;
	p_kb->count  = 0;
	memset(p_kb->buf,0,KB_BUF_SIZE);

	return 1;
}

#define	KEY_CMD_RESET_BUFFER	0

long keyboard_ioctl(struct index_node * inode,struct file * filp,unsigned long cmd,unsigned long arg)
{
	switch(cmd)
	{

		case KEY_CMD_RESET_BUFFER:
			p_kb->p_head = p_kb->buf;
			p_kb->p_tail = p_kb->buf;
			p_kb->count  = 0;
			memset(p_kb->buf,0,KB_BUF_SIZE);
		break;

		default:
		break;
	}

	return 0;
}

long keyboard_read(struct file * filp,char * buf,unsigned long count,long * position)
{
	long counter  = 0;
	unsigned char * tail = NULL;

#if DEBUG
	// test if entry user main
	// color_printk(RED, YELLOW, "\n%s[debug] call keyboard read\n", __func__);
	// BochsMagicBreakpoint();
#endif

	if(p_kb->count == 0)
		sleep_on(&keyboard_wait_queue);

	counter = p_kb->count >= count? count:p_kb->count;
	tail = p_kb->p_tail;
	
	if(counter <= (p_kb->buf + KB_BUF_SIZE - tail))
	{
		copy_to_user(tail,buf,counter);
		p_kb->p_tail += counter;
	}
	else
	{
		copy_to_user(tail,buf,(p_kb->buf + KB_BUF_SIZE - tail));
		copy_to_user(p_kb->p_head,buf,counter - (p_kb->buf + KB_BUF_SIZE - tail));
		p_kb->p_tail = p_kb->p_head + (counter - (p_kb->buf + KB_BUF_SIZE - tail));
	}
	p_kb->count -= counter;

	return counter;	
}

long keyboard_write(struct file * filp,char * buf,unsigned long count,long * position)
{
	return 0;
}


struct file_operations keyboard_fops = 
{
	.open = keyboard_open,
	.close = keyboard_close,
	.ioctl = keyboard_ioctl,
	.read = keyboard_read,
	.write = keyboard_write,
};

/*

*/


void keyboard_init()
{
	struct IO_APIC_RET_entry entry;
	unsigned long i,j;

	wait_queue_init(&keyboard_wait_queue,NULL);

	p_kb = (struct keyboard_inputbuffer *)kmalloc(sizeof(struct keyboard_inputbuffer),0);
	
	p_kb->p_head = p_kb->buf;
	p_kb->p_tail = p_kb->buf;
	p_kb->count  = 0;
	memset(p_kb->buf,0,KB_BUF_SIZE);

	entry.vector = 0x21;
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

	wait_KB_write();
	io_out8(PORT_KB_CMD,KBCMD_WRITE_CMD);
	wait_KB_write();
	io_out8(PORT_KB_DATA,KB_INIT_MODE);

	for(i = 0;i<1000;i++)
		for(j = 0;j<1000;j++)
			nop();

/*	
	shift_l = 0;
	shift_r = 0;
	ctrl_l  = 0;
	ctrl_r  = 0;
	alt_l   = 0;
	alt_r   = 0;
*/
	register_irq(0x21, &entry , &keyboard_handler, (unsigned long)p_kb, &keyboard_int_controller, "ps/2 keyboard");
}

/*

*/

void keyboard_exit()
{
	unregister_irq(0x21);
	kfree((unsigned long *)p_kb);
}
