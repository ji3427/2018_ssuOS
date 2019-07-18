#include <proc/sched.h>
#include <proc/proc.h>
#include <device/device.h>
#include <interrupt.h>
#include <device/kbd.h>
#include <filesys/file.h>

pid_t do_fork(proc_func func, void* aux1, void* aux2)
{
	pid_t pid;
	struct proc_option opt;

	opt.priority = cur_process-> priority;
	pid = proc_create(func, &opt, aux1, aux2);

	return pid;
}

void do_exit(int status)
{
	cur_process->exit_status = status; 	//종료 상태 저장
	proc_free();						//프로세스 자원 해제
	do_sched_on_return();				//인터럽트 종료시 스케줄링
}

pid_t do_wait(int *status)
{
	while(cur_process->child_pid != -1)
		schedule();
	//SSUMUST : schedule 제거.

	int pid = cur_process->child_pid;
	cur_process->child_pid = -1;

	extern struct process procs[];
	procs[pid].state = PROC_UNUSED;

	if(!status)
		*status = procs[pid].exit_status;

	return pid;
}

void do_shutdown(void)
{
	dev_shutdown();
	return;
}

int do_ssuread(void)
{
	return kbd_read_char();
}

int do_open(const char *pathname, int flags)
{
	struct inode *inode;
	struct ssufile **file_cursor = cur_process->file;
	int fd;

	for(fd = 0; fd < NR_FILEDES; fd++)
		if(file_cursor[fd] == NULL) break;

	if (fd == NR_FILEDES)
		return -1;

	if ( (inode = inode_open(pathname)) == NULL)
		return -1;
	
	if (inode->sn_type == SSU_TYPE_DIR)
		return -1;

	fd = file_open(inode,flags,0);
	
	return fd;
}

int do_read(int fd, char *buf, int len)
{
	return generic_read(fd, (void *)buf, len);
}
int do_write(int fd, const char *buf, int len)
{
	return generic_write(fd, (void *)buf, len);
}

int do_lseek(int fd,int offset,int whence){
	uint16_t *pos =  &(cur_process->file[fd]->pos);
	int backup = *pos; //에러시 복구를 위한 변수
	int temp = *pos;
	if(whence == SEEK_SET){ //seek_set인 경우 offset = 현재 위치
		temp = offset;
	}
	else if(whence == SEEK_CUR){ //seek_cur인 경우 offset + 현재위치 = 현재위치
		temp = temp + offset;
	}
	else if(whence == SEEK_END){
		temp = cur_process->file[fd]->inode->sn_size + offset; //seek_end인 경우 파일의 크기(파일의 끝) + offset = 현재위치 
	}
	else{
		
	}
	if(temp > (int)(cur_process->file[fd]->inode->sn_size) || temp < 0){ //파일의 범위를 넘어가는 경우 원래 위치로 변경시킨후 -1 return
		*pos = backup;
		return -1;
	}
	*pos = temp;
	return temp;
}
