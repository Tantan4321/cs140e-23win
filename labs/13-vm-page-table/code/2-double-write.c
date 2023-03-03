// see that you can grow the stack.
#include "vm-ident.h"
#include "libc/bit-support.h"
#include "vector-base.h"

fld_t * mmu_map_section_modify(fld_t *pt, uint32_t va, uint32_t pa, uint32_t dom) {
    // only 16 domains.
    assert(dom < 16);
    // 20 b/c we have 1MB sections.
    assert(mod_pow2(va, 20));
    assert(mod_pow2(pa, 20));

    // assign pte: call <fld_set_base_addr> to set <sec_base_addr>
    // set each pte entry to:
    //   1. global
    //   2. AP = full access in user and kernel (B4-9)
    //   3. APX = 0
    //   4. domain to <dom>
    //   5. TEX strongly ordered (B4-12)
    //   6. executable.
    fld_t *pte = pt +  (va >> 20);
    demand(mod_pow2(pa,20), addr is not aligned!);

    // make sure if you read it back, it's what you set it to.
    pte->sec_base_addr = (pa >> 20);

    // if the previous code worked, this should always succeed.
    assert((pte->sec_base_addr << 20) == pa);
    pte->nG = 1;
    pte->AP = 0b11;
    pte->APX = 0;
    pte->domain = dom;
    pte->TEX = 0b000;
    pte->XN = 0;
    pte->tag = 0b10;

    fld_print(pte);
    printk("my.pte@ 0x%x = %b\n", pt, *(unsigned*)pte);
    hash_print("MODIFIED PTE crc:", pte, sizeof *pte);
    return pte;
}


void vm_test(void) {
    enum { MB = 1024 * 1024 };
    // 1. init
    mmu_init();
    assert(!mmu_is_enabled());

    // ugly hack: we start by allocating a single page table at a known address.
    fld_t *pt = mmu_pt_alloc(4096);
//    assert(pt == (void*)0x100000);

    fld_t *pt2 = mmu_pt_alloc(4096);
//    assert(pt2 == (void*)0x200000);

    // 2. setup mappings

    // map the first MB: shouldn't need more memory than this.
    mmu_map_section(pt, 0x0, 0x0, dom_id);
    mmu_map_section(pt, 0x100000,  0x100000, dom_id);
    mmu_map_section(pt, 0x20000000, 0x20000000, dom_id);
    mmu_map_section(pt, 0x20100000, 0x20100000, dom_id);
    mmu_map_section(pt, 0x20200000, 0x20200000, dom_id);
    mmu_map_section(pt, STACK_ADDR-MB, STACK_ADDR-MB, dom_id);
    mmu_map_section(pt, INT_STACK_ADDR-MB, INT_STACK_ADDR-MB, dom_id);

    // ########### same for PT2!#####
    // map the first MB: shouldn't need more memory than this.
    mmu_map_section(pt2, 0x0, 0x0, dom_id);
    mmu_map_section(pt2, 0x100000,  0x100000, dom_id);

    mmu_map_section(pt2, 0x20000000, 0x20000000, dom_id);
    mmu_map_section(pt2, 0x20100000, 0x20100000, dom_id);
    mmu_map_section(pt2, 0x20200000, 0x20200000, dom_id);
    mmu_map_section(pt2, STACK_ADDR-MB, STACK_ADDR-MB, dom_id);
    mmu_map_section(pt2, INT_STACK_ADDR-MB, INT_STACK_ADDR-MB, dom_id);

    // 3. install fault handler to catch if we make mistake.
    extern uint32_t default_vec_ints[];
    vector_base_set(default_vec_ints);

    // 4. start the context switch:

    // set up permissions for the one domain we use rn.
    domain_access_ctrl_set(0b01 << dom_id*2);

    mmu_map_section_modify(pt, 0x200000, 0x200000, dom_id);
    mmu_map_section_modify(pt2, 0x200000, 0x300000, dom_id);

    mmu_sync_pte_mods();

    set_procid_ttbr0(65, 1, pt);
    mmu_enable();

    put32((void *)0x100000, 0xdeadbeef);
    put32((void *)0x200000, 0xdeadbeef);

    mmu_disable();
    set_procid_ttbr0(66, 2, pt2);
    mmu_init();
    mmu_enable();

    //get
    printk("Get from: %x val=%x\n", (void *)0x100000, get32((void *)0x100000));
    printk("Get from: %x val=%x\n", (void *)0x200000, get32((void *)0x200000));


//    // do we still have the value after disabling?
//    vm_ident_mmu_off();
//    assert(!mmu_is_enabled());

//    assert(get32(p) == 11);
    printk("******** success ************\n");
}

void notmain() {
    kmalloc_init_set_start((void *)OneMB, OneMB);
    output("checking that stack gets extended\n");
    vm_test();
}
