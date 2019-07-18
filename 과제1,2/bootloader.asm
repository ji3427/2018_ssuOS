org	0x7c00   

[BITS 16]

START:   
		jmp		BOOT1_LOAD ;BOOT1_LOAD로 점프

BOOT1_LOAD:
	mov     ax, 0x0900 
        mov     es, ax
        mov     bx, 0x0

        mov     ah, 2	
        mov     al, 0x4		
        mov     ch, 0	
        mov     cl, 2	
        mov     dh, 0		
        mov     dl, 0x80

        int     0x13	
        jc      BOOT1_LOAD
	
	mov ah,0x0f
	int 0x10 
	push ax ; 현재의 비디오 상태 저장
	mov dh,0x00 ;[o]의 초기 위치를 설정
	push dx
	
DRAW_SELECT: ;선택창을 표시하는 함수
	mov ah,0x00
	mov al,0x10
	int 0x10 ;비디오 상태 설정
	mov ax,ds
	mov es,ax
	lea bp,[ssuos_1] ;출력한 문자열 설정
	mov cx,0x0b ;문자열 개수
	mov al,0x00 ;쓰기모드
	mov bh,0x00 ;페이지
	mov bl,0x09 ;글자 속성
	mov dh,0x00 ;쓸 위치의 row
	mov dl,0x00 ;쓸 위치의 column
	mov ah,0x13
	int 0x10 ; 문자열 쓰기
	lea bp,[ssuos_2] ;출력할 문자열 설정
	mov dh,0x01 ;쓸 위치의 row
	int 0x10 ; 문자열 쓰기
	lea bp,[ssuos_3] ;출력할 문자열 설정
	mov dh,0x02 ;쓸 위치의 row
	int 0x10 ;문자열 쓰기 
	pop dx ; [o]를 쓸 위치를 불러옴  
	mov dl,0x00 ; 쓸 위치 column
	mov cx,0x03 ;문자의 개수
	lea bp,[select] ;쓸 문자를 [o]로 설정
	int 0x10 ;문자열 쓰기
	
	
MOVE_SELECT: ;키보드 인터럽트를 통해 키보드입력을 받아서 [o]의 위치를 변경해주는 함수
	mov ah,0x10
	int 0x16 ;키보드 입력을 기다림
	cmp ah,0x48 ;위 방향키가 입력된 경우
	je UP ; UP함수로 이동
	cmp ah,0x50 ;아래 방향키가 입력된 경우
	je DAWN ;DAWN함수로 이동
	cmp ah,0x1c ;엔터키가 입력된 경우
	je SELECT_END ; SELECT_END 함수로 이동
	jne MOVE_SELECT ;그외에 경우에는 다시 키보드를 입력 받음

UP:;위 방향키가 입력된 경우
	cmp dh,0
	je RESET_UP
	dec dh ;dh(쓸 위치의 row)를 감소시켜 [o]에 위치를 조정
	push dx 
	jmp DRAW_SELECT ; DRAW_SELECT로 이동하여 화면을 [o] 위치에 맞게 다시 화면 출력

DAWN:;아래 방향키가 입력된 경우
	cmp dh,2
	je RESET_DAWN
	inc dh ;dh(쓸 위치의 row)를 증가시켜 [o]의 위치를 조정
	push dx
	jmp DRAW_SELECT ; DRAW_SELECT로 이동하여 화면을 [o] 위치에 맞게 다시 화면 출력

RESET_UP: ;제일 상단에서 위 방향키가 입력된 경우 
	push dx 
	jmp DRAW_SELECT ;현재값을 유지한채 다시 DRAW_SELECT로 이동

RESET_DAWN: ;제일 하단에서 아래 방향키가 입력된 경우
	mov dh,0 ;[o]의 위치를 제일 상단으로 이동
	push dx
	jmp DRAW_SELECT ;이동한 값에 맞게 다시 DRAW_SELECT로 이동하여 그려줌
	
SELECT_END: ;enter키가 입력된 경우
	pop ax ;초기에 저장해둔 비디오 상태를 가져옴
	mov ah,0x00
	int 0x10 ; 비디오 상태 복구
	cmp dh,0 ;현재 [o]의 위치가 1번째 라인인 경우
	je KERNEL_1 ;KERENL_1으로 이동
	cmp dh,1 ;현재 [o]의 위치가 2번째 라인인 경우
	je KERNEL_2;KERENL_2으로 이동
	cmp dh,2 ;현재 [o]의 위치가 3번째 라인인 경우
	je KERNEL_3;KERENL_3으로 이동
	jne DRAW_SELECT ; 그 외에 경우 DRAW_SELECT로 이동
	
	

KERNEL_1: ;불러올 커널에 따른 CHS 값 설정(커널1)
	mov ah,2
	mov al,0x3f ;sector의 개수
	mov ch,0x00;Cylinder number
	mov cl,0x06;Sector number
	mov dh,0x00;Head number
	mov dl,0x80;drive number 80은 하드디스크
	push ax ;ax,bx,cx를 스택에 저장
	push cx
	push dx
	jmp KERNEL_LOAD ;kernel_load로 이동
	
KERNEL_2: ;불러올 커널에 따른 CHS 값 설정(커널2)
	mov ah,2
	mov al,0x3f ;sector의 개수
	mov ch,0x09;Cylinder number
	mov cl,0x2f;Sector number
	mov dh,0x0e;Head number
	mov dl,0x80;drive number 80은 하드디스크
	push ax ;ax,bx,cx를 스택에 저장
	push cx
	push dx
	jmp KERNEL_LOAD
KERNEL_3: ;불러올 커널에 따른 CHS 값 설정(커널3)
	mov ah,2
	mov al,0x3f ;sector의 개수
	mov ch,0x0e;Cylinder number
	mov cl,0x07;Sector number
	mov dh,0x0e;Head number
	mov dl,0x80;drive number 80은 하드디스크
	push ax ;ax,bx,cx를 스택에 저장
	push cx
	push dx
	jmp KERNEL_LOAD ;kernel_load로 이동

KERNEL_LOAD:
		mov     ax, 0x1000	
        mov     es, ax		
        mov     bx, 0x0		

		pop dx ; kernel을 load하기 위해 스택에 저장되어 있던 값을 꺼냄  
		pop cx
		pop ax

        int     0x13 ;저장된 값에 맞는 kernel을 load
        jc      KERNEL_LOAD

jmp		0x0900:0x0000

select db "[O]",0
ssuos_1 db "[ ] SSUOS_1",0
ssuos_2 db "[ ] SSUOS_2",0
ssuos_3 db "[ ] SSUOS_3",0
ssuos_4 db "[ ] SSUOS_4",0
partition_num : resw 1

times   446-($-$$) db 0x00

PTE:
partition1 db 0x80, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x3f, 0x0, 0x00, 0x00
partition2 db 0x80, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x3f, 0x0, 0x00, 0x00
partition3 db 0x80, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x98, 0x3a, 0x00, 0x00, 0x3f, 0x0, 0x00, 0x00
partition4 db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
times 	510-($-$$) db 0x00
dw	0xaa55
