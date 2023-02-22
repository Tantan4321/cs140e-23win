// first test do low level setup of everything.
#include "rpi.h"
#include "pinned-vm.h"
#include "mmu.h"

#include "armv6-debug-impl.h"


void prefetch_abort_vector(unsigned pc) {
    uint32_t dfsr = cp15_dfsr_get();
    uint32_t dom = bits_get(dfsr, 4, 7);
    uint32_t status = bits_get(dfsr, 0, 3);

    trace("Prefetch abort occurred at PC: %x\n", pc);
    trace("Domain: %x with the DFSR status: %x\n", dom, status);
    domain_access_ctrl_set((DOM_client << (1*2)) | (DOM_client << (dom*2)));
    return;
}


// if we have a dataabort fault, call the watchpoint
// handler.
void data_abort_vector(unsigned pc) {
    uint32_t dfsr = cp15_dfsr_get();
    uint32_t dom = bits_get(dfsr, 4, 7);
    uint32_t status = bits_get(dfsr, 0, 3);

    trace("Data abort fault occurred at PC: %x\n", pc);
    trace("Domain: %x with the DFSR status: %x\n", dom, status);
    domain_access_ctrl_set((DOM_client << (1*2)) | (DOM_client << (dom*2)));
    return;
}


void notmain(void) { 
    enum { OneMB = 1024*1024};
    // map the heap: for lab cksums must be at 0x100000.

    kmalloc_init_set_start((void*)MB, MB);
    assert(!mmu_is_enabled());

    enum { dom_kern = 1,
           // domain for user = 2
           dom_user = 2,
           dom_heap = 3 // domain for the heap
    };

    // read write the first mb.
    uint32_t user_addr = OneMB * 16;
    assert((user_addr>>12) % 16 == 0);

    procmap_t p = procmap_default_mk(dom_kern);
    procmap_push(&p, pr_ent_mk(user_addr, MB, MEM_RW, dom_user));
//    procmap_push(&p, pr_ent_mk(heap, MB, MEM_RW, dom_heap));

    trace("about to enable\n");
    pin_mmu_on(&p);

//    lockdown_print_entries("about to turn on first time");
//    assert(mmu_is_enabled());
//    trace("MMU is on and working!\n");

    domain_access_ctrl_set((DOM_client << (dom_kern*2)) | (DOM_no_access << (dom_user*2)));
    uint32_t invalid = GET32(user_addr);

    domain_access_ctrl_set((DOM_client << (dom_kern*2)) | (DOM_no_access << (dom_user*2)));
    PUT32(user_addr, 0xe12fff1e);

    domain_access_ctrl_set((DOM_client << (dom_kern*2)) | (DOM_no_access << (dom_user*2)));
//    PUT32(user_addr, 0xe12fff1e);
    asm volatile("mov lr, %0": : "r" ((void *)user_addr));
//    asm volatile("bx lr");

    staff_mmu_disable();
    assert(!mmu_is_enabled());
    trace("MMU is off!\n");

    trace("SUCCESS!\n");
}
