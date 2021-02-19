#include <mmap.h>

// FIXME: For now, we'll just use 1 memory region, later
// on we'll keep track of all of them, order them and use
// them accordingly.
uintptr_t memory_management_region_start = 0;
uintptr_t memory_management_region_end = 0;

size_t e820_usable_memory_regions = 0;

// HACK: Returns ONLY the usable memory region base address if it has a MB or more
// TODO: Clean up the code, optimize it and return more information!
// TODO: Maybe reconfigure the memory map location as a argument so that it can be changed?
uintptr_t init_mmap(uintptr_t kernel_base_ptr, uintptr_t kernel_top_ptr)
{
    size_t e820_mmap_total_entries_for_counting = *(uintptr_t*)MEMORY_MAP_LOCATION;
    const struct bios_memmap_entry* entry = (const struct bios_memmap_entry*)(MEMORY_MAP_LOCATION + 8);

    uintptr_t e820_entry_base_address = 0;
    uintptr_t e820_entry_end_address = 0;
    size_t e820_entry_size = 0;

    size_t e820_counted_entries = 0;

    // Count total available entries
    while (e820_mmap_total_entries_for_counting-- != 0)
    {
        e820_counted_entries++;
    }

#ifdef DEBUG
    printk("Memory map report: %d entries\n", e820_counted_entries);
#endif

    // Only allocate as much entries as needed (Same as total mmap entries)
    // FIXME: Could be more optimal!
    struct e820_mmap_available_entries memory_map[e820_counted_entries];

    for (int i = 0; i <= e820_counted_entries; i++)
    {
        memory_map[e820_counted_entries].base_address = 0;
        memory_map[e820_counted_entries].end_address = 0;
#ifdef DEBUG
        printk("%d base: %d end %d\n", i, memory_map[e820_counted_entries].base_address, memory_map[e820_counted_entries].end_address);
#endif  
    }

    size_t e820_mmap_total_entries = *(uintptr_t*)MEMORY_MAP_LOCATION;

    while (e820_mmap_total_entries-- != 0)
    {

        e820_entry_size = entry->e820_entry_size & 0xFFFFFFFF;
        e820_entry_base_address = entry->e820_entry_base_address & 0xFFFFFFFF;
        e820_entry_end_address = e820_entry_base_address + e820_entry_size;

        const char* t = entry->e820_entry_type <= MEMORY_TYPE_BAD_AREA ?
        e820_type_string[entry->e820_entry_type] : e820_type_string[0];

        if (entry->e820_entry_type == MEMORY_TYPE_USABLE)
        {

            /*
                    Access e820_mmap_available_entries[x] (1 as an example):
                    e820_mmap_available_entries[1] -> Start Address:   0xAAAAAAAA
                                                        -> End Address:     0xBBBBBBBB
                
            */
            memory_map[e820_usable_memory_regions].base_address = e820_entry_base_address;
            memory_map[e820_usable_memory_regions].end_address = e820_entry_end_address;
            // Usable memory region!
            // We should keep track of it.
            // Later on, we should also check the size and see if it fits our needs.

#ifdef DEBUG
            printk("Found an usable memory portion! -> 0x%x -> 0x%x\n", memory_map[e820_usable_memory_regions].base_address, memory_map[e820_usable_memory_regions].end_address);
#endif

            e820_usable_memory_regions++; // This spans the real number, not the array describing it's base & end address!
        }

#ifdef DEBUG
		if (((e820_entry_size / 1024) / 1024) != 0) {
			printk("Start: 0x%x; End: 0x%x Size: %d bytes (%d MB); Type: %s\n", e820_entry_base_address, e820_entry_end_address, e820_entry_size, ((e820_entry_size / 1024) / 1024), t);
		} else {
			printk("Start: 0x%x; End: 0x%x Size: %d bytes; Type: %s\n", e820_entry_base_address, e820_entry_end_address, e820_entry_size, t);
		}
#endif

        ++entry;
    }

#ifdef DEBUG
    printk("e820_usable_memory_regions: %d, memory_locations_max[%d]\n", e820_usable_memory_regions, ((e820_usable_memory_regions * 2) - 1));
#endif

    //printk("MMAP total entries: %d\n", e820_counted_entries);

    size_t size = 0;

    for (int i = 0; i <= (e820_usable_memory_regions - 1); i++)
    {
        if (memory_map[i].end_address != 0) // For now use end!
        {
#ifdef DEBUG
            printk("MMAP i: %d\n", i);
#endif
            size = (memory_map[i].end_address - memory_map[i].base_address);
#ifdef DEBUG
            printk("MMAP baddr: 0x%x endaddr: 0x%x, size: %d\n", memory_map[i].base_address, memory_map[i].end_address, size);
#endif

            if (size >= 0x100000) // 1MB
            {
#ifdef DEBUG
                printk("HEY!, we got start: 0x%x, end: 0x%x\n", memory_map[i].base_address, memory_map[i].end_address);
#endif

                memory_management_region_start = memory_map[i].base_address;
                memory_management_region_end = memory_map[i].end_address;
            }
        }
    }

    printk("memory_management_region_start: 0x%x\n", memory_management_region_start);

    // Check if they collide!
    // TODO: Revise the logic, there are many other possible cases where memory corruption might happen!
    if (memory_management_region_start == kernel_base_ptr)
    {
        printk("[MMAP] Region Overlap!\n   |_- Details:\n       memory_management_region_start: 0x%x kernel_base_ptr: 0x%x\n       memory_management_region_end: 0x%x kernel_top_ptr: 0x%x\n", memory_management_region_start, kernel_base_ptr, memory_management_region_end, kernel_top_ptr);
        printk("[MMAP] Masking the kernel portion of the first free memory hole as non-usable...");
        memory_management_region_start = (memory_management_region_start + ((kernel_top_ptr - kernel_base_ptr) + 1));
        printk("[MMAP] New memory_management_region_start: 0x%x\n", memory_management_region_start);
        return memory_management_region_start;
    } else {
        return memory_management_region_start;
    }
}
