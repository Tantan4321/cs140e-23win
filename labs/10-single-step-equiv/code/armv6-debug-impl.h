#ifndef __ARMV6_DEBUG_IMPL_H__
#define __ARMV6_DEBUG_IMPL_H__
#include "armv6-debug.h"

// all your code should go here.  implementation of the debug interface.

// example of how to define get and set for status registers
coproc_mk(status, p14, 0, c0, c1, 0)
coproc_mk(dscr, p14, 0, c0, c1, 0)
coproc_mk(dfsr, p15, 0, c5, c0, 0)
coproc_mk(wcr0, p14, 0, c0, c0, 7)
coproc_mk(wvr0, p14, 0, c0 ,c0, 6)
coproc_mk(ifsr, p15, 0, c5, c0, 1)
coproc_mk(ifar, p15, 0, c6, c0, 2)
coproc_mk(wfar, p14, 0, c0, c6, 0)

coproc_mk(bcr0, p14, 0, c0, c0, 5)
coproc_mk(bvr0, p14, 0, c0, c0, 4)

coproc_mk(far, p15, 0, c6, c0, 0)

// you'll need to define these and a bunch of other routines.
static inline uint32_t cp15_dfsr_get(void);
static inline uint32_t cp15_ifar_get(void);
static inline uint32_t cp15_ifsr_get(void);
static inline uint32_t cp14_dscr_get(void);

//static inline uint32_t cp14_wcr0_set(uint32_t r);

// return 1 if enabled, 0 otherwise.  
//    - we wind up reading the status register a bunch:
//      could return its value instead of 1 (since is 
//      non-zero).
static inline int cp14_is_enabled(void) {
    return bit_isset(cp14_status_get(), 15);
}

// enable debug coprocessor 
static inline void cp14_enable(void) {
    // if it's already enabled, just return?
    if(cp14_is_enabled())
        panic("already enabled\n");

    // for the core to take a debug exception, monitor debug mode has to be both 
    // selected and enabled --- bit 14 clear and bit 15 set.
    cp14_dscr_set(bits_set(cp14_dscr_get(), 14, 15, 0b10));
    prefetch_flush();

    assert(cp14_is_enabled());
}

// disable debug coprocessor
static inline void cp14_disable(void) {
    if(!cp14_is_enabled())
        return;

    unimplemented();

    assert(!cp14_is_enabled());
}


static inline int cp14_bcr0_is_enabled(void) {
    return bit_isset(cp14_bcr0_get(), 0);
}
static inline void cp14_bcr0_enable(void) {
    cp14_bcr0_set(bit_set(cp14_bcr0_get(), 0));  //read WCR and set bit 0
}
static inline void cp14_bcr0_disable(void) {
    cp14_bcr0_set(bit_clr(cp14_bcr0_get(), 0));  //read BCR and clear bit 0
}

// was this a brkpt fault?
static inline int was_brkpt_fault(void) {
    // use IFSR and then DSCR
    uint32_t v = cp15_ifsr_get();
    if(bit_is_off(v, 10) && bits_get(v, 0, 3) == 0b0010) {
        v = cp14_dscr_get();
        if (bits_get(v, 2, 5) == 0b0001) {
            return 1;
        }
    }
    return 0;
}

// was watchpoint debug fault caused by a load?
static inline int datafault_from_ld(void) {
    return bit_isset(cp15_dfsr_get(), 11) == 0;
}
// ...  by a store?
static inline int datafault_from_st(void) {
    return !datafault_from_ld();
}


// 13-33: tabl 13-23
static inline int was_watchpt_fault(void) {
    // use DFSR then DSCR
    uint32_t v = cp15_dfsr_get();
    if(bit_is_off(v, 10) && (bits_get(v, 0, 3) == 0b0010)) {
        v = cp14_dscr_get();
        if (bits_get(v, 2, 5) == 0b0010) {
            return 1;
        }
    }
    return 0;
}

static inline int cp14_wcr0_is_enabled(void) {
    return bit_isset(cp14_wcr0_get(), 0);
}
static inline void cp14_wcr0_enable(void) {
    cp14_wcr0_set(bit_set(cp14_wcr0_get(), 0));  //read WCR and set bit 0
}
static inline void cp14_wcr0_disable(void) {
    cp14_wcr0_set(bit_clr(cp14_wcr0_get(), 0));  //read WCR and clear bit 0
}

// Get watchpoint fault using WFAR
static inline uint32_t watchpt_fault_pc(void) {
    return (cp14_wfar_get() - 0x8);
}

// Get watchpoint fault addr using FAR
static inline uint32_t watchpt_fault_addr(void) {
    return (cp15_far_get());
}
    
#endif
