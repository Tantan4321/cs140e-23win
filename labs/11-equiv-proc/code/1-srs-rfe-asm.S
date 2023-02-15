#include "rpi-asm.h"

@ r0: memory to store into.
MK_FN(srs_super)
    mov r1, sp
    mov sp, r0
    srs sp, #SUPER_MODE
    mov sp, r1
    mov r0, lr
    bx lr

MK_FN(spsr_set)
    msr spsr, r0
    bx lr

@ pointer to the array passed in <r0>
@ use rfe to invoke it.
MK_FN(rfe_asm)
    mov sp, r0
    rfe sp
    asm_not_reached()


@ a trampoline that loads the stack pointer
@ and calls <rfe_c_code>
MK_FN(rfe_trampoline)
    mov sp, #INT_STACK_ADDR
    mov r0, sp
    bl rfe_c_code
    asm_not_reached()

.weak rfe_c_code
rfe_c_code:
    asm_not_reached()
