; Copyright (C) 2014  Arjun Sreedharan
; License: GPL version 2 or higher http://www.gnu.org/licenses/gpl.html

bits 32
section .text
        ;multiboot spec
        align 4
        dd 0x1BADB002              ;magic
        dd 0x00                    ;flags
        dd - (0x1BADB002 + 0x00)   ;checksum. m+f+c should be zero

global start
global keyboard_handler
global read_port
global write_port
global load_idt
global restart
global exit
global exit_1


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


extern kmain 		;this is defined in the c file
extern keyboard_handler_main

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
	call    keyboard_handler_main
	iretd

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
start:
	;cli	;block interrupts
	mov esp, stack_space ;set stack pointer
	call kmain
	cli
	.loop:
		hlt 				;halt the CPU
    		jmp .loop
	;hlt	;halt (stop) the CPU

section .bss
resb 8192; 8KB for stack
stack_space:
