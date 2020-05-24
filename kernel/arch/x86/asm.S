.code32
.section .data
i386_GDT_KERNEL_CODE_SEGMENT_SELECTOR: .long 0x00
.section .text
.global i386_gdt_update
i386_gdt_update:
	/* Load GDT */
	// Found this solution on StackOverflow, thanks MichaelPetch again!
	# So before landing here, we pushed 3 arguments to the stack:
	# arg[0]: struct i386_gdt_descriptor *i386_gdt_pointer
	# Which holds a pointer to the crafted GDT structure that lgdt will use lo update
	# the GDT-related stuff.
	# arg[1]: unsigned int i386_gdt_code_segment
	# Which holds the value of the GDT's Code Segment Selector.
	# arg[2]: unsigned int i386_gdt_data_segment
	# Which holds the value of the GDT's Data Segment Selector.

    mov 4(%esp), %edx		# GDT Pointer ()
    mov 12(%esp), %eax		# We'll place the Data Segment Selector (3rd Argument) on EAX...
    mov %eax, i386_GDT_KERNEL_CODE_SEGMENT_SELECTOR # ...and we'll also save it for future use.
    lgdt (%edx)				# Update the GDT with GDT record pointer passed as 1st argument.
    mov %eax, %ds			# Reload all the segment descriptors with EAX (Now containing
    						# Data Segment Selector, the 3rd Argument).
    mov %eax, %es
    mov %eax, %gs
    mov %eax, %fs
    mov %eax, %ss

    pushl 8(%esp)								# We will now push Code Segment Selector (3rd Argument) to the stack.
    											# NOTE: This will act as a Far Jump Pointer.
    push $.i386_gdt_update_set_code_segment		# The offset for the Far Jump's Function will be i386_gdt_update_set_code_segment
    											# label below, which is a specially crafted function that "clears out" the stack
    											# after using the argument's values. 
    ljmp *(%esp)								# Actually Far Jump to set the Code Segment Selector and the Instruction Pointer to
    											# the offset of i386_gdt_update_set_code_segment
.i386_gdt_update_set_code_segment:				# "Restore Stack" Subroutine:
    add $8, %esp								# We will remove both pushed stack values that contain the Segment Registers Information
												# that were used to craft the Pointers used in the Far Jump.
    ret											# Return to C

# Defined in isr.c
.extern isr_handler
.extern irq_handler

# Common ISR code
isr_common_stub: 
    # 1. Save CPU state
        pusha # Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax
        mov %ds,%ax # Lower 16-bits of eax = ds.
        push %eax # save the data segment descriptor
        mov i386_GDT_KERNEL_CODE_SEGMENT_SELECTOR,%ax # kernel data segment descriptor
        mov %ax,%ds
        mov %ax,%es
        mov %ax,%fs
        mov %ax,%gs
        push %esp # registers_t *r
    # 2. Call C handler
    cld # C code following the sysV ABI requires DF to be clear on function entry
        call isr_handler

    # 3. Restore state
        pop %eax
    pop %eax
        mov %ax,%ds
        mov %ax,%es
        mov %ax,%fs
        mov %ax,%gs
        popa
        add $8,%esp # Cleans up the pushed error code and pushed ISR number
        iret # pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP

# Common IRQ code. Identical to ISR code except for the 'call' 
# and the 'pop ebx'
irq_common_stub: 
    pusha
    mov %ds,%ax
    push %eax
    mov i386_GDT_KERNEL_CODE_SEGMENT_SELECTOR,%ax
    mov %ax,%ds
    mov %ax,%es
    mov %ax,%fs
    mov %ax,%gs
    push %esp
    cld
    call irq_handler # Different than the ISR code
    pop %ebx # Different than the ISR code
    pop %ebx
    mov %bx,%ds
    mov %bx,%es
    mov %bx,%fs
    mov %bx,%gs
    popa
    add $8,%esp
    iret

# We don't get information about which interrupt was caller
# when the handler is run, so we will need to have a different handler
# for every interrupt.
# Furthermore, some interrupts push an error code onto the stack but others
# don't, so we will push a dummy error code for those which don't, so that
# we have a consistent stack for all of them.

# First make the ISRs global
.global isr0
.global isr1
.global isr2
.global isr3
.global isr4
.global isr5
.global isr6
.global isr7
.global isr8
.global isr9
.global isr10
.global isr11
.global isr12
.global isr13
.global isr14
.global isr15
.global isr16
.global isr17
.global isr18
.global isr19
.global isr20
.global isr21
.global isr22
.global isr23
.global isr24
.global isr25
.global isr26
.global isr27
.global isr28
.global isr29
.global isr30
.global isr31
# IRQs
.global irq0
.global irq1
.global irq2
.global irq3
.global irq4
.global irq5
.global irq6
.global irq7
.global irq8
.global irq9
.global irq10
.global irq11
.global irq12
.global irq13
.global irq14
.global irq15

