#include <list.h>
#include <proc/sched.h>
#include <mem/malloc.h>
#include <proc/proc.h>
#include <proc/switch.h>
#include <interrupt.h>
#include <device/console.h>

extern struct list level_que[QUE_LV_MAX];
extern struct list plist;
extern struct list slist;
extern struct process procs[PROC_NUM_MAX];
extern struct process *idle_process;

struct process *latest = NULL;

struct process* get_next_proc(struct list *rlist_target);
void proc_que_levelup(struct process *cur);
void proc_que_leveldown(struct process *cur);
struct process* sched_find_que(void);

void print_list(struct list * l);

int scheduling;

/*
	linux multilevelfeedback queue scheduler
	level 1 que policy is FCFS(First Come, First Served)
	level 2 que policy is RR(Round Robin).
*/

//sched_find_que find the next process from the highest queue that has proccesses.
struct process* sched_find_que(void) {
	int i,j;
	struct process * result = NULL;
	 
	proc_wake();

		/*TODO :check the queue whether it is empty or not  
		 and find each queue for the next process.
		*/
	if(list_empty(&level_que[1])){ //list 1이 비워져 있는경우 
		if(list_empty(&level_que[2]) == FALSE){
			result = get_next_proc(&level_que[2]);
		}
	}
	else{ //list 1이 비워져 있지않은 경우
		result = get_next_proc(&level_que[1]);
	}
		
	return result;
}

struct process* get_next_proc(struct list *rlist_target) {
	struct list_elem *e;

	for(e = list_begin (rlist_target); e != list_end (rlist_target);
		e = list_next (e))
	{
		struct process* p = list_entry(e, struct process, elem_stat);

		if(p->state == PROC_RUN)
			return p;
	}
	return NULL;
}

void schedule(void)
{
	struct process *cur;
	struct process *next;
	struct process *tmp;
	struct list_elem *ele;
	int i = 0, printed = 0;
	scheduling = 1;	
	cur = cur_process;
	/*TODO : if current process is idle_process(pid 0), schedule() choose the next process (not pid 0).
	when context switching, you can use switch_process().  
	if current process is not idle_process, schedule() choose the idle process(pid 0).
	complete the schedule() code below.
	*/
	if ((cur -> pid) != 0) {
		latest = cur_process; //0이 아닌경우 0에서 queue이동을 위해 현재 프로세스를 저장해놓음
		cur_process = idle_process; //프로세스 변경
		intr_disable();
		switch_process(cur,idle_process);//프로세스 변경
		intr_enable();
		return;
	}
	else if(latest->pid <=10 && latest->pid > 0 && latest->state == PROC_RUN) { //idle process에서 lastest에 저장된 프로세스를 queue를 이동시킴
		switch (latest -> que_level){
			case 1: //que_level이 1인 경우 2번큐로 이동
				printk("proc %d change the queue(1->2)\n",latest -> pid);
				proc_que_leveldown(latest);
				break;
			case 2: //que_level이 2인 경우 2번큐의 마지막으로 이동
				list_remove(&latest->elem_stat);
				list_push_back(&level_que[2],&latest->elem_stat);
				break;
		}
		
	}
		
	proc_wake(); //wake up the processes 
	//print the info of all 10 proc.
	for (ele = list_begin(&plist); ele != list_end(&plist); ele = list_next(ele)) {
		tmp = list_entry (ele, struct process, elem_all);
		if ((tmp -> state == PROC_ZOMBIE) || 
			//(tmp -> state == PROC_BLOCK) || 
			//	(tmp -> state == PROC_STOP) ||
					(tmp -> pid == 0)) 	continue;
			if (!printed) {	
				printk("#=%2d, t=%3d, u=%3d, ", tmp -> pid, tmp -> time_slice, tmp -> time_used);
				printk(" q=%3d\n", tmp->que_level);
				printed = 1;			
			}
			else {
				printk("#=%2d, t=%3d, u=%3d, ", tmp -> pid, tmp -> time_slice, tmp->time_used);
				printk(" q=%3d\n", tmp->que_level);
				}
			
	}

	if (printed)
		printk("\n");
	if ((next = sched_find_que()) != NULL) {
		printk("Selected process : %d\n", next -> pid);
		scheduling = 0; //변수를 0으로 변경함으로써 타이머인터럽트가 다시 동작하게 변경
		cur_process = next;
		cur_process->time_slice = 0;
		intr_disable();
		switch_process(cur,next); //프로세스 변경
		intr_enable();
		return;
	}
	return;
}

void proc_que_levelup(struct process *cur)
{
	cur->que_level = 1;
	list_remove(&cur->elem_stat);
	list_push_back(&level_que[1],&cur->elem_stat);
	
}

void proc_que_leveldown(struct process *cur)
{
	cur->que_level = 2;
	list_remove(&cur->elem_stat);
	list_push_back(&level_que[2],&cur->elem_stat);
	/*TODO : change the queue lv1 to queue lv2.*/
}

void print_list(struct list * l){
	struct list_elem *e;
	int i = 0;
	for(e = list_begin (l); e != list_end (l);
		e = list_next (e))
	{
		struct process* p = list_entry(e, struct process, elem_stat);
	}
	
}
