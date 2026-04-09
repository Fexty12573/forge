.section ".text.crt0","ax"
.global forge_start
.extern forge_module_runtime

forge_start:
    .word 0
    .word forge_mod0 - forge_start // offset to module header location

.section ".rodata.mod0"
.global forge_mod0
forge_mod0:
    .ascii "MOD0"
    .word  __dynamic_start__    - forge_mod0
    .word  __bss_start__        - forge_mod0
    .word  __bss_end__          - forge_mod0
    .word  0
    .word  0
    .word  forge_module_runtime - forge_mod0 // offset to runtime-generated module object
