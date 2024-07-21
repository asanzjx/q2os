#ifndef __LIB_H__
#define __LIB_H__

// =============== Compile & Debug
#define DEBUG 1
// #define OPEN_IRQ_FORWARD	1
// #define CLEAN_SCREEN	1	// !!!be care for, cased panic without spin_lock irq manage; and too slow !!!

// --- TEST ---
// #define TEST_DISK1_FAT32	1
#define TEST_SYSCALL	1
// ------------

#if Bochs
#define BochsMagicBreakpoint()	\
	__asm__ __volatile__("xchgw %bx, %bx")

#define BochsConsolePrintChar(c) io_out8(0xe9, c)
// outputs a character to the debug console
/*
// #define BochsConsolePrintChar(c) outportb(0xe9, c);
#define BochsConsolePrintChar(c) io_out8(0xe9, c)

// print strs 
#define BochsConsolePrintStr(strs)	\
	int x = 0;	\
	char c;	\
	int str_lens = strlen(strs);	\
	for(x; x < str_lens; x++){	\
		c = strs[i];	\
		io_out8(0xe9, c);	\
	}
*/
#endif
// ===============

//////////////////////////////////////////////////
//					Constants					//
//////////////////////////////////////////////////
#define NULL 0
#define	TASK_SIZE	((unsigned long)0x00007fffffffffff)
#define TASK_FILE_MAX	10
#define container_of(ptr,type,member)							\
({											\
	typeof(((type *)0)->member) * p = (ptr);					\
	(type *)((unsigned long)p - (unsigned long)&(((type *)0)->member));		\
})

#define hlt() 		__asm__ __volatile__ ("hlt	\n\t")
#define pause() 	__asm__ __volatile__ ("pause	\n\t")

#define sti() 		__asm__ __volatile__ ("sti	\n\t":::"memory")
#define cli()	 	__asm__ __volatile__ ("cli	\n\t":::"memory")
#define nop() 		__asm__ __volatile__ ("nop	\n\t")
#define io_mfence() 	__asm__ __volatile__ ("mfence	\n\t":::"memory")
#define local_irq_disable()	cli();
#define local_irq_enable()	sti();
#define local_irq_save(x)	__asm__ __volatile__("pushfq ; popq %0 ; cli":"=g"(x)::"memory")
#define local_irq_restore(x)	__asm__ __volatile__("pushq %0 ; popfq"::"g"(x):"memory")

// === spin lock with irq manage
typedef struct
{
	__volatile__ unsigned long lock;		//1:unlock,0:lock
}spinlock_T;

extern inline void spin_lock(spinlock_T * lock);
extern inline void spin_unlock(spinlock_T * lock);
extern inline long spin_trylock(spinlock_T * lock);

#define spin_lock_irqsave(lock,flags)	\
do					\
{					\
	local_irq_save(flags);		\
	spin_lock(lock);		\
}while(0)

#define spin_unlock_irqrestore(lock,flags)	\
do						\
{						\
	spin_unlock(lock);			\
	local_irq_restore(flags);		\
}while(0)

#define spin_lock_irqsave(lock,flags)	\
do					\
{					\
	local_irq_save(flags);		\
	spin_lock(lock);		\
}while(0)

#define spin_unlock_irqrestore(lock,flags)	\
do						\
{						\
	spin_unlock(lock);			\
	local_irq_restore(flags);		\
}while(0)


// ===


#define NR_CPUS 8

struct List
{
	struct List * prev;
	struct List * next;
};

struct time
{
    int second;    //00
    int minute;    //02
    int hour;      //04
    int day;       //07
    int month;     //08
    int year;      //09+32
};

struct timer_list
{
    struct List list;
    unsigned long expect_jiffies;
    void (* func)(void * data);
    void *data;
};

enum
{
    HI_SOFTIRQ = 0,  /* 优先级高的 tasklet */
    TIMER_SOFTIRQ,   /* 定时器的下半部 */
    TASKLET_SOFTIRQ, /* 正常优先权的 tasklet */
};

struct softirq
{
	void (*action)(void * data);
	void * data;
};



struct pt_regs
{
	unsigned long r15;
	unsigned long r14;
	unsigned long r13;
	unsigned long r12;
	unsigned long r11;
	unsigned long r10;
	unsigned long r9;
	unsigned long r8;
	unsigned long rbx;
	unsigned long rcx;
	unsigned long rdx;
	unsigned long rsi;
	unsigned long rdi;
	unsigned long rbp;
	unsigned long ds;
	unsigned long es;
	unsigned long rax;
	unsigned long func;
	unsigned long errcode;
	unsigned long rip;
	unsigned long cs;
	unsigned long rflags;
	unsigned long rsp;
	unsigned long ss;
};

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

