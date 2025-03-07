/***************************************************
*
*
***************************************************/

#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__


#include "lib.h"
#include "task.h"
#include "schedule.h"



typedef struct 
{
	atomic_T counter;
	wait_queue_T wait;
} semaphore_T;


//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////
void wait_queue_init(wait_queue_T * wait_queue,struct task_struct *tsk)
{
	list_init(&wait_queue->wait_list);
	wait_queue->tsk = tsk;
}

void sleep_on(wait_queue_T * wait_queue_head)
{
	wait_queue_T wait;
	wait_queue_init(&wait,current);
	current->state = TASK_UNINTERRUPTIBLE;
	list_add_to_before(&wait_queue_head->wait_list,&wait.wait_list);

	schedule();
}

void interruptible_sleep_on(wait_queue_T *wait_queue_head)
{
	wait_queue_T wait;
	wait_queue_init(&wait,current);
	current->state = TASK_INTERRUPTIBLE;
	list_add_to_before(&wait_queue_head->wait_list,&wait.wait_list);

	schedule();
}

inline void wakeup_process(struct task_struct *tsk)
{
	tsk->state = TASK_RUNNING;
	insert_task_queue(tsk);
	current->flags |= NEED_SCHEDULE;
}

void wakeup(wait_queue_T * wait_queue_head,long state)
{
	wait_queue_T * wait = NULL;

	if(list_is_empty(&wait_queue_head->wait_list))
		return;

	wait = container_of(list_next(&wait_queue_head->wait_list),wait_queue_T,wait_list);

	if(wait->tsk->state & state)
	{
		list_del(&wait->wait_list);
		wakeup_process(wait->tsk);
	}
}

// ================ semaphore
void semaphore_init(semaphore_T * semaphore,unsigned long count)
{
	atomic_set(&semaphore->counter,count);
	wait_queue_init(&semaphore->wait,NULL);
}

void __up(semaphore_T * semaphore)
{
	wait_queue_T * wait = container_of(list_next(&semaphore->wait.wait_list),wait_queue_T,wait_list);

	list_del(&wait->wait_list);
	wait->tsk->state = TASK_RUNNING;
	insert_task_queue(wait->tsk);
	current->flags |= NEED_SCHEDULE;
}

void semaphore_up(semaphore_T * semaphore)
{
	if(list_is_empty(&semaphore->wait.wait_list))
		atomic_inc(&semaphore->counter);
	else
		__up(semaphore);
}

void __down(semaphore_T * semaphore)
{
	wait_queue_T wait;
	wait_queue_init(&wait,current);
	current->state = TASK_UNINTERRUPTIBLE;
	list_add_to_before(&semaphore->wait.wait_list,&wait.wait_list);

	schedule();
}

void semaphore_down(semaphore_T * semaphore)
{
	if(atomic_read(&semaphore->counter) > 0)
		atomic_dec(&semaphore->counter);
	else
		__down(semaphore);
}

#endif
