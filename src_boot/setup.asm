bits 64

global setup

extern gdt_descriptor
extern stack_top

section .data
testword: DW 0x55AA
value_37F: DW 0x37f
value_37E: DW 0x37e
value_37A: DW 0x37a

section .text
setup:
    cli
    lgdt [rel gdt_descriptor]
    mov rax, 0x18
    push rax
    lea rax, [rel .flush]
    push rax
    o64 retf

.flush:
    mov ax, 0x20 ; Set the A-register to the data descriptor.
    mov ds, ax ; Set the data segment to the A-register.
    mov es, ax ; Set the extra segment to the A-register.
    mov fs, ax ; Set the F-segment to the A-register.
    mov gs, ax ; Set the G-segment to the A-register.
    mov ss, ax ; Set the stack segment to the A-register.

    mov rax, cr0
    and rax, ~((1 << 2) | (1 << 3)) ; clear EM (no x87 emulation) and TS
    or rax, (1 << 1) | (1 << 5) ; MP (monitor coprocessor), NE (native FPU errors)
    mov cr0, rax

    ; Enable SSE: OSFXSR (bit 9) allows FXSAVE/SSE, OSXMMEXCPT (bit 10) routes SSE faults to #XM
    mov rax, cr4
    or rax, (1 << 9) | (1 << 10)
    mov cr4, rax

    fninit ; load defaults to FPU
    fnstsw [rel testword]
    cmp word [rel testword], 0
    jne .nofpu ; jump if the FPU hasn't written anything (i.e. it's not there)

    fldcw [rel value_37F] ; writes 0x37f into the control word: the value written by F(N)INIT
    fldcw [rel value_37E] ; writes 0x37e, the default with invalid operand exceptions enabled
    fldcw [rel value_37A] ; writes 0x37a, both division by zero and invalid operands cause exceptions.
    ret

.nofpu:
    ; Reset FPU to off
    mov rax, cr0
    or rax, (1 << 2)
    mov cr0, rax

section .note.GNU-stack noalloc noexec nowrite progbits