section .multiboot_header
header_start:
	dd 0xE85250D6	;магическое число (multiboot 2)
	dd 0 ;архитектура 0 (защищенный режим i386)
	dd header_end - header_start ; длина заголовка
	; контрольная сумма
	dd 0x100000000 - (0xE85250D6 + 0 + (header_end - header_start))

	; завершающий тэг
	dw 0 ;тип
	dw 0 ;флаги
	dd 8 ;размер
header_end:
section .text
bits 32 ;nasm directive - 32 bit
global start

; я добавил
global keyboard_handler
global read_port
global write_port
global load_idt
global restart
global exit
global exit_1
; я добавил


;;;;;;;;; for GDT initialisation ;;;;;;;;;;;;;;;;;

global gdt_flush
extern gp
gdt_flush:
lgdt [gp]
mov ax, 0x10
mov ds, ax; This line restarts the computer
mov es, ax
mov fs, ax
mov gs, ax
mov ss, ax
jmp 0x08:flush2
flush2:
ret               

;;;;;;;;; for GDT initialisation ;;;;;;;;;;;;;;;;;





; я добавил
extern keyboard_handler_main
; я добавил
 
extern kmain	;kmain is define in the c file


start:
	;cli	;block interrupts
	mov esp, stack_space ;set stack pointer
	call kmain
	cli
	.loop:
		hlt 				;halt the CPU
    		jmp .loop
	;hlt	;halt (stop) the CPU

; я добавил ---------------

read_port:
	mov edx, [esp + 4]
			;al is the lower 8 bits of eax
	in al, dx	;dx is the lower 16 bits of edx
	ret

write_port:
	mov   edx, [esp + 4]    
	mov   al, [esp + 4 + 4]  
	out   dx, al  
	ret

load_idt:
	mov edx, [esp + 4]
	lidt [edx]
	sti 				;turn on interrupts
	ret

keyboard_handler: 

	push ds
	push es
	push fs
	push gs
	pushad
                
	call    keyboard_handler_main

	popad
	pop gs
	pop fs
	pop es
	pop ds

	iretd

; я добавил ---------------


restart:
	mov al, 0xFE
	out 0x64, al

exit:
	mov ax, 0x1000
	mov ax, ss
	mov sp, 0xf000
	mov ax, 0x5307
	mov bx, 0x0001
	mov cx, 0x0003
	int 0x15
	
	ret

exit_1:
	; Connect to APM API
	mov ax, 0x5301
	xor bx, bx
	int 0x15
	
	; Try to set APM version (to 1.2)
	mov ax, 0x530e
	xor bx, bx
	mov cx, 0x0102
	int 0x15
	
	; Turn off the system
	mov ax, 0x5307
	mov bx, 0x0001
	mov cx, 0x0003
	int 0x15
	
	;Exit (for good measure and in case of failure)
	ret



section .bss
resb 8192	;8KB for stack
stack_space:
