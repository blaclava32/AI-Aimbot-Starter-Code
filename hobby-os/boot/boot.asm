; SparkOS — Multiboot2 entry (32-bit protected mode)
; GRUB loads us and jumps here with EAX = magic, EBX = multiboot info.

MB2_MAGIC        equ 0xE85250D6
MB2_ARCH_I386    equ 0
MB2_HEADER_LEN   equ header_end - header_start
MB2_CHECKSUM     equ -(MB2_MAGIC + MB2_ARCH_I386 + MB2_HEADER_LEN)

section .multiboot_header
align 8
header_start:
    dd MB2_MAGIC
    dd MB2_ARCH_I386
    dd MB2_HEADER_LEN
    dd MB2_CHECKSUM

    ; end tag
    dw 0
    dw 0
    dd 8
header_end:

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .note.GNU-stack noalloc noexec nowrite progbits

section .text
global _start
extern kernel_main

bits 32
_start:
    mov esp, stack_top
    ; Multiboot2: EAX = 0x36d76289, EBX = info struct
    push ebx
    push eax
    call kernel_main
.hang:
    cli
    hlt
    jmp .hang
