    .set noreorder
    .globl printf
    .ent printf

printf:
    li    $t2, 0xa0
    jr    $t2
    li    $t1, 0x3f
    
    .end printf

