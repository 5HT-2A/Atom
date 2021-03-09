#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <paging.h>
#include <kernel.h>

extern uint32_t *__KERNEL_END;
// A bitset of frames - used or free.
static uint32_t total_frames = NULL; //Max number of frames for installed memory
static uint32_t *frame_bitmap_array; //Pointer to array with the frames status (0 FREE, 1 USED)

uint32_t first_free_frame_index();
bool test_frame(uint32_t frame_addr);

// Function to allocate a frame.
void alloc_frame_int(page_frame_t *page, bool is_kernel, bool is_writeable, bool is_accessed, bool is_dirty, bool map_frame, uint32_t frameAddr){
  uint32_t idx;
  if(map_frame){
    idx = frameAddr >> 12;
  }else if(page->frame == 0){
    idx = first_free_frame_index(); // idx is now the index of the first free frame.
  } else {
    idx = page->frame;
  }
  
  set_frame(idx << 12); // this frame is now ours!
  page->ps = 0; //We are using 4KB pages
  page->present = 1; // Mark it as present.
  is_writeable = true;
  page->rw = (is_writeable == true) ? 1 : 0; // Should the page be writeable?
  is_kernel = false;
  page->user = (is_kernel == true) ? 0 : 1; // Should the page be user-mode?
  page->accessed = (is_accessed == true)?1:0;
  page->dirty = (is_dirty == true)?1:0;
  page->frame = idx;
}

void alloc_frame(page_frame_t *page, bool is_kernel, bool is_writeable){
  alloc_frame_int(page, is_kernel, is_writeable, false, false, false, NULL);
}

// Function to deallocate a frame.
void free_frame(page_frame_t *page){
  if(page == NULL){
    return;
  }
  if (!test_frame((page->frame) << 12)){
    return; // The given page didn't actually have an allocated frame!
  }else{
    clear_frame((page->frame) << 12); // Frame is now free again.
    *((uint32_t *)page) = 0;
  }
}

// Static function to set a bit in the frames bitset
void set_frame(uint32_t frame_addr){
  uint32_t frame = frame_addr/PAGE_SIZE;
  uint32_t idx = INDEX_FROM_BIT(frame);
  uint32_t off = OFFSET_FROM_BIT(frame);
  frame_bitmap_array[idx] |= (0x1 << off);
}


// Static function to clear a bit in the frames bitset
void clear_frame(uint32_t frame_addr){
  uint32_t frame = frame_addr/PAGE_SIZE;
  uint32_t idx = INDEX_FROM_BIT(frame);
  uint32_t off = OFFSET_FROM_BIT(frame);
  frame_bitmap_array[idx] &= ~(0x1 << off);
}

// Static function to test if a bit is set.
bool test_frame(uint32_t frame_addr){
  uint32_t frame = frame_addr/PAGE_SIZE;
  uint32_t idx = INDEX_FROM_BIT(frame);
  uint32_t off = OFFSET_FROM_BIT(frame);
  return (frame_bitmap_array[idx] & (0x1 << off)) != 0;
}

// Static function to find the first free frame.
uint32_t first_free_frame_index(){
  uint32_t i, j;
  for (i = 0; i < INDEX_FROM_BIT(total_frames); i++){
    if (frame_bitmap_array[i] != 0xFFFFFFFF){ // nothing free, exit early.
      // at least one bit is free here.
      for (j = 0; j < 32; j++){
        uint32_t toTest = 0x1 << j;
        if ( !(frame_bitmap_array[i]&toTest) ){
          return j+(i*8*4);
        }
      }
    }
  }
  //PANIC("No free frames!");
  printk("\nNo free frames!");
  asm("cli;hlt");
}

uint32_t first_frame() {
    uint32_t idx = first_free_frame_index(); // idx is now the index of the first free frame.
    set_frame(idx << 12);
    return (idx << 12);
}


void init_mmu(uintptr_t *kernel_base_ptr, uintptr_t *kernel_top_ptr)
{
  // HACK: Memory region provided by mmap_init is
  // ensured to be AT LEAST, 1MB in size, which is probably more
  // than enough for now, but this is hacky and should probably be avoided.

  // TODO: Return more information (Maybe a struct?) such as
  // top address and/or size just to name a few bits of stuff.

  // TODO: Whenever I manage to implement a menuconfig-like
  // config scheme, let the user decide if it should be compiled
  // using the Higher-Half memory scheme.

  // BUG?: If we have 1MB into address space as start address free, we might
  // have a bug where we overwrite kernel parts!
  // Kernel Pos after PADDR -> VADDR 0xC0100000;
  
  printk("[MMU] kernel_base_ptr: 0x%x, kernel_top_ptr: 0x%x\n", kernel_base_ptr, kernel_top_ptr);
  
  init_mmap(kernel_base_ptr, kernel_top_ptr);

  size_t memory_size = (memory_management_region_end - memory_management_region_start);
  printk("[MMU] Free Physical Memory Start: 0x%x; End: 0x%x. Size: %d MB\n", memory_management_region_start, memory_management_region_end, (((memory_size)/1024)/1024));
  total_frames = memory_size / PAGE_SIZE;
  
  // Place the next available virtual address the very first available location after the kernel ends
  // NOTE: Add up Virtual Base (0xC0000000) to mem_mgmt_reg_start to avoid referencing the physical (Unmapped) region
  uintptr_t first_free_virtual_addr = (memory_management_region_start + 0xC0000000);

  printk("[MMU] First Available Virtual Address 0x%x, Physical: 0x%x\n", first_free_virtual_addr, (first_free_virtual_addr - 0xC0000000));

  frame_bitmap_array = first_free_virtual_addr;

  // Get the total size of the bitmap array containing all the frames
  size_t frame_bitmap_array_size = INDEX_FROM_BIT(total_frames);

  if (frame_bitmap_array_size % 32 != 0)
  {
    frame_bitmap_array_size += 1;
  }

  // Clear the entire bitmap array
  memset(frame_bitmap_array, 0, (sizeof(size_t) * frame_bitmap_array_size));

  printk("[MMU] Frames-Containing Bitmap Array @ 0x%x\n", (uintptr_t)frame_bitmap_array);

  // Start the memory allocation routines
  kmalloc_init(first_free_virtual_addr, KERNEL_VIRTUAL_BASE);

#ifdef DEBUG_KMALLOC
  // This function merely checks if and only if the first allocation we do, resides inside the regions we provided.
  // This might come handy later on when we have to deal with merging two memory locations that contain available (Or Reclaimable) memory.
  uintptr_t kmalloc_test = (uintptr_t *) kmalloc (sizeof(uintptr_t) * frame_bitmap_array_size);
  printk("[DEBUG] Frames Array @ 0x%x; First Allocation @ 0x%x\n", frame_bitmap_array, kmalloc_test);
  
  // Check if the kmalloc'd location resides on a higher memory position than the bitmap array containing the frames
  if (kmalloc_test >= frame_bitmap_array)
  {
    printk("[DEBUG] Kmalloc test passed\n");
  } else {
    printk("[DEBUG] Kmalloc test failed\n");
  }
#endif

  printk("[MMU] Total frames: %d (%d bytes)\n", total_frames, memory_size);

  // TODO: Re-adjust the minimum needed memory for the kernel to work correctly; for now, just use a rudimentary 16MB boundary.
  if (total_frames <= 4)
  {
    //PANIC("INSTALLED MEMORY BELOW 16MB");
    printk("INSTALLED MEMORY BELOW 16MB");
    asm("cli;hlt");
  }
}