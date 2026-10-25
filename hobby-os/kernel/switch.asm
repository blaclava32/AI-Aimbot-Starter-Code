; void task_switch_asm(uint32_t *old_esp, uint32_t new_esp)
; save callee-saved regs + return IP onto old stack, switch, restore

section .note.GNU-stack noalloc noexec nowrite progbits

global task_switch_asm

section .text
bits 32
task_switch_asm:
    mov eax, [esp + 4]      ; old_esp ptr
    mov ecx, [esp + 8]      ; new_esp value

    push ebx
    push esi
    push edi
    push ebp
    mov [eax], esp          ; save old esp

    mov esp, ecx            ; load new esp
    pop ebp
    pop edi
    pop esi
    pop ebx
    ret
