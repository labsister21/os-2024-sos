global loader                        ; the entry symbol for ELF
global load_gdt                      ; load GDT table
global kernel_start_user_mode
global set_tss_register              ; set tss register to GDT entry
global kernel_execute_user_program
extern kernel_setup                  ; kernel C entrypoint
extern _paging_kernel_page_directory ; kernel page directory
extern _linker_kernel_stack_top

KERNEL_VIRTUAL_BASE equ 0xC0000000    ; kernel virtual memory
MAGIC_NUMBER        equ 0x1BADB002    ; define the magic number constant
FLAGS               equ 0x0           ; multiboot flags
CHECKSUM            equ -MAGIC_NUMBER ; calculate the checksum (magic number + checksum + flags == 0)


section .multiboot  ; GRUB multiboot header
align 4             ; the code must be 4 byte aligned
    dd MAGIC_NUMBER ; write the magic number to the machine code,
    dd FLAGS        ; the flags,
    dd CHECKSUM     ; and the checksum


; start of the text (code) section
section .setup.text 
loader equ (loader_entrypoint - KERNEL_VIRTUAL_BASE)
loader_entrypoint:         ; the loader label (defined as entry point in linker script)
    ; Set CR3 (CPU page register)
    mov eax, _paging_kernel_page_directory - KERNEL_VIRTUAL_BASE
    mov cr3, eax

    ; Use 4 MB paging
    mov eax, cr4
    or  eax, 0x00000010    ; PSE (4 MB paging)
    mov cr4, eax

    ; Enable paging
    mov eax, cr0
    or  eax, 0x80000000    ; PG flag
    mov cr0, eax

    ; Jump into higher half first, cannot use C because call stack is still not working
    lea eax, [loader_virtual]
    jmp eax

loader_virtual:
    mov dword [_paging_kernel_page_directory], 0
    invlpg [0]                                ; Delete identity mapping and invalidate TLB cache for first page
    mov esp, [_linker_kernel_stack_top] ; Setup stack register to proper location
    call kernel_setup
.loop:
    jmp .loop                                 ; loop forever


section .text
; More details: https://en.wikibooks.org/wiki/X86_Assembly/Protected_Mode
load_gdt:
    cli
    mov  eax, [esp+4]
    lgdt [eax] ; Load GDT from GDTDescriptor, eax at this line will point GDTR location
    
    ; Set bit-0 (Protection Enable bit-flag) in Control Register 0 (CR0)
    ; This is optional, as usually GRUB already start with protected mode flag enabled
    mov  eax, cr0
    or   eax, 1
    mov  cr0, eax

    ; Far jump to update cs register
    ; Warning: Invalid GDT will raise exception in any instruction below
    jmp 0x8:flush_cs
flush_cs:
    ; Update all segment register
    mov ax, 10h
    mov ss, ax
    mov ds, ax
    mov es, ax
    ret

set_tss_register:
    mov ax, 0x28 | 0 ; GDT TSS Selector, ring 0
    ltr ax
    ret

kernel_execute_user_program:
    mov eax, 0x20 | 0x3 ; Data segment
		mov ds, ax
		mov es, ax
		mov fs, ax
		mov gs, ax

		mov ecx, [esp+4]
		push eax ; Data segment
		mov eax, ecx
		add eax, 0x400000 - 4
		push eax ; Stack address
		pushf ; Flags
		mov eax, 0x18 | 0x3
		push eax ; Code segment
		mov eax, ecx
		push eax ; Program address

		iret

kernel_start_user_mode:
		mov eax, [esp + 4] ; Interrupt Frame source
		mov edi, [eax]
		mov esi, [eax + 4]
		mov ebp, [eax + 8]
		; mov esp, [eax + 12] ; Setup for iret
		; mov ebx, [eax + 16] ; Needed for temporary register later
		mov edx, [eax + 20]
		mov ecx, [eax + 24]
		; mov eax, [eax + 28] ; Needed for source address

		mov gs, [eax + 32]
		mov fs, [eax + 36]
		mov es, [eax + 40]
		mov ds, [eax + 44]

		; ignore int_number [eax + 48]
		; ignore error_code [eax + 52]

		; ; Setup for iret
		mov ebx, [eax + 72] ; Data segment for stack segment
		push ebx

		mov ebx, [eax + 68] ; User stack
		push ebx

		mov ebx, [eax + 64] ; eflags
		push ebx

		mov ebx, [eax + 60] ; cs
		push ebx

		mov ebx, [eax + 56] ; eip
		push ebx

		iret
