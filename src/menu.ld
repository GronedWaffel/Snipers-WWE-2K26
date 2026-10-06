ENTRY(menu_tick)
SECTIONS { . = 0x6000; .header : { KEEP(*(.header)) } .text : { *(.text*) } .rodata : { *(.rodata*) } .data : { *(.data*) } .bss : { *(.bss*) *(COMMON) } /DISCARD/ : { *(.eh_frame*) *(.comment*) *(.note*) } }
