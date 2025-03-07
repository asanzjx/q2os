/***************************************************
*
***************************************************/

#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__


#include "lib.h"
#include "linkage.h"


/*

*/

/*
typedef struct
{
	__volatile__ unsigned long lock;		//1:unlock,0:lock
}spinlock_T;
*/
/*

*/

inline void spin_init(spinlock_T * lock)
{
	lock->lock = 1;
}

/*
// move to task.h
inline void spin_lock(spinlock_T * lock)
{
	preempt_disable();
	__asm__	__volatile__	(	"1:	\n\t"
					"lock	decq	%0	\n\t"
					"jns	3f	\n\t"
					"2:	\n\t"
					"pause	\n\t"
					"cmpq	$0,	%0	\n\t"
					"jle	2b	\n\t"
					"jmp	1b	\n\t"
					"3:	\n\t"
					:"=m"(lock->lock)
					:
					:"memory"
				);
}



inline void spin_unlock(spinlock_T * lock)
{
	__asm__	__volatile__	(	"movq	$1,	%0	\n\t"
					:"=m"(lock->lock)
					:
					:"memory"
				);
	preempt_enable();
}

inline long spin_trylock(spinlock_T * lock)
{
	unsigned long tmp_value = 0;
	preempt_disable();
	__asm__	__volatile__	(	"xchgq	%0,	%1	\n\t"
				:"=q"(tmp_value),"=m"(lock->lock)
				:"0"(0)
				:"memory"
			);
	if(!tmp_value)
		preempt_enable();
	return tmp_value;
}
*/
#endif
