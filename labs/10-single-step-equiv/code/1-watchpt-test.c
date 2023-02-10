// handle a store to address 0 (null)
#include "rpi.h"
#include "vector-base.h"
#include "armv6-debug-impl.h"

static int load_fault_n, store_fault_n;

// change to passing in the saved registers.
void data_abort_vector(unsigned lr) {
    if(was_brkpt_fault())
        panic("should only get debug faults!\n");

    if(!was_watchpt_fault())
        panic("should only get watchpoint faults!\n");

    // instruction that caused watchpoint.
    uint32_t pc = watchpt_fault_pc();

    if(datafault_from_ld()) {
        trace("load fault at pc=%x\n", pc);
        assert(pc == (uint32_t)GET32);
        load_fault_n++;
    } else {
        trace("store fault at pc=%x\n", pc);
        assert(pc == (uint32_t)PUT32);
        store_fault_n++;
    }

    cp14_wcr0_disable();
}

extern uint32_t interrupt_vec;

void notmain(void) {

    // 1. install exception handlers using vector base.
    //      must have an entry for data-aborts that has
    //      a valid trampoline to call <data_abort_vector>
    vector_base_set((void *)&interrupt_vec);

    // 2. enable the debug coprocessor.
    cp14_enable();

    /* 
     * see: 
     *   - 13-47: how to set a simple watchpoint.
     *   - 13-17 for how to set bits in the <wcr0>
     */
    enum { null = 0 };

    //clear WCR[0] enable watchpoint bit in the read word and write it back to the WCR
    cp14_wcr0_disable();
    prefetch_flush();

    // just started, should not be enabled.
    assert(!cp14_wcr0_is_enabled());


    // now watchpoint is disabled, write DMVA to the WVR
    cp14_wvr0_set(null);  // 4 bit aligned, bottom 2 bits of WVR can get blasted
    prefetch_flush();


    uint32_t b = cp14_wcr0_get();
//    4.Write to the WCR with its fields set as follows:
//      WCR[20] enable linking bit cleared, to indicate that this watchpoint is not to be linked
    b = bit_clr(b, 20);
//    Watchpoints both in secure and non-secure
    b = bits_set(b, 14, 15, 0b00);
//    Byte address select for all accesses (0x0, 0x1, 0x2, 0x3).
    b = bits_set(b, 5, 8, 0b1111);
//    For both loads and stores.
//    Both priviledged and user.
//    WCR[0] enable watchpoint bit set.
    b = bits_set(b, 0, 4, 0b11111);

    cp14_wcr0_set(b);
    prefetch_flush();

    assert(cp14_wcr0_is_enabled());
    trace("set watchpoint for addr %p\n", null);

    trace("should see a store fault!\n");
    PUT32(null,0);

    if(!store_fault_n)
        panic("did not see a store fault\n");

    assert(!cp14_wcr0_is_enabled());
    cp14_wcr0_set(b);
    prefetch_flush();
    assert(cp14_wcr0_is_enabled());

    trace("should see a load fault!\n");
    GET32(null);
    if(!load_fault_n)
        panic("did not see a load fault\n");
    
    trace("SUCCESS\n");
}
