#include <interrupt.h>
#include <device/console.h>
#include <type.h>
#include <device/kbd.h>
#include <device/io.h>
#include <device/pit.h>
#include <stdarg.h>
#include <proc/proc.h>

char next_line[2]; //"\r\n";


//위 전역변수를 사용하는 코드를 아래 변수를 사용하는 코드로 변경
struct Console console[MAX_CONSOLE_NUM];
extern struct process *cur_process;
struct Console *cur_console;

void init_console(void)
{
	//콘솔 버퍼 초기화
	for(int i = 0 ; i < MAX_CONSOLE_NUM; i++){
		for(int j = 0 ; j < SIZE_SCROLL; j++){
			console[i].buf_s[j]=0;
		}
	}
		cur_console = get_console();//현재 콘솔에 새로운 콘솔 한당
		//콘솔 초기화
		cur_console->Glob_x = 0;
		cur_console->Glob_y = 2;
		cur_console->buf_w = cur_console->buf_s;
		cur_console->buf_p = cur_console->buf_s;
		cur_console->a_s = TRUE;
		cur_console->sum_y = 0;

	next_line[0] = '\r';
	next_line[1] = '\n';

}

void set_cursor(void)
{
	outb(0x3D4, 0x0F);
	outb(0x3D5, (cur_console->Glob_y*HSCREEN+cur_console->Glob_x)&0xFF);
	outb(0x3D4, 0x0E);
	outb(0x3D5, (((cur_console->Glob_y*HSCREEN+cur_console->Glob_x)>>8)&0xFF));
}

void PrintCharToScreen(int x, int y, const char *pString) 
{
	if(cur_process == NULL){ //init_proc 전에 init_console이 나오기 떄문에
		cur_console->Glob_x = x;
		cur_console->Glob_y = y;
		int i = 0;
		while(pString[i] != 0)
		{
			PrintChar(cur_console->Glob_x++, cur_console->Glob_y, pString[i++]);
		}
		cur_console->a_s = TRUE;
	}
	else{
		cur_process->console->Glob_x = x;
		cur_process->console->Glob_y = y;
		int i = 0;
		while(pString[i] != 0)
		{
			PrintChar(cur_process->console->Glob_x++, cur_process->console->Glob_y, pString[i++]);
		}
		cur_process->console->a_s = TRUE;
	}
}

void PrintChar(int x, int y, const char String) 
{
#ifdef SCREEN_SCROLL
	if(cur_process == NULL){//init_proc 전에 init_console이 나오기 떄문에
		if (String == '\n') {
			if((y+1) > VSCREEN) {
				scroll();
				y--;
			}
			cur_console->Glob_x = 0;
			cur_console->Glob_y = y+1;
			cur_console->sum_y++;
			return;
		}
		else if (String == '\b') {
			if(cur_console->Glob_x == 0) return;
			cur_console->Glob_x-=2;
			cur_console->buf_w[y * HSCREEN + x - 1] = 0;
		}
		else {
			if ((y >= VSCREEN) && (x >= 0)) {
				scroll();
				x = 0;
				y--;
			}      	              	

			char* b = &cur_console->buf_w[y * HSCREEN + x];
			if(b >= SCROLL_END)
				b-= SIZE_SCROLL;
			*b = String;

			if(cur_console->Glob_x >= HSCREEN)
			{
				cur_console->Glob_x = 0;
				cur_console->Glob_y++;
				cur_console->sum_y++;
			}    
		}
	}
	else{
		if (String == '\n') {
			if((y+1) > VSCREEN) {
				scroll();
				y--;
			}
			cur_process->console->Glob_x = 0;
			cur_process->console->Glob_y = y+1;
			cur_process->console->sum_y++;
			return;
		}
		else if (String == '\b') {
			if(cur_process->console->Glob_x == 0) return;
			cur_process->console->Glob_x-=2;
			cur_process->console->buf_w[y * HSCREEN + x - 1] = 0;
		}
		else {
			if ((y >= VSCREEN) && (x >= 0)) {
				scroll();
				x = 0;
				y--;
			}      	              	

			char* b = &cur_process->console->buf_w[y * HSCREEN + x];
			if(b >= SCROLL_END_P)
				b-= SIZE_SCROLL;
			*b = String;

			if(cur_process->console->Glob_x >= HSCREEN)
			{
				cur_process->console->Glob_x = 0;
				cur_process->console->Glob_y++;
				cur_process->console->sum_y++;
			}    
		}
	}


#endif
}

void clrScreen(void) 
{
	CHAR *pScreen = (CHAR *) VIDIO_MEMORY;
	int i;

	for (i = 0; i < 80*25; i++) {
		(*pScreen).bAtt = 0x07;
		(*pScreen++).bCh = ' ';
	}   
	cur_console->Glob_x = 0;
	cur_console->Glob_y = 0;
}

//Ctrl+l 화면 클리어 구현
void clearScreen(void)
{
	char *buf_ptr = cur_console->buf_w + SIZE_SCREEN;
	for(int i = 0; i < HSCREEN*25; i++) //함수 호출시 다음 페이지의 콘솔화면 초기화
	{
		if(buf_ptr >= SCROLL_END)
			buf_ptr -= SIZE_SCROLL;
		*(buf_ptr++) = 0;
	}
	
	set_fallow(); //pageup,pagedawn 을 초기화 시켜줌
	cur_console->buf_w += (cur_console->Glob_y*80); //현재 커서의 위치만큼 버퍼를 이동시켜 화면을 변경함
	while(cur_console->buf_w >= SCROLL_END) //버퍼가 버퍼의 최대값을 넘어간 경우 처음으로 돌림
		cur_console->buf_w -= SIZE_SCROLL;
		
	cur_console->buf_p = cur_console->buf_w; 
	cur_console->Glob_y = 0; //화면의 맨위로 옮겨졌기떄문에 커서 역시 맨위로
	set_cursor();
}

