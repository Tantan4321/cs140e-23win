@ Must carefully read B2 of the armv6 manual.  You cannot rely on "well it
@ worked on my test case": mistakes cause stale data, which may or may not
@ show up in your test case (likely not) despite being wildly broken.
@
@ Note: Rd is allegedly a read-only parameter to these instructions, but
@ the manual says SBZ (should be zero).  I think (and the linux source seems
@ to think) that this means we have to explicitly set it to 0.  Not setting
@ Rd=0 is an easy mistake to make.  Perhaps is worth building in (Linux does
@ not, but) --- the cost of an extra mov is negligible given our other 
@ overheads.
@
@ Alot of people put these instructions in inline assembly, wrapped up by 
@ function calls (e.g., cs107e's MMU code).  This practice is dangerous. 
@ For example, if you are flushing caches, you cannot be sure that the 
@ function return, etc does not then reload them, esp w.r.t. the BTB.  I 
@ think the only safe approach is to put directly in assembly so you are 
@ guaranteed no branch, indirect jump, load/store happens in between.
@
@ A lot of MMU code out there appears pretty broken b/c they don't do enough
@ flushing operations and/or rely on gcc code generation to not do the above.

#include "rpi-asm.h"
#include "armv6-coprocessor-asm.h"

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ armv6-coprocessor-asm.h
@   - has many macros for the privileged instructions.
@   - many page numbers 
@
@ make sure you look at B2 as well as 

@ just read the cache type: use C code to figure out if it is unified
@ b6-13, b6-14
MK_FN(get_cache_type)
    asm_todo("implement this")
    bx lr

MK_FN(mmu_sync_pte_mods)
    mov r0, #0
    CLEAN_INV_DCACHE(r0)  @ arm1176 6.4.2 says that data cache must be disabled before
    DSB(r0)  @ typical code for writing PTE B2-23
    INV_TLB(r0) @ invalidate both DTLB and ITLB here
    FLUSH_BTB(r0) @ B2-23, need to do this after modifying PTE
    DSB(r0) @ ensure tlb invalidations complete and are visible
    PREFETCH_FLUSH(r0) @ need to flush after a BTB flush, also make TLB invalidation visible
    bx lr

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ part 1:  write to the domain register.
@    easy sequencing rule: look at 2.7.6
@
@ depending on your code, you might name this
@ something else (or inline into the c code):
@  void domain_access_ctrl_set(uint32_t dom_reg);
MK_FN(cp15_domain_ctrl_wr)
    mov r1, #0
    DOMAIN_CTRL_WR(r0)  @ need to write input param into Rd
    PREFETCH_FLUSH(r1)  @ according to section B2.7.6: any change to cp15 needs prefectchflush, no DSB needed
    bx lr

MK_FN(cp15_domain_ctrl_rd)
    mov r1, #0
    DOMAIN_CTRL_RD(r0)  @ need to read dom ctrl
    PREFETCH_FLUSH(r1)  @ according to section B2.7.6: any change to cp15 needs prefectchflush, no DSB needed
    bx lr

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ part 2: 
@ void mmu_reset(void)
@   - called on boot-up.
@   - clear the dcache, icache, tlbs: make sure you obey 
@     any rules from B2
MK_FN(mmu_reset)
    mov r0, #0
    @ first clear dcache and icache
    @ see page B2-22, data cache invalidate must come before instruc cache
    @ DO NOT clean data cache as it might potentially write garbage
    INV_ALL_CACHES(r0) @ invalidate both caches, do not need to DSB between as we aren't cleaning dcache
    FLUSH_BTB(r0) @ 2.7.5 Invalidate BTB: need this b/c the code changed and so the branch predictor contents could be stale
    DSB(r0) @ ensures completion of the ICache invalidation
    PREFETCH_FLUSH(r0) @ Need icache invalidation to be visible, top of B2-22

    @ Invalidate TLBs next, so follow TLB maintenance ops on 2.7.3
    @ Previous DSB guarantees visibility of dcache and icache invalidation
    INV_TLB(r0) @ invalidate both DTLB and ITLB here
    FLUSH_BTB(r0) @ B2-23, need to do this after modifying PTE
    DSB(r0) @ ensure tlb invalidations complete and are visible
    PREFETCH_FLUSH(r0) @ need to flush after a BTB flush, also make TLB invalidation visible
    bx lr



@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ part 3
@ void mmu_disable_set_asm(cp15_ctrl_reg1_t c);
@
@ given the control register value <c>, write it 
@ and do any needed sync
@
@ this is a pretty tricky routine.  make sure you handle the 
@ icache and dcache.   when the mmu is off, these mappings will
@ change.  also, dcache does not work with MMU off.
@
@ make sure you look at arm1176, 6-9.
@
MK_FN(mmu_disable_set_asm)
    @ note: this piece of code is mapped into the sample place in both
    @ MMU-enabled and MMU-disabled so BTB and Prefetch entries work identically
    @ to both MMU-enable and disable.   Without this requirement I believe is
    @ very hard to get right (perhaps impossible?) since we cannot flush
    @ the BTB and prefetch and switch MMU on/off atomically.  thus, would
    @ flush, but instructions could get fetched before we could disable,
    @ defeating the purpose.
    mov r1, #0
    CLEAN_INV_DCACHE(r1)  @ arm1176 6.4.2 says that data cache must be disabled before
    DSB(r1)
    CONTROL_REG1_WR(r0)  @ arm1176 6.4.2 to disable MMU
    PREFETCH_FLUSH(r1) @ need to prefetch flush after cp15 modification B2.7.6, do not need DSB

    INV_ICACHE(r1)  @ clear out icache on mmu disable, put into known state for next run (readme)
    FLUSH_BTB(r1)  @ 2.7.5: need this b/c the icache changed and so the branch predictor contents could be stale
    DSB(r1)
    PREFETCH_FLUSH(r1)

    INV_TLB(r1) @ readme states that we must invalidate TLBs aswell
    FLUSH_BTB(r1)  @ Since we are disabling the MMU here, we must invalidate BTB and PF
    DSB(r1)
    PREFETCH_FLUSH(r1)
    bx lr

@ void mmu_enable_set_asm(cp15_ctrl_reg1_t c);
@  called by <mmu.c:mmu_enable()>
@
@ make sure you look at arm1176, 6-9.
@
MK_FN(mmu_enable_set_asm)
    mov r1, #0
    CONTROL_REG1_RD(r2) @ disable the icache
    BIC r2, r2, #0x1000
    CONTROL_REG1_WR(r2)
    PREFETCH_FLUSH(r1) @ need to prefetch flush after cp15 modification B2.7.6

    INV_ICACHE(r1)  @ clear out icache before mmu enable, readme
    DSB(r1)

    CONTROL_REG1_WR(r0)  @ arm1176 6.4.1 to enable MMU after all the config steps
    PREFETCH_FLUSH(r1) @ need to prefetch flush after cp15 modification B2.7.6, do not
    FLUSH_BTB(r1)  @ Since we are modifying the MMU state, we must invalidate BTB and PF
    DSB(r1)
    PREFETCH_FLUSH(r1)

    bx lr

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ part 4
@
@ void cp15_set_procid_ttbr0(uint32_t proc_and_asid, fld_t *pt);
@     sequence from b2-25 + rules from 3-129
@
@ this is the fanciest one.
MK_FN(cp15_set_procid_ttbr0)
    mov r2, #0
    @ B2-25:
    DSB(r2)
    ASID_SET(r2) @ Change ASID to 0
    DSB(r2)
    PREFETCH_FLUSH(r2) @ B4-41

    @ Change Translation Table Base Register
    TTBR0_GET(r3) @ RWM from the TTR0 B4-41
    AND r3, r3, #0b11111  @ wipe out all bits above bit 4
    ORR r3, r3, r1 @ RWM or in our new address
    TTBR0_SET(r3)

    TTBR1_SET(r2) @ set TTBR1 to 0, reference Readme and B4-41
    TTBR_BASE_CTRL_WR(r2) @ Set ttbrd to 0, see readme

    FLUSH_BTB(r2) @ need to invalidate BTB after changing TTB regs section 2.7.5
    DSB(r2)
    PREFETCH_FLUSH(r2) @ B4-41
    ASID_SET(r0) @ Change ASID to new value
    DSB(r2) @ Atomicity, BTB flush must come after ASID
    FLUSH_BTB(r2) @ need to invalidate BTB after changing ASID
    DSB(r2)
    PREFETCH_FLUSH(r2)
    bx lr

MK_FN(cp15_ctrl_reg1_wr)
    mov r1, #0
    CONTROL_REG1_WR(r0)  @ need to write input param into Rd
    PREFETCH_FLUSH(r1)  @ according to section B2.7.6: any change to cp15 needs prefectchflush, no DSB needed
    bx lr

MK_FN(cp15_ctrl_reg1_rd)
    mov r1, #0
    CONTROL_REG1_RD(r0)  @ need to read control reg1
    PREFETCH_FLUSH(r1)  @ according to section B2.7.6: any change to cp15 needs prefectchflush, no DSB needed
    bx lr


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ you need this for pinned-vm.c
@
@ this just turns the MMU on for the first time.
@ r0 = asid
@ r1 = pointer to empty page table
@
@ is a combination/simplification of mmu_init() and 
@ mmu_enable_set_asm().
@
@ also could just call the above from C code! :)

@@@@@@@@@@@@@@@ IMPLEMENTED IN PINNED-VM.C @@@@@@@@@@@@@@@@@@@@@@@@@@
@MK_FN(mmu_on_first_time)
@   bx lr
