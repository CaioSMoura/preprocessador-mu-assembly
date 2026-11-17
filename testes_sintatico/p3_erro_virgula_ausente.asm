# Virgulas ausentes entre operandos: 3 erros, um por linha
.text
    add $t0 $t1, $t2
    lw $t0 4($sp)
    beq $t0, $t1 fim
fim:
    syscall
