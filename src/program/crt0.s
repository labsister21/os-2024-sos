global _start
extern main

section .text
_start:
	call main
	mov eax, 123
	int 0x30
