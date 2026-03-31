OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kmain)

PHDRS
{
    text    PT_LOAD    FLAGS((1 << 0) | (1 << 2));
    rodata  PT_LOAD    FLAGS((1 << 2));
    data    PT_LOAD    FLAGS((1 << 1) | (1 << 2));
}

SECTIONS
{
    . = 0xffffffff80000000;

    .text : ALIGN(0x1000) {
        *(.text .text.*)
    } :text

    . = ALIGN(0x1000);

    .rodata : ALIGN(0x1000) {
        *(.rodata .rodata.*)
    } :rodata

    . = ALIGN(0x1000);

    .data : ALIGN(0x1000) {
        *(.data .data.*)
    } :data

    .bss : ALIGN(0x1000) {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    .requests : ALIGN(0x1000) {
        KEEP(*(.requests_start_marker))
        KEEP(*(.requests))
        KEEP(*(.requests_end_marker))
    } :data
}