void scroll(void) 
{
	if(cur_process == NULL){
		cur_console->buf_w += HSCREEN;
		cur_console->buf_p += HSCREEN;

		while(cur_console->buf_w >= SCROLL_END)
			cur_console->buf_w -= SIZE_SCROLL;


		//clear line
		int i;
		char *buf_ptr = cur_console->buf_w + SIZE_SCREEN;
		for(i = 0; i < HSCREEN; i++)
		{
			if(buf_ptr >= SCROLL_END)
				buf_ptr -= SIZE_SCROLL;
			*(buf_ptr++) = 0;
		}

		//
		cur_console->Glob_y--;
	}
	else{
		cur_process->console->buf_w += HSCREEN;
		cur_process->console->buf_p += HSCREEN;

		while(cur_process->console->buf_w >= SCROLL_END_P)
			cur_process->console->buf_w -= SIZE_SCROLL;


		//clear line
		int i;
		char *buf_ptr = cur_process->console->buf_w + SIZE_SCREEN;
		for(i = 0; i < HSCREEN; i++)
		{
			if(buf_ptr >= SCROLL_END_P)
				buf_ptr -= SIZE_SCROLL;
			*(buf_ptr++) = 0;
		}

		//
		cur_process->console->Glob_y--;
	}


}

#ifdef SERIAL_STDOUT
void printCharToSerial(const char *pString)
{
	int i;
	enum intr_level old_level = intr_disable();
	for(;*pString != NULL; pString++)
	{
		if(*pString != '\n'){
			while((inb(LINE_STATUS) & THR_EMPTY) == 0)
				continue;
			outb(FIRST_SPORT, *pString);

		}
		else{
			for(i=0; i<2; i++){
				while((inb(LINE_STATUS) & THR_EMPTY) == 0)
					continue;
				outb(FIRST_SPORT, next_line[i]);
			}
		}
	}
	intr_set_level (old_level);
}
#endif


int printk(const char *fmt, ...)
{
	char buf[1024];
	va_list args;
	int len;

	va_start(args, fmt);
	len = vsprintk(buf, fmt, args);
	va_end(args);

#ifdef SERIAL_STDOUT
	printCharToSerial(buf);
#endif
	if(cur_process ==NULL){ //init_proc 전에 init_console이 나오기 떄문에
		PrintCharToScreen(cur_console->Glob_x, cur_console->Glob_y, buf);
	}
	else{
		PrintCharToScreen(cur_process->console->Glob_x, cur_process->console->Glob_y, buf);
	}

	set_cursor();
	return len;
}

#ifdef SCREEN_SCROLL
void scroll_screen(int offset)
{
	char * tmp_buf_p;
	char * tmp_buf_w;
	if(cur_console->a_s == TRUE && offset > 0 && cur_console->buf_p == cur_console->buf_w)
		return;

	cur_console->a_s = FALSE;

	tmp_buf_p = (char*)((int)cur_console->buf_p + (HSCREEN * offset));
	tmp_buf_w = cur_console->buf_w + SIZE_SCREEN;
	if(tmp_buf_w >= SCROLL_END)
		tmp_buf_w = (char *)((int)tmp_buf_w - SIZE_SCROLL);

	if(cur_console->sum_y < NSCROLL && offset < 0 && tmp_buf_p <= cur_console->buf_s && cur_console->buf_p >= cur_console->buf_s) return;
	if(offset > 0 && tmp_buf_p > cur_console->buf_w && cur_console->buf_p <= cur_console->buf_w) return;
	else if(offset < 0 && tmp_buf_p <= tmp_buf_w && cur_console->buf_p > tmp_buf_w) return;

	cur_console->buf_p = tmp_buf_p;

	if(cur_console->buf_p >= SCROLL_END)
		cur_console->buf_p = (char*)((int)cur_console->buf_p - SIZE_SCROLL);
	else if(cur_console->buf_p < cur_console->buf_s)
		cur_console->buf_p = (char*)((int)cur_console->buf_p + SIZE_SCROLL);

	refreshScreen();
}

void set_fallow(void)
{
	cur_console->a_s = TRUE;
}

void refreshScreen(void)
{
	CHAR *p_s= (CHAR *) VIDIO_MEMORY;
	int i;

	if(cur_console->a_s)
		cur_console->buf_p = cur_console->buf_w;

	char* b = cur_console->buf_p;

	for(i = 0; i < SIZE_SCREEN; i++, b++, p_s++)
	{
		if(b >= SCROLL_END)
			b -= SIZE_SCROLL;
		p_s->bAtt = 0x07;
		p_s->bCh = *b;
	}
}

//콘솔 할당 함수
struct Console *get_console(void){
	int i;

	for(i = 0; i < MAX_CONSOLE_NUM; i++){
		if(console[i].used == FALSE){
			console[i].used = TRUE;
			return &console[i];
		}
	}

	return NULL;
}
#endif

