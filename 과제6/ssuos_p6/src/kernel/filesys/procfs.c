#include <filesys/procfs.h>
#include <filesys/vnode.h>
#include <proc/proc.h>
#include <list.h>
#include <string.h>
#include <ssulib.h>
#include <proc/proc.h>
#include <device/console.h>

extern struct list p_list;
extern struct process *cur_process;

struct vnode *init_procfs(struct vnode *mnt_vnode)
{
	mnt_vnode->v_op.ls = proc_process_ls;
	mnt_vnode->v_op.mkdir = NULL;
	mnt_vnode->v_op.cd = proc_process_cd;

	return mnt_vnode;
}

int proc_process_ls()
{
	int result = 0;
	struct list_elem *e;

	printk(". .. ");
	for(e = list_begin (&p_list); e != list_end (&p_list); e = list_next (e))
	{
		struct process* p = list_entry(e, struct process, elem_all);

		printk("%d ", p->pid);
	}
	printk("\n");

	return result;
}

int proc_process_cd(char *dirname)
{ 
	struct vnode * new = find_vnode(dirname);//. 혹은 ..으로 상위폴더로 진입했다가 다시 원래폴더로 돌아오는 경우 새로운 vnode가 만들어지지 않게 하기 위해
	struct list_elem *e;
	
	if(strcmp(dirname, ".") == 0){
		return 0;
	}else if(strcmp(dirname, "..") == 0){
		cur_process->cwd = cur_process->cwd->v_parent;
		return 0;
	}else if(strcmp(dirname, "/") == 0){
		cur_process->cwd = cur_process->rootdir;
		return 0;
	}

	if(new == NULL){ //처음으로 진입하는 경우
		for(e = list_begin (&p_list); e != list_end (&p_list); e = list_next (e))
		{
			struct process* p = list_entry(e, struct process, elem_all);
			if(*dirname-'0' == p->pid){ //인자로 들어온 dirname의 pid를 찾는다.
				//새로 vnode 할당 후 세팅
				new = vnode_alloc();
				memcpy(new->v_name, dirname, LEN_VNODE_NAME); 
				new->v_parent = cur_process->cwd;
				new->type = cur_process->cwd->type;
				//각 명령어를 세팅
				new->v_op.ls = proc_process_info_ls;
				new->v_op.cd = proc_process_info_cd;
				new->v_op.cat = proc_process_info_cat;
				//cur_process->cwd를 새로 만든 vnode로 변경
				cur_process->cwd = new;
				return 0;
			}
		}
		printk("no directory\n");
	}
	else{//진입한 적이있다가 다시 들어가는 경우
		cur_process->cwd = new; //find_vnode로 찾은 vnode를 cur_process의 cwd로 변경한다.
	}
	

}

int proc_process_info_ls()
{
	int result = 0;

	printk(". .. ");
	printk("cwd root time stack");

	printk("\n");

	return result;
}

int proc_process_info_cd(char *dirname)
{
	int temp;
	struct vnode * new = find_vnode(dirname); ////. 혹은 ..으로 상위폴더로 진입했다가 다시 원래폴더로 돌아오는 경우 새로운 vnode가 만들어지지 않게 하기 위해
	struct list_elem *e;
	
	if(strcmp(dirname, ".") == 0){
		return 0;
	}else if(strcmp(dirname, "..") == 0){
		cur_process->cwd = cur_process->cwd->v_parent;
		return 0;
	}else if(strcmp(dirname, "/") == 0){
		cur_process->cwd = cur_process->rootdir;
		return 0;
	}
	if(new == NULL){
		if(strcmp(dirname,"root") == 0 || strcmp(dirname, "cwd") == 0){ //cd root혹은 cd cwd인 경우
			//vnode 할당 및 세팅
			struct vnode * new = vnode_alloc();
			memcpy(new->v_name, dirname, LEN_VNODE_NAME);
			new->v_parent = cur_process->cwd;
			//명령어 세팅
			new->v_op.ls= proc_link_ls;
			cur_process->cwd = new;
			return 0;
		}
		else{
			printk("not use!\n"); //time이나 stack으로 cd한경우
			return 0;
		}
	}
	cur_process->cwd = new;
	return 0;
}

int proc_process_info_cat(char *filename)
{
	struct process *temp = get_process(*(cur_process->cwd->v_name) - '0'); //현재 진입한 프로세스의 process구조체를 가져와서 temp에 저장
	
	if(strcmp(filename,"time") == 0){ 
		printk("time_used %d\n",temp->time_used);
	}
	else if(strcmp(filename,"stack") == 0){
		printk("stack : %x\n",temp->stack);
	}
	else{
		printk("not use!\n");
	}

}

int proc_link_ls()
{
	struct process * temp;
	struct vnode * v_temp;
	printk(". .. ");
	if(strcmp(cur_process->cwd->v_name,"root") == 0){ //root인 경우  rootdir의 하위폴더 출력
		print_child(cur_process->rootdir);
	}
	else if(strcmp(cur_process->cwd->v_name,"cwd") == 0){ //cwd인 경우 cwd가 procfs인 경우 가장 최근에 ssufs인 폴더를 cwd로 둔다.
		temp = get_process(*(cur_process->cwd->v_parent->v_name) - '0');
		v_temp = temp->cwd;
		while(v_temp->info == NULL){
			v_temp = v_temp->v_parent;
		}
		if(v_temp->v_op.mkdir == NULL){
			v_temp = v_temp->v_parent;
		}
		print_child(v_temp);
	}

}
