/***************************************************
*
*
***************************************************/


#include "lib.h"
#include "printk.h"
#include "task.h"
#include "schedule.h"
#include "SMP.h"

extern unsigned long volatile jiffies;
struct schedule task_schedule[NR_CPUS];

struct task_struct *get_next_task()
{
	struct task_struct * tsk = NULL;

	if(list_is_empty(&task_schedule[SMP_cpu_id()].task_queue.list))
	{
		return init_task[SMP_cpu_id()];
	}

	tsk = container_of(list_next(&task_schedule[SMP_cpu_id()].task_queue.list),struct task_struct,list);
	list_del(&tsk->list);

	task_schedule[SMP_cpu_id()].running_task_count -= 1;

	return tsk;
}

void insert_task_queue(struct task_struct *tsk)
{
	struct task_struct *tmp = NULL;

	if(tsk == init_task[SMP_cpu_id()])
		return ;

	tmp = container_of(list_next(&task_schedule[SMP_cpu_id()].task_queue.list),struct task_struct,list);

	if(list_is_empty(&task_schedule[SMP_cpu_id()].task_queue.list))
	{
	}
	else
	{
		while(tmp->vrun_time < tsk->vrun_time)
			tmp = container_of(list_next(&tmp->list),struct task_struct,list);
	}

	list_add_to_before(&tmp->list,&tsk->list);
	task_schedule[SMP_cpu_id()].running_task_count += 1;
}

void schedule()
{
	struct task_struct *tsk = NULL;
	long cpu_id = SMP_cpu_id();

	// !!!maybe not schedule
	// cli();
	current->flags &= ~NEED_SCHEDULE;
	tsk = get_next_task();
#if DEBUG
/*
	color_printk(RED,BLACK,"[+]%s #cur 0x%p, list prev->0x%p, next->0x%p, pid: %d, vrun_time:%d; \
		\n\tnext task :0x%p, list prev->0x%p, next->0x%p, pid:%d, vrun_time:%d#\n", \
		__func__, current, current->list.prev, current->list.next, current->pid, current->vrun_time, \
		tsk, tsk->list.prev, tsk->list.next, tsk->pid, tsk->vrun_time);
	if(current == tsk)
		// __asm__ __volatile__("xchgw %bx, %bx");
*/
#endif
	// color_printk(RED,BLACK,"RFLAGS:%#018lx\n",get_rflags());
	// color_printk(RED,BLACK,"#schedule:%d#\n",jiffies);
	
	if(current->vrun_time >= tsk->vrun_time || current->state != TASK_RUNNING)
	{
		if(current->state == TASK_RUNNING)
			insert_task_queue(current);
			
		if(!task_schedule[cpu_id].CPU_exec_task_jiffies)
			switch(tsk->priority)
			{
				case 0:
				case 1:
					task_schedule[cpu_id].CPU_exec_task_jiffies = 4/task_schedule[cpu_id].running_task_count;
					break;
				case 2:
				default:
					task_schedule[cpu_id].CPU_exec_task_jiffies = 4/task_schedule[cpu_id].running_task_count*3;
					break;
			}
		
		switch_mm(current,tsk);
		switch_to(current,tsk);	
	}
	else
	{
		insert_task_queue(tsk);
		
		if(!task_schedule[cpu_id].CPU_exec_task_jiffies)
			switch(tsk->priority)
			{
				case 0:
				case 1:
					task_schedule[cpu_id].CPU_exec_task_jiffies = 4/task_schedule[cpu_id].running_task_count;
					break;
				case 2:
				default:
					task_schedule[cpu_id].CPU_exec_task_jiffies = 4/task_schedule[cpu_id].running_task_count*3;
					break;
			}
	}
	// sti();
}

void schedule_init()
{
	int i = 0;
	memset(&task_schedule,0,sizeof(struct schedule) * NR_CPUS);

	for(i = 0;i<NR_CPUS;i++)
	{
		list_init(&task_schedule[i].task_queue.list);
		task_schedule[i].task_queue.vrun_time = 0x7fffffffffffffff;

		task_schedule[i].running_task_count = 1;
		task_schedule[i].CPU_exec_task_jiffies = 4;
	}
}


