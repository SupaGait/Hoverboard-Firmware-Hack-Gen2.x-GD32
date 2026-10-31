MEMORY
{
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 4K
  RAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 8K
}

ENTRY(Reset_Handler)

SECTIONS
{
  .text :
  {
    KEEP(*(.isr_vector))
    . = ORIGIN(FLASH) + 0x200;
    KEEP(*(.iap_entry))
    *(.text*)
    *(.rodata*)
    _etext = .;
  } >FLASH

  _sidata = LOADADDR(.data);
  .data :
  {
    _sdata = .;
    *(.data*)
    _edata = .;
  } >RAM AT> FLASH

  .bss :
  {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    _ebss = .;
  } >RAM

  _estack = ORIGIN(RAM) + LENGTH(RAM);
}
