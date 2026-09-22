.data
valor1: .word 10
valor2: .word 20

.text
main:
    lw $t0, valor1
    lw $t1, valor2
    add $t2, $t0, $t1

    li $v0, 1
    move $a0, $t2
    syscall

    li $v0, 10
    syscall
