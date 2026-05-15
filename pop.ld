/* pop.ld — minimal linker script for flat binary */

ENTRY(pop_main)        /* entry point symbol */
SECTIONS {
  /* Place code at start of binary */
  . = 0x0;

  .text : {
    *(.text*)
  }

  .rodata : {
    *(.rodata*)
  }

  .data : {
    *(.data*)
  }

  .bss : {
    *(.bss*)
    *(COMMON)
  }
}