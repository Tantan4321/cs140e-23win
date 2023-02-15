#include "rpi.h"
#include "vector-base.h"

#include "armv6-debug-impl.h"


// disable mismatch
void brkpt_mismatch_stop(void) {
    cp14_bcr0_disable();
}

// enable mismatch
void brkpt_mismatch_start(void) {
    cp14_enable();

    // just started, should not be enabled.
    assert(!cp14_bcr0_is_enabled());

    uint32_t b = cp14_bcr0_get();
    b = bits_set(b, 20, 22, 0b100);
//    Watchpoints both in secure and non-secure
    b = bits_set(b, 14, 15, 0b00);
//    Byte address select for all accesses (0x0, 0x1, 0x2, 0x3).
    b = bits_set(b, 5, 8, 0b1111);
    b = bits_set(b, 0, 2, 0b111);

    cp14_bcr0_set(b);
    prefetch_flush();

    assert(cp14_bcr0_is_enabled());
}

// set breakpoint on <addr>
void brkpt_mismatch_set(uint32_t addr) {

    cp14_bvr0_set(addr);
    prefetch_flush();
}

// is it a breakpoint fault?
int brkpt_fault_p(void) {
    return was_brkpt_fault();
}
