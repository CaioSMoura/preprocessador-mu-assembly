.data
vetor: .word 1, 2, 3, 4, 5

.text
main:
    la $s0, vetor
    lw $t0, 0($s0)
    lw $t1, 4($s0)
    lw $t2, 8($s0)
    sw $t0, 12($s0)
    sw $t1, -4($sp)
    lb $t3, 0($gp)
    sh $t4, 2($fp)
    syscall
