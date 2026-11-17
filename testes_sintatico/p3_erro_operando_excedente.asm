# Operandos excedentes: 3 erros, um por linha
.text
    syscall $t0
    jr $ra, $t0
    add $t0, $t1, $t2, $t3
    nop
