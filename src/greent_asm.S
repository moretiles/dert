#include "greent_asm.h"

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
    
// struct my_jmp_buf
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

    //.global bar
    .global greent_root
    .global greent_yield
    .global greent_resume

    .text

greent_root:
    // store return address then increment stack back to what it was in caller
    mov (%rsp), %rdx
    add $8, %rsp

    // store everything
    xor %rax, %rax
    lea 0(%rip), %rcx
    test %rax, %rax
    jnz 98f
    mov %rdi, %rax
    mov %r12, VERT_R12_OFFSET(%rax)
    mov %r13, VERT_R13_OFFSET(%rax)
    mov %r14, VERT_R14_OFFSET(%rax)
    mov %r15, VERT_R15_OFFSET(%rax)
    mov %rbx, VERT_RBX_OFFSET(%rax)
    mov %rsp, VERT_RSP_OFFSET(%rax)
    mov %rbp, VERT_RBP_OFFSET(%rax)
    // %rcx currently holds %rip prior to cmp
    mov %rcx, VERT_RIP_OFFSET(%rax)
    // %rdx holds the return address we should use
    mov %rdx, VERT_FIRST_CALLED_FROM(%rax)
    sub $8, %rsp
    xor %rax, %rax
    jmp 99f

98:
    mov GREENT_PARENT_OFFSET(%rax), %rdx
    mov VERT_FIRST_CALLED_FROM(%rdx), %rdx
    push %rdx

99:
    ret
    
greent_yield:
    .equ greent_buffer, -8
    .equ return_value, -16
    PRE $16

    mov %rdi, greent_buffer(%rbp)
    //mov %rsi, return_value(%rbp)

    // copy stack
    mov GREENT_PARENT_OFFSET(%rdi), %rcx
    mov VERT_RSP_OFFSET(%rcx), %rsi
    sub %rsp, %rsi
    mov %rsi, GREENT_TOP_OF_FRAME_OFFSET(%rdi)
    mov %rsi, %rdx
    mov %rsp, %rsi
    lea GREENT_STACK_FRAMES_OFFSET(%rdi), %rdi
    call memcpy@PLT

    // store own registers
    mov greent_buffer(%rbp), %rax
    mov $0, %rdi
    lea 0(%rip), %rcx
    cmp $0, %rdi
    jne 99f
    mov %r12, GREENT_R12_OFFSET(%rax)
    mov %r13, GREENT_R13_OFFSET(%rax)
    mov %r14, GREENT_R14_OFFSET(%rax)
    mov %r15, GREENT_R15_OFFSET(%rax)
    mov %rbx, GREENT_RBX_OFFSET(%rax)
    mov %rsp, GREENT_RSP_OFFSET(%rax)
    mov %rbp, GREENT_RBP_OFFSET(%rax)
    // %rcx currently holds %rip prior to cmp
    mov %rcx, GREENT_RIP_OFFSET(%rax)

    // prepare the Green itself as return value
    mov %rax, %r8

    // want to load the parent root_buf registers, not own
    lea GREENT_PARENT_OFFSET(%rax), %rdx
    mov (%rdx), %rax
    xor %rdx, %rdx

    mov VERT_R12_OFFSET(%rax), %r12
    mov VERT_R13_OFFSET(%rax), %r13
    mov VERT_R14_OFFSET(%rax), %r14
    mov VERT_R15_OFFSET(%rax), %r15
    mov VERT_RBX_OFFSET(%rax), %rbx
    mov VERT_RSP_OFFSET(%rax), %rsp
    mov VERT_RBP_OFFSET(%rax), %rbp

    // prepare return address
    push VERT_RIP_OFFSET(%rax)

    // use Green as return value
    mov %r8, %rax

    // return as if using longjmp
    ret

99:
    POST

