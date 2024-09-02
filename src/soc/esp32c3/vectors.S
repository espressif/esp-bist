    /*
     * https://github.com/espressif/esp32c3-direct-boot-example/blob/main/common/vectors.S
     *
     * This is the vector table. MTVEC points here.
     * Use 4-byte intructions here. 1 instruction = 1 entry of the table.
     * The CPU jumps to MTVEC (i.e. the first entry) in case of an exception,
     * and (MTVEC & 0xfffffffc) + (mcause & 0x7fffffff) * 4, in case of an interrupt.
     */
    .global _vector_table
    .section .exception_vectors.text
    .balign 0x100
    .type _vector_table, @function
_vector_table:
    .option push
    .option norvc
    j _panic_handler            /* exception handler, entry 0 */
    .rept 31
    j _interrupt_handler        /* 31 identical entries, all pointing to the interrupt handler */
    .endr
    .option pop
    .size _vector_table, .-_vector_table

    /* This should be the exception handler.
     * It doesn't do anything useful yet, but you can set a breakpoint here, to see if an exception happens.
     */
    .global _panic_handler
    .weak _panic_handler
    .type _panic_handler, @function
_panic_handler:
    j _panic_handler
    .size  _panic_handler, .-_panic_handler

    /* This is the interrupt handler.
     * It is declared as a weak function, and can be overridden.
     */
    .global _interrupt_handler
    .weak _interrupt_handler
    .type _interrupt_handler, @function
_interrupt_handler:
    /* Before dispatch c handler, restore interrupt to enable nested intr */
    csrr    s1, mcause
    csrr    s2, mstatus
    /* call the C dispatcher */
    mv      a0, sp      /* argument 1, stack pointer */
    mv      a1, s1      /* argument 2, interrupt number (mcause) */
    /* mask off the interrupt flag of mcause */
    li	    t0, 0x00000fff
    and     a1, a1, t0
    jal     _global_interrupt_handler

    mret
    .size  _interrupt_handler, .-_interrupt_handler