# 0: Divide By Zero Exception
isr0: 
    push $0
    push $0
    jmp isr_common_stub

# 1: Debug Exception
isr1: 
    push $0
    push $1
    jmp isr_common_stub

# 2: Non Maskable Interrupt Exception
isr2: 
    push $0
    push $2
    jmp isr_common_stub

# 3: Int 3 Exception
isr3: 
    push $0
    push $3
    jmp isr_common_stub

# 4: INTO Exception
isr4: 
    push $0
    push $4
    jmp isr_common_stub

# 5: Out of Bounds Exception
isr5: 
    push $0
    push $5
    jmp isr_common_stub

# 6: Invalid Opcode Exception
isr6: 
    push $0
    push $6
    jmp isr_common_stub

# 7: Coprocessor Not Available Exception
isr7: 
    push $0
    push $7
    jmp isr_common_stub

# 8: Double Fault Exception (With Error Code!)
isr8: 
    push $8
    jmp isr_common_stub

# 9: Coprocessor Segment Overrun Exception
isr9: 
    push $0
    push $9
    jmp isr_common_stub

# 10: Bad TSS Exception (With Error Code!)
isr10: 
    push $10
    jmp isr_common_stub

# 11: Segment Not Present Exception (With Error Code!)
isr11: 
    push $11
    jmp isr_common_stub

# 12: Stack Fault Exception (With Error Code!)
isr12: 
    push $12
    jmp isr_common_stub

# 13: General Protection Fault Exception (With Error Code!)
isr13: 
    push $13
    jmp isr_common_stub

# 14: Page Fault Exception (With Error Code!)
isr14: 
    push $14
    jmp isr_common_stub

# 15: Reserved Exception
isr15: 
    push $0
    push $15
    jmp isr_common_stub

# 16: Floating Point Exception
isr16: 
    push $0
    push $16
    jmp isr_common_stub

# 17: Alignment Check Exception
isr17: 
    push $0
    push $17
    jmp isr_common_stub

# 18: Machine Check Exception
isr18: 
    push $0
    push $18
    jmp isr_common_stub

# 19: Reserved
isr19: 
    push $0
    push $19
    jmp isr_common_stub

# 20: Reserved
isr20: 
    push $0
    push $20
    jmp isr_common_stub

# 21: Reserved
isr21: 
    push $0
    push $21
    jmp isr_common_stub

# 22: Reserved
isr22: 
    push $0
    push $22
    jmp isr_common_stub

# 23: Reserved
isr23: 
    push $0
    push $23
    jmp isr_common_stub

# 24: Reserved
isr24: 
    push $0
    push $24
    jmp isr_common_stub

# 25: Reserved
isr25: 
    push $0
    push $25
    jmp isr_common_stub

# 26: Reserved
isr26: 
    push $0
    push $26
    jmp isr_common_stub

# 27: Reserved
isr27: 
    push $0
    push $27
    jmp isr_common_stub

# 28: Reserved
isr28: 
    push $0
    push $28
    jmp isr_common_stub

# 29: Reserved
isr29: 
    push $0
    push $29
    jmp isr_common_stub

# 30: Reserved
isr30: 
    push $0
    push $30
    jmp isr_common_stub

# 31: Reserved
isr31: 
    push $0
    push $31
    jmp isr_common_stub

# IRQ handlers
irq0: 
        push $0
        push $32
        jmp irq_common_stub

irq1: 
        push $1
        push $33
        jmp irq_common_stub

irq2: 
        push $2
        push $34
        jmp irq_common_stub

irq3: 
        push $3
        push $35
        jmp irq_common_stub

irq4: 
        push $4
        push $36
        jmp irq_common_stub

irq5: 
        push $5
        push $37
        jmp irq_common_stub

irq6: 
        push $6
        push $38
        jmp irq_common_stub

irq7: 
        push $7
        push $39
        jmp irq_common_stub

irq8: 
        push $8
        push $40
        jmp irq_common_stub

irq9: 
        push $9
        push $41
        jmp irq_common_stub

irq10: 
        push $10
        push $42
        jmp irq_common_stub

irq11: 
        push $11
        push $43
        jmp irq_common_stub

irq12: 
        push $12
        push $44
        jmp irq_common_stub

irq13: 
        push $13
        push $45
        jmp irq_common_stub

irq14: 
        push $14
        push $46
        jmp irq_common_stub

irq15: 
        push $15
        push $47
        jmp irq_common_stub