inline void list_init(struct List * list)
{
	list->prev = list;
	list->next = list;
}

inline void list_add_to_behind(struct List * entry,struct List * new)	////add to entry behind
{
	new->next = entry->next;
	new->prev = entry;
	new->next->prev = new;
	entry->next = new;
}

inline void list_add_to_before(struct List * entry,struct List * new)	////add to entry behind
{
	new->next = entry;
	entry->prev->next = new;
	new->prev = entry->prev;
	entry->prev = new;
}

inline void list_del(struct List * entry)
{
	entry->next->prev = entry->prev;
	entry->prev->next = entry->next;
}

inline long list_is_empty(struct List * entry)
{
	if(entry == entry->next && entry->prev == entry)
		return 1;
	else
		return 0;
}

inline struct List * list_prev(struct List * entry)
{
	if(entry->prev != NULL)
		return entry->prev;
	else
		return NULL;
}

inline struct List * list_next(struct List * entry)
{
	if(entry->next != NULL)
		return entry->next;
	else
		return NULL;
}

/*
		From => To memory copy Num bytes
*/

inline void * memcpy(void *From,void * To,long Num)
{
	int d0,d1,d2;
	__asm__ __volatile__	(	"cld	\n\t"
					"rep	\n\t"
					"movsq	\n\t"
					"testb	$4,%b4	\n\t"
					"je	1f	\n\t"
					"movsl	\n\t"
					"1:\ttestb	$2,%b4	\n\t"
					"je	2f	\n\t"
					"movsw	\n\t"
					"2:\ttestb	$1,%b4	\n\t"
					"je	3f	\n\t"
					"movsb	\n\t"
					"3:	\n\t"
					:"=&c"(d0),"=&D"(d1),"=&S"(d2)
					:"0"(Num/8),"q"(Num),"1"(To),"2"(From)
					:"memory"
				);
	return To;
}

/*
		FirstPart = SecondPart		=>	 0
		FirstPart > SecondPart		=>	 1
		FirstPart < SecondPart		=>	-1
*/

inline int memcmp(void * FirstPart,void * SecondPart,long Count)
{
	register int __res;

	__asm__	__volatile__	(	"cld	\n\t"		//clean direct
					"repe	\n\t"		//repeat if equal
					"cmpsb	\n\t"
					"je	1f	\n\t"
					"movl	$1,	%%eax	\n\t"
					"jl	1f	\n\t"
					"negl	%%eax	\n\t"
					"1:	\n\t"
					:"=a"(__res)
					:"0"(0),"D"(FirstPart),"S"(SecondPart),"c"(Count)
					:
				);
	return __res;
}

/*
		set memory at Address with C ,number is Count
*/

inline void * memset(void * Address,unsigned char C,long Count)
{
	int d0,d1;
	unsigned long tmp = C * 0x0101010101010101UL;
	__asm__	__volatile__	(	"cld	\n\t"
					"rep	\n\t"
					"stosq	\n\t"
					"testb	$4, %b3	\n\t"
					"je	1f	\n\t"
					"stosl	\n\t"
					"1:\ttestb	$2, %b3	\n\t"
					"je	2f\n\t"
					"stosw	\n\t"
					"2:\ttestb	$1, %b3	\n\t"
					"je	3f	\n\t"
					"stosb	\n\t"
					"3:	\n\t"
					:"=&c"(d0),"=&D"(d1)
					:"a"(tmp),"q"(Count),"0"(Count/8),"1"(Address)	
					:"memory"					
				);
	return Address;
}

/*
		string copy
*/

inline char * strcpy(char * Dest,char * Src)
{
	__asm__	__volatile__	(	"cld	\n\t"
					"1:	\n\t"
					"lodsb	\n\t"
					"stosb	\n\t"
					"testb	%%al,	%%al	\n\t"
					"jne	1b	\n\t"
					:
					:"S"(Src),"D"(Dest)
					:
					
				);
	return 	Dest;
}

/*
		string copy number bytes
*/

inline char * strncpy(char * Dest,char * Src,long Count)
{
	__asm__	__volatile__	(	"cld	\n\t"
					"1:	\n\t"
					"decq	%2	\n\t"
					"js	2f	\n\t"
					"lodsb	\n\t"
					"stosb	\n\t"
					"testb	%%al,	%%al	\n\t"
					"jne	1b	\n\t"
					"rep	\n\t"
					"stosb	\n\t"
					"2:	\n\t"
					:
					:"S"(Src),"D"(Dest),"c"(Count)
					:					
				);
	return Dest;
}

/*
		string cat Dest + Src
*/

