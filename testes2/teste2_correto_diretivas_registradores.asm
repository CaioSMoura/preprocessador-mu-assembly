.data
buffer: .space 40
letra:  .byte 65
metade: .half 300
lista:  .word 1, 2, 3, 4

.text
main:
    li $s0, 0x1001
    and $s1, $s0, $s0
    or  $s2, $s0, $s1
    xor $s3, $s1, $s2
    nor $s4, $s3, $s4
    sll $s5, $s0, 2
    srl $s6, $s0, 2
    slt $s7, $s5, $s6
    move $k0, $gp
    move $k1, $sp
    move $t0, $fp
    move $t1, $ra
    syscall