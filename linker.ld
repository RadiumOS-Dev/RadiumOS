/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_start)

/* Define memory regions */
MEMORY
{
    RAM (rw) : ORIGIN = 0x00100000, LENGTH = 64M  /* 1 MiB to 65 MiB */
    ROM (rx) : ORIGIN = 0x00000000, LENGTH = 1M    /* Bootloader area */
}

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS
{
    /* Begin putting sections at 1 MiB, a conventional place for kernels to be
       loaded at by the bootloader. */
    . = 1M;

    /* Multiboot header, required for bootloader recognition */
    .multiboot ALIGN(4K) : {
        *(.multiboot)
    }

    /* Kernel text section */
    .text ALIGN(4K) : {
        *(.text)
        *(.text.*)  /* Include all text sections */
    }

    /* Read-only data section */
    .rodata ALIGN(4K) : {
        *(.rodata*)
        *(.rodata.*)  /* Include all read-only data sections */
    }

    /* Initialized data section */
    .data ALIGN(4K) : {
        *(.data)
        *(.data.*)  /* Include all initialized data sections */
    }

    /* Uninitialized data section (BSS) */
    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss)
        *(.bss.*)  /* Include all uninitialized data sections */
    }

    /* Stack section */
    .stack ALIGN(4K) : {
        . = ALIGN(4K);
        . = . + 8K;  /* Reserve 8 KiB for the stack */
    }

    /* Heap section */
    .heap ALIGN(4K) : {
        . = ALIGN(4K);
        . = . + 32M;  /* Reserve 32 MiB for the heap */
    }

    /* Debugging information (optional) */
    .debug ALIGN(4K) : {
        *(.debug*)
    }

    /* Other sections can be added as needed */
    . = ALIGN(4K);
}