inline char * strcat(char * Dest,char * Src)
{
	__asm__	__volatile__	(	"cld	\n\t"
					"repne	\n\t"
					"scasb	\n\t"
					"decq	%1	\n\t"
					"1:	\n\t"
					"lodsb	\n\t"
					"stosb	\n\r"
					"testb	%%al,	%%al	\n\t"
					"jne	1b	\n\t"
					:
					:"S"(Src),"D"(Dest),"a"(0),"c"(0xffffffff)
					:					
				);
	return Dest;
}

/*
		string compare FirstPart and SecondPart
		FirstPart = SecondPart =>  0
		FirstPart > SecondPart =>  1
		FirstPart < SecondPart => -1
*/

inline int strcmp(char * FirstPart,char * SecondPart)
{
	register int __res;
	__asm__	__volatile__	(	"cld	\n\t"
					"1:	\n\t"
					"lodsb	\n\t"
					"scasb	\n\t"
					"jne	2f	\n\t"
					"testb	%%al,	%%al	\n\t"
					"jne	1b	\n\t"
					"xorl	%%eax,	%%eax	\n\t"
					"jmp	3f	\n\t"
					"2:	\n\t"
					"movl	$1,	%%eax	\n\t"
					"jl	3f	\n\t"
					"negl	%%eax	\n\t"
					"3:	\n\t"
					:"=a"(__res)
					:"D"(FirstPart),"S"(SecondPart)
					:					
				);
	return __res;
}

/*
		string compare FirstPart and SecondPart with Count Bytes
		FirstPart = SecondPart =>  0
		FirstPart > SecondPart =>  1
		FirstPart < SecondPart => -1
*/

inline int strncmp(char * FirstPart,char * SecondPart,long Count)
{	
	register int __res;
	__asm__	__volatile__	(	"cld	\n\t"
					"1:	\n\t"
					"decq	%3	\n\t"
					"js	2f	\n\t"
					"lodsb	\n\t"
					"scasb	\n\t"
					"jne	3f	\n\t"
					"testb	%%al,	%%al	\n\t"
					"jne	1b	\n\t"
					"2:	\n\t"
					"xorl	%%eax,	%%eax	\n\t"
					"jmp	4f	\n\t"
					"3:	\n\t"
					"movl	$1,	%%eax	\n\t"
					"jl	4f	\n\t"
					"negl	%%eax	\n\t"
					"4:	\n\t"
					:"=a"(__res)
					:"D"(FirstPart),"S"(SecondPart),"c"(Count)
					:
				);
	return __res;
}

/*

*/

inline int strlen(char * String)
{
	register int __res;
	__asm__	__volatile__	(	"cld	\n\t"
					"repne	\n\t"
					"scasb	\n\t"
					"notl	%0	\n\t"
					"decl	%0	\n\t"
					:"=c"(__res)
					:"D"(String),"a"(0),"0"(0xffffffff)
					:
				);
	return __res;
}

/*

*/

inline unsigned long bit_set(unsigned long * addr,unsigned long nr)
{
	return *addr | (1UL << nr);
}

/*

*/

inline unsigned long bit_get(unsigned long * addr,unsigned long nr)
{
	return	*addr & (1UL << nr);
}

/*

*/

inline unsigned long bit_clean(unsigned long * addr,unsigned long nr)
{
	return	*addr & (~(1UL << nr));
}

/*

*/

inline unsigned char io_in8(unsigned short port)
{
	unsigned char ret = 0;
	__asm__ __volatile__(	"inb	%%dx,	%0	\n\t"
				"mfence			\n\t"
				:"=a"(ret)
				:"d"(port)
				:"memory");
	return ret;
}

/*

*/

inline unsigned int io_in32(unsigned short port)
{
	unsigned int ret = 0;
	__asm__ __volatile__(	"inl	%%dx,	%0	\n\t"
				"mfence			\n\t"
				:"=a"(ret)
				:"d"(port)
				:"memory");
	return ret;
}

/*

*/

inline void io_out8(unsigned short port,unsigned char value)
{
	__asm__ __volatile__(	"outb	%0,	%%dx	\n\t"
				"mfence			\n\t"
				:
				:"a"(value),"d"(port)
				:"memory");
}

/*

*/

inline void io_out32(unsigned short port,unsigned int value)
{
	__asm__ __volatile__(	"outl	%0,	%%dx	\n\t"
				"mfence			\n\t"
				:
				:"a"(value),"d"(port)
				:"memory");
}

/*

*/

#define port_insw(port,buffer,nr)	\
__asm__ __volatile__("cld;rep;insw;mfence;"::"d"(port),"D"(buffer),"c"(nr):"memory")

