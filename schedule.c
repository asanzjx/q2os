/***************************************************
*
*
***************************************************/


#include "lib.h"
#include "printk.h"
#include "task.h"
#include "schedule.h"

extern unsigned long volatile jiffies;

struct task_struct *get_next_task()
{
	struct task_struct * tsk = NULL;

	if(list_is_empty(&task_schedule.task_queue.list))
	{
		return &init_task_union.task;
	}

	tsk = container_of(list_next(&task_schedule.task_queue.list),struct task_struct,list);
	list_del(&tsk->list);

	task_schedule.running_task_count -= 1;

	return tsk;
}

void insert_task_queue(struct task_struct *tsk)
{
	struct task_struct *tmp = container_of(list_next(&task_schedule.task_queue.list),struct task_struct,list);

	if(tsk == &init_task_union.task)
		return ;

	if(list_is_empty(&task_schedule.task_queue.list))
	{
	}
	else
	{
		while(tmp->vrun_time < tsk->vrun_time)
			tmp = container_of(list_next(&tmp->list),struct task_struct,list);
	}

	list_add_to_before(&tmp->list,&tsk->list);
	task_schedule.running_task_count += 1;
}

void schedule()
{
	struct task_struct *tsk = NULL;

	current->flags &= ~NEED_SCHEDULE;
	tsk = get_next_task();

	#if DEBUG
// /*
		// struct task_struct *tsk = NULL;
		// tsk = get_next_task();
		color_printk(RED,BLACK,"[+]%s #cur 0x%p, list prev->0x%p, next->0x%p, pid: %d, vrun_time:%d; \
		\n\t next task :0x%p, list prev->0x%p, next->0x%p, pid:%d, vrun_time:%d;	\
		\n\t task_schedule: 0x%p, CPU_exec_task_jiffies: %d, task_queue: 0x%p\n", \
		__func__, current, current->list.prev, current->list.next, current->pid, current->vrun_time, \
		tsk, tsk->list.prev, tsk->list.next, tsk->pid, tsk->vrun_time,	\
		&task_schedule, task_schedule.CPU_exec_task_jiffies, task_schedule.task_queue);
		BochsMagicBreakpoint();
// */
	#endif

	color_printk(RED,BLACK,"#schedule:%d#\n",jiffies);
	
	if(current->vrun_time >= tsk->vrun_time)
	{
		if(current->state == TASK_RUNNING)
			insert_task_queue(current);
			
		if(!task_schedule.CPU_exec_task_jiffies)
			switch(tsk->priority)
			{
				case 0:
				case 1:
					task_schedule.CPU_exec_task_jiffies = 4/task_schedule.running_task_count;
					break;
				case 2:
				default:
					task_schedule.CPU_exec_task_jiffies = 4/task_schedule.running_task_count*3;
					break;
			}
		
		switch_to(current,tsk);	
	}
	else
	{
		insert_task_queue(tsk);
		
		if(!task_schedule.CPU_exec_task_jiffies)
			switch(tsk->priority)
			{
				case 0:
				case 1:
					task_schedule.CPU_exec_task_jiffies = 4/task_schedule.running_task_count;
					break;
				case 2:
				default:
					task_schedule.CPU_exec_task_jiffies = 4/task_schedule.running_task_count*3;
					break;
			}
	}
}

void schedule_init()
{
	memset(&task_schedule,0,sizeof(struct schedule));

	list_init(&task_schedule.task_queue.list);
	task_schedule.task_queue.vrun_time = 0x7fffffffffffffff;

	task_schedule.running_task_count = 1;
	task_schedule.CPU_exec_task_jiffies = 4;
}


