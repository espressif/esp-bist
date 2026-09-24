/*
 * Live HP DRAM march bounds. Included from snippets-sections.ld, after
 * .dram0.noinit exists, so ADDR() is the first byte of that section.
 * The end is the SRAM region, not _image_ram_end (that symbol is the
 * end of the image; the heap is the unused tail after it).
 */
_bist_hp_ram_start = ADDR(.dram0.noinit);
_bist_hp_ram_end = ORIGIN(sram0_0_seg) + LENGTH(sram0_0_seg);