#define port_outsw(port,buffer,nr)	\
__asm__ __volatile__("cld;rep;outsw;mfence;"::"d"(port),"S"(buffer),"c"(nr):"memory")

inline unsigned long rdmsr(unsigned long address)
{
	unsigned int tmp0 = 0;
	unsigned int tmp1 = 0;
	__asm__ __volatile__("rdmsr	\n\t":"=d"(tmp0),"=a"(tmp1):"c"(address):"memory");	
	return (unsigned long)tmp0<<32 | tmp1;
}

inline void wrmsr(unsigned long address,unsigned long value)
{
	// __asm__ __volatile__("wrmsr	\n\t"::"d"(value >> 32),"a"(value & 0xffffffff),"c"(address):"memory");	
	__asm__ __volatile__("wrmsr	\n\t"::"d"(value >> 32),"a"(value & 0xffffffff),"c"(address));
}

inline void get_cpuid(unsigned int Mop,unsigned int Sop,unsigned int * a,unsigned int * b,unsigned int * c,unsigned int * d)
{
	__asm__ __volatile__	(	"cpuid	\n\t"
					:"=a"(*a),"=b"(*b),"=c"(*c),"=d"(*d)
					:"0"(Mop),"2"(Sop)
				);
}

inline unsigned long get_rsp()
{
	unsigned long tmp = 0;
	__asm__ __volatile__	( "movq	%%rsp, %0	\n\t":"=r"(tmp)::"memory");
	return tmp;
}

inline unsigned long get_rflags()
{
	unsigned long tmp = 0;
	__asm__ __volatile__	("pushfq	\n\t"
				 "movq	(%%rsp), %0	\n\t"
				 "popfq	\n\t"
				:"=r"(tmp)::"memory");
	return tmp;
}


// ========================= atomic
typedef struct
{
    __volatile__ long value;
} atomic_T;

inline void atomic_add(atomic_T * atomic,long value)
{
    __asm__ __volatile__    (    "lock    addq    %1,    %0    \n\t"
                                 :"=m"(atomic->value):"r"(value):"memory"
                            );
}
inline void atomic_sub(atomic_T *atomic,long value)
{
    __asm__ __volatile__    (    "lock    subq    %1,    %0    \n\t"
                                 :"=m"(atomic->value):"r"(value):"memory"
                            );
}
inline void atomic_inc(atomic_T *atomic)
{
    __asm__ __volatile__    (    "lock    incq    %0    \n\t"
                                 :"=m"(atomic->value):"m"(atomic->value):"memory"
                            );
}
inline void atomic_dec(atomic_T *atomic)
{
    __asm__ __volatile__    (    "lock    decq    %0    \n\t"
                                 :"=m"(atomic->value):"m"(atomic->value):"memory"
                            );
}

#define atomic_read(atomic)	((atomic)->value)
#define atomic_set(atomic,val)	(((atomic)->value) = (val))
// =========================

// ========================= for user application
inline long verify_area(unsigned char* addr,unsigned long size)
{
	if(((unsigned long)addr + size) <= (unsigned long)0x00007fffffffffff )
		return 1;
	else
		return 0;
}

inline long copy_from_user(void * from,void * to,unsigned long size)
{
	unsigned long d0,d1;
	if(!verify_area(from,size))
		return 0;
	__asm__ __volatile__	(	"rep	\n\t"
					 "movsq	\n\t"
					 "movq	%3,	%0	\n\t"
					 "rep	\n\t"
					 "movsb	\n\t"
					:"=&c"(size),"=&D"(d0),"=&S"(d1)
					:"r"(size & 7),"0"(size / 8),"1"(to),"2"(from)
					:"memory"
				);
	return size;
}

inline long copy_to_user(void * from,void * to,unsigned long size)
{
	unsigned long d0,d1;
	if(!verify_area(to,size))
		return 0;
	__asm__ __volatile__	(	"rep	\n\t"
				 	"movsq	\n\t"
				 	"movq	%3,	%0	\n\t"
					 "rep	\n\t"
					 "movsb	\n\t"
					:"=&c"(size),"=&D"(d0),"=&S"(d1)
					:"r"(size & 7),"0"(size / 8),"1"(to),"2"(from)
					:"memory"
				);
	return size;
}

inline long strncpy_from_user(void * from,void * to,unsigned long size)
{
	if(!verify_area(from,size))
		return 0;

	strncpy(to,from,size);
	return	size;
}

inline long strnlen_user(void * src,unsigned long maxlen)
{
	unsigned long size = strlen(src);
	if(!verify_area(src,size))
		return 0;

	return size <= maxlen ? size : maxlen;
}
// =========================

#endif
