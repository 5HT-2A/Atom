#include <kernel.h>
#include <stdbool.h>
#include <isr.h>

extern void* KERNEL_VADDR_END, * KERNEL_VADDR_START, * KERNEL_PHYSADDR_END, * KERNEL_PHYSADDR_START;

void _kmain(unsigned int initium_signature, uint32_t *initial_ebp, uint32_t *initial_esp)
{
	x86_setup_vga_text_mode();

	int initial_vga_offset = vga_get_cursor_offset();

    /* VGA 'cells' consist of the character and its control data
     * e.g. 'white on black background', 'red text on white bg', etc */

	// Check if we booted using Atom's Initium bootloader, if not, stop execution.
	// 0xA1B0516C =  Atom (A) Initium (I) Bootloader (B0) Signature (516) Check (C)
	if (initium_signature == 0xA1B0516C)
	{
		unsigned char* vga = (unsigned char*) 0xC00B8000;
		unsigned char bootedfrominitium[] = "[ATOM] Initium Bootloader detected!";

		for (int i = 0; i < (sizeof(bootedfrominitium) - 1); i++)
		{
				vga[initial_vga_offset] = bootedfrominitium[i];
				vga[initial_vga_offset + 1] = 0x0F;
				initial_vga_offset += 2;
		}
	} else {
		unsigned char* vga = (unsigned char*) 0xC00B8000;
		unsigned char notbootedfrominitium[] = "[ATOM] Initium Bootloader not detected!";

		for (int i = 0; i < (sizeof(notbootedfrominitium) - 1); i++)
		{
				vga[initial_vga_offset] = notbootedfrominitium[i];
				vga[initial_vga_offset + 1] = 0x4F;
				initial_vga_offset += 2;
		}
		// TODO: Do we really have to abort? If we got here, maybe another user booted
		// our kernel using a compatible bootloader, even if this doesn't gain
		// popularity, someone might want to try their bootloader's capabilities using
		// a random kernel (Mine in this case), so let's not prohibit them from booting the kernel
		// if they already got this far.
		/* for(;;); */
	}
    
    printk("\n");
    
    printkok("Booted to kernel mode!");

    printk("initial_esp: 0x%x, initial_ebp: 0x%x\n", initial_esp, initial_ebp);

#ifdef DEBUG // TODO: Differentiate / divide debug into subgroups
    printk("Kernel Start: 0x%x, Kernel End: 0x%x, Kernel Size: %d bytes (%d KB)\n", KERNEL_START, KERNEL_END, KERNEL_SIZE, ((KERNEL_END - KERNEL_START) / 1024));
#endif

    if (arch_gdt_install() == 1)
    {
        printkok("Configured GDT");
    } else {
        printkfail("Failed to configure GDT");
    }

    if (arch_idt_install() == 1)
    {
        printkok("Configured IDTs");
    } else {
        printkfail("Failed to configure IDTs");
    }

    if (arch_isr_install() == 1)
    {
        printkok("Configured ISRs");
    } else {
        printkfail("Failed to configure ISRs");
    }

    if (arch_irq_install() == 1)
    {
        printkok("Configured IRQs");
    } else {
        printkfail("Failed to confiugure IRQs");
    }

    if (arch_update_idt() == 1)
    {
        printkok("Flushed IDT");
    } else {
        printkfail("Failed to flush IDT");
    }

    if (arch_enable_interrupts() == 1)
    {
        printkok("Enabled Interrupts");
    } else {
        printkfail("Failed to enable Interrupts");
    }

    // Initialize the MMU
    init_mmu((uintptr_t) &KERNEL_PHYSADDR_START, (uintptr_t) &KERNEL_PHYSADDR_END);

    printkok("Initialized MMU");

    // We need the memory size in bits so that we can provide a good reference to the paging mechanism
    bool inKiloBytes = false;

    init_paging(get_max_phys_mem(inKiloBytes), (uintptr_t) &KERNEL_VADDR_START,
        (uintptr_t) &KERNEL_VADDR_END, (uintptr_t) &KERNEL_PHYSADDR_START, (uintptr_t) &KERNEL_PHYSADDR_END);

    printkok("Initialized Paging");

#ifdef DEBUG
    uintptr_t *p = (size_t*)kmalloc(sizeof(p));
    printk("p allocated at 0x%x\n", p);

    uintptr_t *p2 = (size_t*)kmalloc(sizeof(p2));
    printk("allocated 2 blocks for p2 at 0x%x\n", p2);

    /*kfree (p); TEST whenever we implement kfree
    p = (size_t*)kmalloc(sizeof(p));
    printk("Unallocated p to free block 1. p is reallocated to 0x%x", p);*/
#endif

#ifdef DEBUG
   // This code should work!
    uint32_t *ptr = (uintptr_t*)0xC0100000;
    uint32_t do_page_fault = *ptr;

    printk("1st Memory pointer accessed is 0xC0100000, mmu returns 0x%x\n", do_page_fault);

    uint32_t *ptr2 = (uintptr_t*)0xC0000000;
    uint32_t do_page_fault2 = *ptr2;

    printk("2nd Memory pointer accessed is 0xC0000000, mmu returns 0x%x\n", do_page_fault2);

    // This code should page fault because we're accessing a physical memory address that isn't paged (It doesn't point to an MMU aware section)
    uint32_t *ptr3 = (uintptr_t*)0x00100000;
    uint32_t do_page_fault3 = *ptr3;

    printk("3rd Memory pointer accessed is 0x00100000, mmu returns 0x%x\n", do_page_fault3);

    // This code should page fault because we don't have 0xA0000000 paged anywhere
    uint32_t *ptr4 = (uintptr_t*)0xA0000000;
    uint32_t do_page_fault4 = *ptr4;

    printk("4th Memory pointer accessed is 0xA0000000, mmu returns 0x%x\n", do_page_fault4);
    
#endif

    /* IRQ0: timer */
    init_timer(50);

    /* IRQ1: keyboard */
    init_keyboard();

    init_multitasking();

	for(;;);
}

