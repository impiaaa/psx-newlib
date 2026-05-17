    .text
    .align  2
    .globl  printf
    .ent    printf
    .type   printf, @function
printf:
    addiu   $sp, $sp, -24
    sw      $ra, 20($sp)
    li      $v0, 0xA0
    li      $9, 0x3F
    jalr    $v0
    lw      $ra, 20($sp)
    addiu   $sp, $sp, 24
    jr      $ra
    .end printf

