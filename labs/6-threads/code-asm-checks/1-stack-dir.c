// checks if stack grows up or down.
#include "rpi.h"

// get the current sp register.
static inline uint32_t *sp_get(void) {
    uint32_t *sp = 0;
    asm volatile("mov %0, sp" : "=r"(sp));
    assert(sp);
    return sp;
}


uint32_t *recurse(int n) {
    uint32_t *sp = sp_get();
    if(n == 0) {
        return sp;
    }
    else {
        return recurse(n - 1);
    }
}

// implement this code
int stack_grows_up(void) {
    uint32_t *start_sp = sp_get();
    volatile int i = 4;
    uint32_t *end_sp = recurse(i);
//    printk("Before: %p\n", start_sp);
//    printk("After: %p\n", end_sp);
    return start_sp < end_sp;
}

/******************************************************************
 * below is test code: you don't have to modify it.
 */



// test if grows up is correct or not.
//
// at the end of the recursion
//  - if stack grows up, memeset 4k of the stack above the
//    current stack pointer (since won't be used)
//  - if grows down memset 4k of stack below (since won't be used)
int check_stack_dir(int n) {
    if(n>0)
        return 1 + check_stack_dir(n-1);

    uint32_t *sp = sp_get();
    if(stack_grows_up())  {
        output("grows up: about to memset above\n");
        memset(sp, 0, 4096);
    } else {
        output("grows down: about to memset below\n");
        memset(sp-1025, 0, 4096);
    }
    return 0;
}

void notmain(void) {
    int n = 10;

    if(check_stack_dir(n) != n)
        panic("invalid count?\n");
    output("SUCCESS\n");
}
