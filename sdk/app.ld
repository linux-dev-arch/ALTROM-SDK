
OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)

ENTRY(_start)

/* Application memory region */
APP_START = 0x00000000;
APP_SIZE  = 0x003DA1FF;

SECTIONS
{
    . = APP_START;
    __app_start = .;

    .text :
    {
        KEEP(*(.text._start))
        *(.text .text.*)
    }

    .rodata :
    {
        *(.rodata .rodata.*)
    }

    .data :
    {
        *(.data .data.*)
    }

    .bss (NOLOAD) :
    {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }

    __app_end = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.ARM.attributes)
    }

    ASSERT(__app_end <= APP_START + APP_SIZE,
           "Application overflow")
}
