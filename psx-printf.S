    .text
    .align  2
    .globl  printf
    .ent    printf
    .type   printf, @function
printf:
    li      $t1, 0x3F
    li      $t2, 0xA0
    jr      $t2
    .end printf

