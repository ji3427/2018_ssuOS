#include <syscall.h>
#include <do_syscall.h>
#include <proc/proc.h>
#include <string.h>

/*
   시스템 콜 실행 과정
   syscall(유저) -> interrupt(커널)
   -> do_syscall(커널) -> syscall(유저 공간으로 리턴)
*/

#define syscall0(SYS_NUM) ({				\
		int ret;							\
		__asm__ __volatile(					\
				"pushl %[num]\n\t"			\
				"int $0x30\n\t"				\
				"addl %%esp, 4"				\
				:							\
				: [num] "g" (SYS_NUM)		\
				);							\
		ret;								\
		})

#define syscall1(SYS_NUM, ARG0) ({				\
		int ret;								\
		__asm__ __volatile(						\
				"pushl %[arg0] \n\t"			\
				"pushl %[num] \n\t"				\
				"int $0x30\n\t"					\
				"addl %%esp, 8"					\
				:								\
				: [num] "g" (SYS_NUM),			\
				[arg0] "g" (ARG0)				\
				);								\
		ret;									\
		})

#define syscall2(SYS_NUM, ARG0, ARG1) ({		\
		int ret;								\
		__asm__ __volatile(						\
				"pushl %[arg1] \n\t"			\
				"pushl %[arg0] \n\t"			\
				"pushl %[num] \n\t"				\
				"int $0x30\n\t"					\
				"addl %%esp, 12"				\
				:								\
				: [num] "g" (SYS_NUM),			\
				[arg0] "g" (ARG0),				\
				[arg1] "g" (ARG1)				\
				);								\
		ret;									\
		})

#define syscall3(SYS_NUM, ARG0, ARG1, ARG2) ({	\
		int ret;								\
		__asm__ __volatile(						\
				"pushl %[arg2] \n\t"			\
				"pushl %[arg1] \n\t"			\
				"pushl %[arg0] \n\t"			\
				"pushl %[num] \n\t"				\
				"int $0x30\n\t"					\
				"addl %%esp, 16"				\
				:								\
				: [num] "g" (SYS_NUM),			\
				[arg0] "g" (ARG0),				\
				[arg1] "g" (ARG1),				\
				[arg2] "g" (ARG2)				\
				);								\
		ret;									\
		})

int syscall_tbl[SYS_NUM][2];

#define REGSYS(NUM, FUNC, ARG) \
	syscall_tbl[NUM][0] = (int)FUNC; \
syscall_tbl[NUM][1] = ARG; 

void init_syscall(void)
{
	REGSYS(SYS_FORK, do_fork, 3);
	REGSYS(SYS_EXIT, do_exit, 1);
	REGSYS(SYS_WAIT, do_wait, 1);
	REGSYS(SYS_SSUREAD, do_ssuread, 0);
	REGSYS(SYS_SHUTDOWN, do_shutdown, 0);
	REGSYS(SYS_OPEN, do_open, 2);
	REGSYS(SYS_READ, do_read, 3);
	REGSYS(SYS_WRITE, do_write, 3);
	REGSYS(SYS_LSEEK, do_lseek,3); //lseek 함수 syscall에 등록
}

void exit(int status)
{
	syscall1(SYS_EXIT, status);
}

pid_t fork(proc_func func, void* aux1, void* aux2)
{
	return syscall3(SYS_FORK, func, aux1, aux2);
}

pid_t wait(int *status)
{
	return syscall1(SYS_WAIT, status);
}

int ssuread()
{
	return syscall0(SYS_SSUREAD);
}

void shutdown(void)
{
	syscall0(SYS_SHUTDOWN);
}

int open(const char *pathname, int flags)
{
	return syscall2(SYS_OPEN, pathname, flags);
}

int read(int fd, char *buf, size_t len)
{
	return syscall3(SYS_READ, fd, buf, len);
}

