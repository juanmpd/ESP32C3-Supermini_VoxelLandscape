.section .rodata
.align 4
.global terrainHeights
terrainHeights:
    .incbin "data/D1.bin"

.align 4
.global terrainColors
terrainColors:
    .incbin "data/C1W_indexed.bin"
