#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <kernel.h>
#include <paging.h>

// Declarations
uint32_t kdmalloc_int(uint32_t sz, bool align, uint32_t *phy);
void assert_kd_active();

// Inner globals vars
bool stoped = false;
uintptr_t base_addr = NULL;
uintptr_t next_v_address;
uintptr_t next_p_address;

void assert_kd_active() {
	if (base_addr == NULL){
		printk("\nCalling kernel dumb malloc when not initialized");
		asm("cli;hlt");
	}
	if (stoped){
		printk("\nCalling kernel dumb malloc after disabling it");
		asm("cli;hlt");
	}
}

uint32_t kdmalloc_int(uint32_t sz, bool align, uint32_t *phy){
	assert_kd_active();
	// This is dumb malloc
	uint32_t addr_v = next_v_address;
	uint32_t addr_p = next_p_address;
	if (align && (addr_p & PAGE_MASK_L) != 0) {
		// Align the placement address;
		addr_p &= PAGE_MASK_H;
		addr_v &= PAGE_MASK_H;
		addr_p += PAGE_TAM;
		addr_v += PAGE_TAM;
	}
	addr_p += sz;
	addr_v += sz;
	if (addr_p >= (base_addr + KD_MAX_TAM)){
		*phy = NULL;
		return NULL;
	}
	uint32_t nframes = (sz / PAGE_TAM) + (sz % PAGE_TAM != 0 ? 1 : 0);
	for (uint32_t frame = 0; frame < nframes; frame++){
		set_frame(next_p_address + (PAGE_TAM * frame));
	}
	if (phy != NULL){
		*phy = addr_p;
	}
	next_v_address = addr_v;
	next_p_address = addr_p;
	return addr_v;
}


// Implementations of alloc.h

void kmalloc_init(uintptr_t start, uintptr_t base)
{
	next_v_address = start;
	printk("[MALLOC] Placement Address @ 0x%x\n", next_v_address);

	if ((next_v_address & PAGE_MASK_L) != 0)
	{
		next_v_address &= PAGE_MASK_H;
		next_v_address += PAGE_TAM;
	}

	next_p_address = next_v_address - base;
	base_addr = next_p_address;
}

void kmalloc_stop() {
	stoped = true;
}

uint32_t kmalloc(uint32_t sz){
	return kdmalloc_int(sz, 0, 0);
}

uint32_t kmalloc_a(uint32_t sz){
	return kdmalloc_int(sz, 1, 0);
}

uint32_t kmalloc_p(uint32_t sz, uint32_t *phy){
	return kdmalloc_int(sz, 0, phy);
}

uint32_t kmalloc_ap(uint32_t sz, uint32_t *phy){
	return kdmalloc_int(sz, 1, phy);
}

size_t get_max_phys_mem(bool inKiloBytes)
{
	if (inKiloBytes == true)
	{
		return (((memory_management_region_end - memory_management_region_start) / 1024));
	}
	else
	{
		return (memory_management_region_end - memory_management_region_start);
	}
} 
