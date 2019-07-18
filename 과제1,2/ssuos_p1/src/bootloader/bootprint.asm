org	0x7c00
[BITS 16]

START:   
mov		ax, 0xb800
mov		es, ax
mov		ax, 0x00
mov		bx, 0
mov		cx, 80*25*2
mov		si, 0
mov		di, 0

CLS:
mov		[es:bx], ax
add		bx, 1
loop 	CLS

mov ax,ds 
mov es,ax ;ds를 es에 저장
lea bp,[msg] ;출력할 문자열(msg)의 주소값을 bp에 저장
mov cx,0x17 ;문자열 개수
mov al,0x00
mov bh,0x00
mov bl,0x09 ;글자속성
mov ah,0x13 ;int 10h에서 문자열 출력을 위한 ah값
mov dx,0x00 ;출력할 위치 
int 0x10 ;문자열 출력





msg db "Hello, Hwayeong's World",0
