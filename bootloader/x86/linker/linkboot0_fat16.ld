ENTRY(init0_fat16)

OUTPUT_FORMAT(elf32-i386)

OUTPUT_ARCH(i386)

SECTIONS {

    . = 0x7C00;

    .text :
    {
        *(.text)
    }

    .rodata :
    {
        *(.rodata)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(.bss)
    }

    /DISCARD/ :
    {
      *(.stage2_discard)
    }
}
