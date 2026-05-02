bits 64
extern dispatch_syscall
extern kernel_rsp
extern user_rsp
extern user_rcx
extern user_r11

global handle_syscall
section .text
handle_syscall:
    ; rcx = user RIP (saved by syscall)
    ; r11 = user RFLAGS (saved by syscall)
    ; rax = syscall number
    ; rdi, rsi, rdx, r10 = args 1-4

    mov [user_rsp], rsp
    mov rsp, [kernel_rsp]

    mov [user_rcx], rcx    ; save user RIP
    mov [user_r11], r11    ; save user RFLAGS

    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov rcx, rdx
    mov rdx, rsi
    mov rsi, rdi
    mov rdi, rax
    call dispatch_syscall

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    mov rcx, [user_rcx]    ; restore user RIP
    mov r11, [user_r11]    ; restore user RFLAGS

    push rax
    mov ax, 0x2b        ; ring-3 data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    pop rax

    mov rsp, [user_rsp]

    o64 sysret


section .note.GNU-stack noalloc noexec nowrite progbits