void user_input(char *input)
{
    if (strcmp(input, "end") == 0) {
        printk("Stopping the CPU. Bye!\n");
        __asm__ __volatile__("hlt");
    } else if (strcmp(input, "int") == 0) {
    	/* Test the interrupts */
    	__asm__ __volatile__("int $2");
    	__asm__ __volatile__("int $3");
    } else if (strcmp(input, "tick") == 0) {
    	extern unsigned int tick;
    	printk("Ticks: %u\n", tick);    
    } else if (strcmp(input, "pfault") == 0) {
        uint32_t *ptr = (uint32_t*)0xA0000000;
        uint32_t do_page_fault = *ptr;
    } else if (strcmp(input, "a") == 0) {
        uint32_t *ptr = (uint32_t*)0x00100000;
        uint32_t do_page_fault = *ptr;
    } else if (strcmp(input, "b") == 0) {
        uint32_t *ptr = (uint32_t*)0xC0100000;
        uint32_t do_page_fault = *ptr;
    } else if (strcmp(input, "kinfo") == 0) {
        printk("Kernel PhysAddr Start: 0x%x, Kernel PhysAddr End: 0x%x\nKernel Size: %d bytes (%d KB)\n", (KERNEL_START - 0xC0000000), (KERNEL_END - 0xC0000000), KERNEL_SIZE, ((KERNEL_END - KERNEL_START) / 1024));
        printk("Kernel VirtAddr Start: 0x%x, Kernel VirtAddr End: 0x%x\n\n", KERNEL_START, KERNEL_END);
        printk("Memory management is using a %d MB region located \nPhysAddr: from 0x%x to 0x%x\nVirtAddr: from 0x%x to 0x%x", (((memory_management_region_end - memory_management_region_start) / 1024) / 1024),
        memory_management_region_start, memory_management_region_end, (memory_management_region_start + 0xC0000000), (memory_management_region_end + 0xC0000000));
    } else if (strcmp(input, "loop") == 0) {
        int i = 0;
        while (true) {
            switch (i) {
                case 0:
                    printk("\b|");
                    break;
                case 1:
                    printk("\b/");
                    break;
                case 2:
                    printk("\b-");
                    break;
                case 3:
                    printk("\b\\");
                    i = -1;
                    break;
                }
            i++;
        }
    } else {
    	printk("Unknown Command: ");
    	printk(input);
    }
    printk("\n> ");
}