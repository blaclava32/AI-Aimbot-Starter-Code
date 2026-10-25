; SparkOS — Multiboot2 entry (32-bit protected mode)

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
    dw 0
    dw 0
    dd 8
header_end:

section .bss
align 16
stack_bottom:
    resb 32768
stack_top:

section .note.GNU-stack noalloc noexec nowrite progbits

section .text
global _start
extern kernel_main

bits 32
_start:
    mov esp, stack_top
    xor ebp, ebp
    push ebx              ; multiboot info
    push eax              ; magic
    call kernel_main
.hang:
    cli
    hlt
    jmp .hang