greent_resume:
    .equ greent_buffer, -8
    .equ return_value, -16
    // stack frames being created are weird...
    // the resume caller stack frame and stack frame prepared by resume hold lasting data
    // the stack frame used by resume itself is ephemeral
    //
    // ------------------------------
    // resume caller stack frame
    // ------------------------------
    // stack frame prepared by resume
    // ------------------------------
    // stack frame used by resume itself
    // ------------------------------
    // expanded version of resume macro

    // overwrite return address
    // want %rbp and %rsp to be the same as in caller at the start
    add $8, %rsp

    // decrement $rsp to create separate stack frame below yielded function stack frames
    // goal is to not interfere with memory region being used by memcpy
    mov %rdi, %rax
    mov GREENT_TOP_OF_FRAME_OFFSET(%rax), %rdx
    sub %rdx, %rsp
    mov %rsp, %rbp
    sub $24, %rsp
    movq %rdi, greent_buffer(%rbp)
    movq %rsi, return_value(%rbp)

    mov %rbp, %rdi
    lea GREENT_STACK_FRAMES_OFFSET(%rax), %rsi
    call memcpy@PLT
    
    mov %rbp, %rdi
    mov greent_buffer(%rbp), %rax
    mov GREENT_R12_OFFSET(%rax), %r12
    mov GREENT_R13_OFFSET(%rax), %r13
    mov GREENT_R14_OFFSET(%rax), %r14
    mov GREENT_R15_OFFSET(%rax), %r15
    mov GREENT_RBX_OFFSET(%rax), %rbx
    mov GREENT_RSP_OFFSET(%rax), %rsp
    mov GREENT_RBP_OFFSET(%rax), %rbp

    // return as if from function
    mov GREENT_RIP_OFFSET(%rax), %rcx
    push %rcx
    // rdi holds a %rbp relative to return_value
    mov return_value(%rdi), %rax
    ret

//bar:
    //PRE $64
    //.equ special2, -8
    //.equ counter_ptr, -16

    //xor %rax, %rax
    //lea counter_ptr(%rbp), %rsi
    //mov %rdi, (%rsi)
    //mov %rax, (%rdi)

    //lea special2(%rbp), %rax
    //movq $16, (%rax)

//bar_loop:
    //lea counter_ptr(%rbp), %rax
    //mov (%rax), %rdi
    //incl (%rdi)
    //mov (%rdi), %rsi
    //lea bar_fstring(%rip), %rdi
    //xor %rax, %rax
    //call printf@PLT

    //lea greent_buf(%rip), %rdi
    //call greent_yield

//bar_loop_body:
    //lea counter_ptr(%rbp), %rdi
    //movq (%rdi), %rsi
    //cmpq $5, (%rsi)
    //jl bar_loop

    //POST

//main:
    //.set counter, -4
    //.set special, -8
    //PRE $48
    //xor %rax, %rax
    //mov %eax, counter(%rbp)

    //mov $123456789, %eax
    //mov %eax, special(%rbp)

    //lea vert(%rip), %rdi
    //call greent_root

    //test %rax, %rax
    //jnz main_print_then_resume

    // pick one of these three function calls to uncomment
    //================================================

    // bar function
    //lea counter(%rbp), %rdi
    //call bar

    // print_and_yield function
    //lea greent_buf(%rip), %rdi
    //mov $2223, %rsi
    //call print_and_yield@PLT

    // fib function
    //lea greent_buf(%rip), %rdi
    //movq $19, %rsi
    //call fib_with_yield@PLT
    //lea fib_with_yield_fstring(%rip), %rdi
    //mov %rax, %rsi
    //call printf@PLT

    //================================================

    //jmp main_ret

//main_print_then_resume:
    //xor %rax, %rax
    //lea hello_from_main(%rip), %rdi
    //call printf@PLT

    //lea greent_buf(%rip), %rdi
    //mov $444, %rsi
    //call greent_resume

//main_ret:
    //POST

bar_fstring:
    .asciz "bar(%d) called!\n"

fib_with_yield_fstring:
    .asciz "fib_with_yield return %d!\n"

hello_from_main:
    .asciz "Hello from the main function in yield.S!\n"