int write(int fd, const char *buf, size_t len)
{
	return syscall3(SYS_WRITE, fd, buf, len);
}
int lseek(int fd,int offset,int whence,char * option){
	flag = 0;
	memset(_buf,0,BUFSIZ); //버퍼 초기화
	backup = cur_process->file[fd]->pos; //현재 위치를 저장 ->error 발생 시 복구하기 위해
	pos = cur_process->file[fd]->pos;
	if(whence == SEEK_END){ //SEEK_END인경우 현재 위치를 파일의 끝으로 변경
		pos = cur_process->file[fd]->inode->sn_size;
		cur_process->file[fd]->pos = pos;
	}
	else if(whence == SEEK_SET){ //SEEK_SET인경우 현재 위치를 파일의 시작으로 변경
		cur_process->file[fd]->pos = 0;
		pos = 0;
	}
	if(strcmp(option,"-e") == 0){ //e옵션이 들어온 경우
		if(offset + pos > (int)(cur_process->file[fd]->inode->sn_size)){ //파일의 끝보다 뒤로 옮기는 경우
			whence = SEEK_END;
			offset = pos + offset - cur_process->file[fd]->inode->sn_size; //확장할 크기를 구함
			cur_process->file[fd]->pos = cur_process->file[fd]->inode->sn_size;//현재 위치를 파일의 끝으로 변경
			for(num = 0 ; num <offset ; num++){ //확장
				write(fd,"0",1);
			}
			offset = 0;
		}
		else if(offset + pos < 0){ //파일의 시작보다 더 앞으로 옮기는 경우
			offset = 0;
			whence = SEEK_SET;
		}
	}
	else if(strcmp(option,"-a") == 0){ //a옵션의 경우
		if(offset > 0){ //offset이 양수인 경우에만 처리
			num = cur_process->file[fd]->pos; //현재 위치 백업
			do{ //do-else 문을 통해 버퍼가 꽉차있는 경우 처리
				memset(_buf,0,BUFSIZ);
				length = read(fd,_buf,BUFSIZ); //문자열을 읽어드림
				if(flag == 0){ //a옵션 수행시 pos를 기준으로 뒤에 문자열을 offset 만큼 이동
					cur_process->file[fd]->pos = num;
					cur_process->file[fd]->pos += offset;
				}
				write(fd,_buf,length); //옮겨진 위치에 문자열 작성
				flag = 1;
			}while(length == BUFSIZ); //꽉차게 버퍼를 읽은 경우			
			cur_process->file[fd]->pos = num; //위치를 기존에 위치에 백업
			for(num = 0 ; num < offset; num++){ //0으로 초기화
				write(fd,"0",1);
			}
			if(whence == SEEK_END){
				offset = 0;
			}
		}
	}
	else if(strcmp(option,"-re") == 0){
		if(offset + pos < 0){ //파일의 시작보다 앞으로 이동한 경우
			whence = SEEK_SET;
			cur_process->file[fd]->pos = 0;
			offset = (offset + pos) * -1; //확장할 offset 계산
			do{//do-else 문을 통해 버퍼가 꽉차있는 경우 처리
				memset(_buf,0,BUFSIZ);
				length = read(fd,_buf,BUFSIZ);
				if(flag == 0){ //re옵션 수행 시 기존의 문자열들을 옮기는 경우일때 공간을 만들어줌
					cur_process->file[fd]->pos = 0;
					for(num = 0 ; num < offset ; num++){
						write(fd,"0",1);
					}
				}
				cur_process->file[fd]->pos = offset;
				write(fd,_buf,length);
				flag = 1;
			}while(length == BUFSIZ);
			cur_process->file[fd]->pos = 0;
			offset = 0;
		}
		else if(offset + pos > (int)(cur_process->file[fd]->inode->sn_size)){ //파일의 끝을 벗아나는 경우
			whence = SEEK_END;
			offset = 0;
		}
		
	}
	else if(strcmp(option,"-c")== 0){ //C옵션의 경우
		offset = offset % ((int)(cur_process->file[fd]->inode->sn_size)+1); //모듈러 연산을 이용해서 이동해야할 offset을 구함
		if(offset + pos > (int)(cur_process->file[fd]->inode->sn_size)){ //파일의 끝보다 뒤로 이동하는 경우 파일의 시작으로 돌아와서 이동
			whence = SEEK_SET;
			offset  = offset - (cur_process->file[fd]->inode->sn_size + 1 - pos);	
		}
		else if(offset + pos < 0){ //파일의 시작보다 앞으로 이동하는 경우 파일의 끝으로 돌아와서 이동
			whence = SEEK_END;
			offset  = offset + pos + 1;
		}
	}
	cur_process->file[fd]->pos = backup;
	
	return syscall3(SYS_LSEEK,fd,offset,whence);
}


