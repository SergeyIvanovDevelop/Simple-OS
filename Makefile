CA = nasm
CC = gcc
CA_parametrs = -f elf32
CC_parametrs_without_error = -m32 -c
CC_parametrs_with_error = -fno-stack-protector -m32 -c
C_LD = ld
C_LD_parametrs = -m elf_i386 -T

.PHONY: default assembler_code c_code ld_code rename copy_kernel_to_boot

default : assembler_code c_code ld_code rename copy_kernel_to_boot
assembler_code: nasm/kernel.asm
	$(CA) $(CA_parametrs) nasm/kernel.asm -o nasm/kasm.o
c_code: c/kernel.c c/keyboard_map.h
	$(CC) $(CC_parametrs_with_error) c/kernel.c -o c/kc.o
ld_code: nasm/kasm.o c/kc.o ld/link.ld
	$(C_LD) $(C_LD_parametrs) ld/link.ld -o executable/kernel nasm/kasm.o c/kc.o
rename: executable/kernel
	cp executable/kernel executable/kernel-45
copy_kernel_to_boot: executable/kernel-45
	sudo cp executable/kernel-45 /boot/
clear:
	rm executable/kernel-45 executable/kernel c/kc.o nasm/kasm.o				
