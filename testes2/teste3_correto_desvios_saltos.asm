.text
main:
    li $t0, 0
    li $t1, 10

loop:
    beq $t0, $t1, fim
    addi $t0, $t0, 1
    j loop

fim:
    li $v0, 4
    jal imprime
    j sair

imprime:
    jr $ra

sair:
    mul $t2, $t0, $t1
    div $t2, $t1
    mfhi $t3
    mflo $t4
    nop
    syscall
