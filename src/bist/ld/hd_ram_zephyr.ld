/*
 * Host Diagnostics safe RAM for the ESP32 HP Zephyr linker.
 *
 * Included from snippets-ram-sections.ld, before .dram0.noinit. The march
 * span starts at the first byte of .dram0.noinit (hd_ram_zephyr_span.ld),
 * so this section stays outside it.
 */
SECTION_PROLOGUE(.dram0.safe_ram, (NOLOAD),)
{
	. = ALIGN(16);
	_safe_ram_start = .;
	KEEP(*(.dram0.safe_ram))
	KEEP(*(.dram0.safe_ram.*))
	_safe_ram_end = .;
} GROUP_LINK_IN(RAMABLE_REGION)
