# 0 "src/greent_asm.S"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "src/greent_asm.S"
# 1 "./header/greent_asm.h" 1
       
# 2 "src/greent_asm.S" 2

 .macro pre shift=$16
 push %rbp
 mov %rsp, %rbp
 sub \shift, %rsp

 # insure aligned to 16
 and $240, %spl
 .endm

 .macro post
 mov %rbp, %rsp
 pop %rbp
 ret
 .endm


    .struct 0
my_jump_buf.r12:
    .space 8
my_jump_buf.r13:
    .space 8
my_jump_buf.r14:
    .space 8
my_jump_buf.r15:
    .space 8
my_jump_buf.rbx:
    .space 8
my_jump_buf.rsp:
    .space 8
my_jump_buf.rbp:
    .space 8
my_jump_buf.rip:
    .space 8
my_jump_buf.stack_ptr:
    .space 256
my_jump_buf.stack_size:
    .space 8
my_jump_buf.parent:
    .space 8
my_jump_buf.sizeof:
    .space 336


    .global greent_root
    .global greent_yield
    .global greent_resume

    .text

greent_root:

    mov (%rsp), %rdx
    add $8, %rsp


    xor %rax, %rax
    lea 0(%rip), %rcx
    test %rax, %rax
    jnz 98f
    mov %rdi, %rax
    mov %r12, (0)(%rax)
    mov %r13, (8)(%rax)
    mov %r14, (16)(%rax)
    mov %r15, (24)(%rax)
    mov %rbx, (32)(%rax)
    mov %rsp, (40)(%rax)
    mov %rbp, (48)(%rax)

    mov %rcx, (56)(%rax)

    mov %rdx, (64)(%rax)
    sub $8, %rsp
    xor %rax, %rax
    jmp 99f

98:
    mov (1096)(%rax), %rdx
    mov (64)(%rdx), %rdx
    push %rdx

99:
    ret

greent_yield:
    .equ greent_buffer, -8
    .equ return_value, -16
    PRE $16

    mov %rdi, greent_buffer(%rbp)



    mov (1096)(%rdi), %rcx
    mov (40)(%rcx), %rsi
    sub %rsp, %rsi
    mov %rsi, (1088)(%rdi)
    mov %rsi, %rdx
    mov %rsp, %rsi
    lea (64)(%rdi), %rdi
    call memcpy@PLT


    mov greent_buffer(%rbp), %rax
    mov $0, %rdi
    lea 0(%rip), %rcx
    cmp $0, %rdi
    jne 99f
    mov %r12, (0)(%rax)
    mov %r13, (8)(%rax)
    mov %r14, (16)(%rax)
    mov %r15, (24)(%rax)
    mov %rbx, (32)(%rax)
    mov %rsp, (40)(%rax)
    mov %rbp, (48)(%rax)

    mov %rcx, (56)(%rax)


    mov %rax, %r8


    lea (1096)(%rax), %rdx
    mov (%rdx), %rax
    xor %rdx, %rdx

    mov (0)(%rax), %r12
    mov (8)(%rax), %r13
    mov (16)(%rax), %r14
    mov (24)(%rax), %r15
    mov (32)(%rax), %rbx
    mov (40)(%rax), %rsp
    mov (48)(%rax), %rbp


    push (56)(%rax)


    mov %r8, %rax


    ret

99:
    POST

greent_resume:
    .equ greent_buffer, -8
    .equ return_value, -16
# 166 "src/greent_asm.S"
    add $8, %rsp



    mov %rdi, %rax
    mov (1088)(%rax), %rdx
    sub %rdx, %rsp
    mov %rsp, %rbp
    sub $24, %rsp
    movq %rdi, greent_buffer(%rbp)
    movq %rsi, return_value(%rbp)

    mov %rbp, %rdi
    lea (64)(%rax), %rsi
    call memcpy@PLT

    mov %rbp, %rdi
    mov greent_buffer(%rbp), %rax
    mov (0)(%rax), %r12
    mov (8)(%rax), %r13
    mov (16)(%rax), %r14
    mov (24)(%rax), %r15
    mov (32)(%rax), %rbx
    mov (40)(%rax), %rsp
    mov (48)(%rax), %rbp


    mov (56)(%rax), %rcx
    push %rcx

    mov return_value(%rdi), %rax
    ret
# 284 "src/greent_asm.S"
bar_fstring:
    .asciz "bar(%d) called!\n"

fib_with_yield_fstring:
    .asciz "fib_with_yield return %d!\n"

hello_from_main:
    .asciz "Hello from the main function in yield.S!\n"
