ENTRY(_start)
PHDRS
{
    text_seg PT_LOAD FLAGS(5);
    data_seg PT_LOAD FLAGS(5);
}

SECTIONS
{
    . = 0x400000;

    .text : {
        *(.text)
    } :text_seg

    .rodata : {
        *(.rodata)
    } :text_seg

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .data : {
        *(.data)
    } :data_seg

    .bss : {
        *(.bss)
        *(COMMON)
    } :data_seg
}
